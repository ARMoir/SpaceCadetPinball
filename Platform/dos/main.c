/* Native 8086 real-mode DOS / Tandy 1000 backend. */
#include "game.h"
#include "menu_art.h"
#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Table table;
static Game game;

/* F-002 /record flight recorder: only 14 seconds of state in a 256-frame
 * circular memory buffer. No disk writes in the active play loop, so even a
 * 7 MHz V20 pays only a few cheap stores per rendered frame when requested.
 * Use FLIGHT.TXT after Esc to diagnose a ball that vanishes near a ramp. */
typedef struct {
    unsigned short frame;
    short x,y,vx,vy;
    unsigned short border,flips[2],powered[2];
    unsigned char input,balls,waiting,in_shooter,event,charge;
} FlightTrace;
static FlightTrace flight[256];
static unsigned flight_next,flight_used;
static unsigned char __far *background;
static unsigned char __far *video;
static volatile unsigned char keys[128];
static volatile unsigned char pressed_keys[128];
static void (__interrupt __far *old_keyboard)(void);
static int sound_enabled=1, sound_ticks;
static int shake_enabled=1,video_shift=0;

/* Shift the 6845 start-address by one unit for a short screen jolt.
 * This shifts the visible image in hardware without copying 32 KB of
 * graphics. Always restore zero on pause/exit; /noshake disables it on
 * systems with unusual video hardware. */
static void camera_shift(int offset)
{
    if(!shake_enabled) offset=0;
    if(video_shift==offset) return;
    outp(0x3d4,13);
    outp(0x3d5,offset);
    video_shift=offset;
}

static void __interrupt __far keyboard(void)
{
    unsigned scan=inp(0x60), acknowledge=inp(0x61);
    outp(0x61,acknowledge|0x80); outp(0x61,acknowledge);
    /* XT/V20 make/break scancodes; extended prefixes are ignored. */
    if(scan!=0xe0 && scan!=0xe1) {
        unsigned code=scan&127;
        /* Menu and toggle keys must fire only once per physical press.
         * Flipper/launcher quick-tap latching stays unchanged. */
        if(!(scan&128) &&
           ((code!=1 && code!=28 && code!=31 && code!=49 &&
             code!=50 && code!=59 && code!=60 && code!=61 &&
             code!=72 && code!=80) || !keys[code]))
            pressed_keys[code]=1;
        keys[code]=(scan&128)?0:1;
    }
    outp(0x20,0x20);
}

static unsigned ticks(void)
{
    /* Atomic 16-bit low word. Unsigned subtraction handles wraparound. */
    return *(volatile unsigned __far *)MK_FP(0x40,0x6c);
}

static void mode(unsigned char value)
{
    union REGS r;
    memset(&r,0,sizeof(r)); r.h.ah=0; r.h.al=value; int86(0x10,&r,&r);
}

static unsigned address(int x,int y)
{ return (unsigned)((y&3)*8192+(y>>2)*160+(x>>1)); }

static void pixel(int x,int y,unsigned color)
{
    unsigned a;
    if(x<0 || x>=320 || y<0 || y>=200) return;
    a=address(x,y);
    if(x&1) video[a]=(video[a]&0xf0)|(unsigned char)color;
    else video[a]=(video[a]&0x0f)|(unsigned char)(color<<4);
}

static void restore(int x,int y,int w,int h)
{
    int row,start,end;
    unsigned a;
    if(x<0) { w+=x; x=0; } if(y<0) { h+=y; y=0; }
    if(x+w>320) w=320-x; if(y+h>200) h=200-y;
    if(w<=0 || h<=0) return;
    start=x/2; end=(x+w+1)/2;
    for(row=y;row<y+h;row++) {
        a=address(start*2,row);
        _fmemcpy(video+a,background+a,end-start);
    }
}

/* Draw a sprite by scanline. The Tandy's interleaved banks make a
 * pixel() call (with modulo and address calculation) expensive on a V20. */
static void sprite(const Sprite *s,int x,int y)
{
    int xx,yy,start,end,dst_y;
    unsigned a;
    for(yy=0;yy<s->h;yy++) {
        const unsigned char *p=s->pixels+yy*s->w;
        dst_y=y+yy;
        if(dst_y<0 || dst_y>=200) continue;
        start=x<0?-x:0;
        end=s->w;
        if(x+end>320) end=320-x;
        if(start>=end) continue;
        a=address(x+start,dst_y);
        for(xx=start;xx<end;xx++) {
            unsigned char c=p[xx];
            if(c!=255) {
                unsigned char old=video[a];
                if((x+xx)&1) video[a]=(old&0xf0)|c;
                else video[a]=(old&0x0f)|(unsigned char)(c<<4);
            }
            if((x+xx)&1) a++;
        }
    }
}

/* A stationary flipper only needs repainting if the old ball was erased
 * over it. Otherwise leave its pixels alone until the key state changes. */
static int ball_overlaps(const Sprite *s,int x,int y,int w,int h)
{
    return x<s->x+s->w && x+w>s->x &&
           y<s->y+s->h && y+h>s->y;
}

static int rect_overlaps(int x,int y,int w,int h,
                         int x2,int y2,int w2,int h2)
{
    return x<x2+w2 && x+w>x2 && y<y2+h2 && y+h>y2;
}

/* Original tiny UI font, 3x5. Table art and moving sprites come from the pack. */
static const unsigned short font[]={
    31599,11415,29671,29647,23497,31183,31215,29257,31727,31695,
    23533,27566,31015,27502,31207,31204,31087,23533,29847,4719,
    23469,18727,24557,24557,31599,31716,31609,31724,31183,29842,
    23407,23402,23421,23213,23186,29351
};

/* The original Windows layout has a decorated title box, then separate
 * ball/score and message boxes. At 320x200 there are only 80 right-hand
 * pixels; all panel graphics are dirty-rectangle redraws, not a screen
 * refresh. The title is source-derived from splash_bitmap.bmp. */
static void draw_title(void)
{
    int y,x;
    for(y=0;y<MENU_ART_H;y++) {
        unsigned a=address(242,4+y);
        for(x=0;x<MENU_ART_W/2;x++)
            video[a+x]=menu_art[y*(MENU_ART_W/2)+x];
    }
    for(x=241;x<=318;x++) {
        pixel(x,2,7); pixel(x,58,7);
    }
    for(y=2;y<=58;y++) {
        pixel(240,y,8); pixel(319,y,7);
    }
}

static void text_color(int x,int y,const char *s,unsigned color)
{
    int i,j,index;
    unsigned bits;
    while(*s) {
        index=(*s>='0' && *s<='9')?*s-'0':
              ((*s>='A' && *s<='Z')?*s-'A'+10:-1);
        bits=index<0?0:font[index];
        for(j=0;j<5;j++) for(i=0;i<3;i++)
            if(bits&(1U<<(14-j*3-i))) pixel(x+i,y+j,color);
        x+=4; s++;
    }
}

static void panel_frame(int x,int y,int w,int h)
{
    int i;
    for(i=x;i<x+w;i++) { pixel(i,y,8); pixel(i,y+h-1,7); }
    for(i=y;i<y+h;i++) { pixel(x,i,8); pixel(x+w-1,i,7); }
}

/* view: 0 = score, 1 = Game/Options menu, 2 = controls.
 * The original top menu actions that are implemented by this DOS port are
 * keyboard-operable; unsupported music, players and high scores are not
 * falsely advertised as functional. */
static void draw_panel(int view,int choice,int paused)
{
    static const char *items[]={"NEW GAME","PAUSE","SOUND","CONTROLS","EXIT"};
    int i,y;
    char buf[24];
    restore(240,61,80,139);
    if(view==1) {
        panel_frame(241,63,78,110);
        text_color(245,67,"GAME OPT HELP",13);
        text_color(245,79,"MENU",14);
        for(i=0;i<5;i++) {
            y=92+i*15;
            if(i==choice) {
                pixel(244,y+2,14); pixel(245,y+2,14);
            }
            if(i==1) text_color(251,y,paused?"RESUME":"PAUSE",
                                 i==choice?14:15);
            else if(i==2) text_color(251,y,sound_enabled?
                                     "SOUND ON":"SOUND OFF",i==choice?14:15);
            else text_color(251,y,items[i],i==choice?14:15);
        }
        text_color(245,177,"UP DN ENTER",7);
        text_color(245,188,"ESC BACK",7);
    } else if(view==2) {
        panel_frame(241,63,78,129);
        text_color(245,68,"CONTROLS",13);
        text_color(245,84,"Z LEFT FLIP",15);
        text_color(245,96,"/ RIGHT FLIP",15);
        text_color(245,108,"SPACE PLUNGER",15);
        text_color(245,120,"A D SIDE BUMP",15);
        text_color(245,132,"X UP BUMP",15);
        text_color(245,144,"S SOUND F2 NEW",15);
        text_color(245,156,"F3 PAUSE",15);
        text_color(245,177,"ENTER BACK",7);
    } else {
        panel_frame(241,63,78,30);
        text_color(245,67,"BALL",13);
        sprintf(buf,"%d",4-game.balls);
        text_color(269,67,buf,15);
        text_color(245,78,"SCORE",13);
        sprintf(buf,"%010lu",game.score);
        text_color(245,86,buf,15);
        panel_frame(241,97,78,41);
        text_color(245,101,"STATUS",13);
        text_color(245,113,paused?"PAUSED":game.over?"GAME OVER":
                      game.tilted?"TILT":game.tilt_warning?"WARNING":
                      game.kick_delay?"KICKING":game.waiting?"READY":"IN PLAY",
                      game.tilted?12:15);
        if(!paused && !game.over && !game.waiting) {
            sprintf(buf,"KICK %c %c",game.kick_armed[0]?'L':'-',
                                  game.kick_armed[1]?'R':'-');
            text_color(245,126,buf,7);
        } else
            text_color(245,126,paused?"F3 RESUME":game.over?"N NEW GAME":
                      "SPACE LAUNCH",7);
        panel_frame(241,142,78,51);
        text_color(245,147,"M MENU F3 PAUSE",15);
        text_color(245,158,sound_enabled?"S SOUND ON":"S SOUND OFF",15);
        text_color(245,170,"Z / FLIPPERS",15);
        text_color(245,182,"ESC EXIT",7);
    }
}

static void silence(void)
{
    if(!sound_enabled) return;
    outp(0xc0,0x9f); outp(0xc0,0xbf); outp(0xc0,0xdf); outp(0xc0,0xff);
}

static void play_event(int event)
{
    unsigned divisor;
    if(!sound_enabled) return;
    if(!event) { if(sound_ticks && !--sound_ticks) silence(); return; }
    divisor=event==1?180:event==2?320:event==4?230:
            event==5?760:600;
    outp(0xc0,0x80|(divisor&15)); outp(0xc0,divisor>>4); outp(0xc0,0x94);
    sound_ticks=2;
}

static int load_background(void)
{
    FILE *f=fopen("TABLE.TGA","rb");
    unsigned i;
    unsigned char buf[512];
    int ok=1;
    if(!f) return 0;
    for(i=0;i<64;i++) {
        if(fread(buf,1,512,f)!=512) { ok=0; break; }
        _fmemcpy(background+i*512U,buf,512);
    }
    if(fgetc(f)!=EOF) ok=0;
    fclose(f); return ok;
}

static void dump_screen(void)
{
    FILE *f=fopen("SCREEN.BIN","wb");
    unsigned i;
    unsigned char buf[512];
    if(!f) return;
    for(i=0;i<64;i++) { _fmemcpy(buf,video+i*512U,512); fwrite(buf,1,512,f); }
    fclose(f);
}

int main(int argc,char **argv)
{
    unsigned last,now,frame=0,input,previous_input=0;
    unsigned long old_score=~0UL,slow_frames=0;
    int i,smoke=0,record=0,old_x=-10,old_y=-10,old_balls=-1,old_wait=-1,old_over=-1;
    int old_new=0,event,next_x,next_y,ball_changed,ball_redraw;
    int plunger_x,plunger_y,old_plunger_x=-1,old_plunger_y=-1;
    int plunger_changed,plunger_redraw,plunger_charge;
    int plunger_frame,old_plunger_frame=-1,old_plunger_w=0,old_plunger_h=0;
    const Sprite *plunger_sprite;
    int view=0,choice=0,paused=0,exit_requested=0,ui_dirty=0;
    int old_flags=-1;
    unsigned char toggle_sound,toggle_menu,key_up,key_down;
    unsigned char key_enter,key_esc,key_f1,key_f2,key_f3,key_new;
    unsigned observed=0,launches=0,full_launches=0;
    union REGS r;
    unsigned char old_mode;
    FILE *report;
    for(i=1;i<argc;i++) {
        if(!strcmp(argv[i],"/smoke")) smoke=1;
        else if(!strcmp(argv[i],"/record")) record=1;
        else if(!strcmp(argv[i],"/nosound")) sound_enabled=0;
        else if(!strcmp(argv[i],"/noshake")) shake_enabled=0;
        else { puts("Usage: CADET [/smoke] [/record] [/nosound] [/noshake]"); return 1; }
    }
    if(sizeof(short)!=2 || sizeof(long)!=4 || sizeof(Edge)!=18) return 1;
    if(!table_load(&table,"TABLE.DAT")) {
        puts("Missing or invalid TABLE.DAT. Run convert.py or make_demo.py first."); return 1;
    }
    background=_fmalloc(32768UL);
    if(!background) { puts("Not enough conventional memory (32KB background)."); return 1; }
    if(!load_background()) { _ffree(background); puts("Missing/invalid TABLE.TGA (32768 bytes)."); return 1; }
    game_reset(&game,&table);
    if(!smoke) game.plunger_rng^=ticks(); /* Reproducible smoke, varied interactive play. */
    memset(&r,0,sizeof(r)); r.h.ah=15; int86(0x10,&r,&r); old_mode=r.h.al;
    mode(9);
    video=(unsigned char __far *)MK_FP(0xb800,0);
    _fmemcpy(video,background,32768U);
    draw_title();
    draw_panel(view,choice,paused);
    silence();
    old_keyboard=_dos_getvect(9);
    _disable(); _dos_setvect(9,keyboard); _enable();
    last=ticks();
    while(!exit_requested && (!smoke || frame<360)) {
        now=ticks();
        if(now==last) continue;
        if((unsigned)(now-last)>1) slow_frames++;
        last=now; frame++;
        /* Latch quick taps that start and end between 18.2 Hz frames. */
        _disable();
        input=((keys[44]||pressed_keys[44])?KEY_LEFT:0)|
              ((keys[53]||pressed_keys[53])?KEY_RIGHT:0)|
              ((keys[57]||pressed_keys[57])?KEY_LAUNCH:0)|
              ((keys[45]||pressed_keys[45])?KEY_NUDGE:0)|
              ((keys[30]||pressed_keys[30])?KEY_NUDGE_LEFT:0)|
              ((keys[32]||pressed_keys[32])?KEY_NUDGE_RIGHT:0);
        pressed_keys[44]=pressed_keys[53]=pressed_keys[57]=pressed_keys[45]=0;
        pressed_keys[30]=pressed_keys[32]=0;
        toggle_sound=pressed_keys[31]; /* S */
        toggle_menu=pressed_keys[50]; /* M */
        key_up=pressed_keys[72]; key_down=pressed_keys[80];
        key_enter=pressed_keys[28]; key_esc=pressed_keys[1];
        key_f1=pressed_keys[59]; key_f2=pressed_keys[60];
        key_f3=pressed_keys[61]; key_new=pressed_keys[49];
        pressed_keys[31]=pressed_keys[50]=pressed_keys[72]=0;
        pressed_keys[80]=pressed_keys[28]=pressed_keys[1]=0;
        pressed_keys[59]=pressed_keys[60]=pressed_keys[61]=0;
        pressed_keys[49]=0;
        _enable();
        ui_dirty=0;
        if(smoke) {
            /* Hold until fully charged, then release; fixed short pulses
             * never reached the 96-substep pullback and hid failed shots.
             * Match the deterministic full-shot regression's flipper phase. */
            input=((frame-1)%20<10?KEY_LEFT:KEY_RIGHT);
            if(game.waiting && game.charge<PLUNGER_MAX_CHARGE)
                input|=KEY_LAUNCH;
        }
        observed|=input;
        if(toggle_sound) {
            /* Mute before disabling the PSG, so a held note cannot stick. */
            if(sound_enabled) { silence(); sound_ticks=0; sound_enabled=0; }
            else { sound_enabled=1; sound_ticks=0; }
            ui_dirty=1;
        }
        if(toggle_menu) {
            view=view?0:1;
            ui_dirty=1;
        }
        if(key_f1) { view=2; ui_dirty=1; }
        if(key_esc) {
            if(view) { view=0; ui_dirty=1; }
            else exit_requested=1;
        }
        if(view==1) {
            if(key_up) { choice=(choice+4)%5; ui_dirty=1; }
            if(key_down) { choice=(choice+1)%5; ui_dirty=1; }
            if(key_enter) {
                ui_dirty=1;
                if(choice==0) key_new=1;
                else if(choice==1) { paused=!paused; view=0; }
                else if(choice==2) {
                    if(sound_enabled) { silence(); sound_ticks=0; sound_enabled=0; }
                    else { sound_enabled=1; sound_ticks=0; }
                } else if(choice==3) view=2;
                else exit_requested=1;
            }
        } else if(view==2 && key_enter) {
            view=1; ui_dirty=1;
        }
        if(key_f3) { paused=!paused; ui_dirty=1; }
        if(key_f2 || key_new) {
            game_reset(&game,&table);
            game.plunger_rng^=ticks();
            paused=0; view=0; ui_dirty=1;
        }
        old_new=keys[49];
        event=0;
        /* Keeping physics frozen is important for a faithful Game menu
         * and for testing on a slow V20. No catch-up on resume. */
        if(!view && !paused && !exit_requested) {
            for(i=0;i<4;i++) {
                game_step(&game,&table,input);
                if(game.event) {
                    event=game.event;
                    if(event==2) {
                        launches++;
                        if(game.charge==PLUNGER_MAX_CHARGE) full_launches++;
                    }
                }
            }
        } else input=0;
        /* Match the unpowered resting collision surface during tilt. */
        if(game.tilted) input&=~(KEY_LEFT|KEY_RIGHT);
        play_event(event);
        if(record) {
            FlightTrace *sample=&flight[flight_next];
            sample->frame=frame;
            sample->x=game.x; sample->y=game.y;
            sample->vx=game.vx; sample->vy=game.vy;
            sample->input=(unsigned char)input;
            sample->flips[0]=game.flipper_contacts[0];
            sample->flips[1]=game.flipper_contacts[1];
            sample->powered[0]=game.flipper_power_contacts[0];
            sample->powered[1]=game.flipper_power_contacts[1];
            sample->balls=(unsigned char)game.balls;
            sample->waiting=(unsigned char)game.waiting;
            sample->in_shooter=game.in_shooter;
            sample->charge=(unsigned char)game.charge;
            sample->event=(unsigned char)event;
            sample->border=game.boundary_hits;
            flight_next=(flight_next+1)&255;
            if(flight_used<256) flight_used++;
        }
        /* Visible shake comes from the CRTC, never a memory-wide screen
         * redraw. The offset alternates briefly and resets on pause/menu. */
        camera_shift(!view && !paused && !exit_requested &&
                     ((game.shake_frames/4)&1)?1:0);
        plunger_charge=game.waiting?game.charge:0;
        plunger_frame=plunger_charge*7/PLUNGER_MAX_CHARGE;
        plunger_sprite=&table.plunger[plunger_frame];
        plunger_x=plunger_sprite->x; plunger_y=plunger_sprite->y;
        plunger_changed=(plunger_frame!=old_plunger_frame);
        if(plunger_changed && old_plunger_y>=0)
            restore(old_plunger_x,old_plunger_y,old_plunger_w,old_plunger_h);
        next_x=game.x/FP-table.sprites[4].w/2-1;
        next_y=game.y/FP-table.sprites[4].h/2-1;
        /* Positions are Q6 but only whole screen pixels are displayed.
         * Do not erase/repaint a ball that has not moved onscreen.
         * This also prevents an 18 Hz erase/repaint flash at the plunger. */
        ball_changed=(next_x!=old_x || next_y!=old_y || old_over!=game.over);
        ball_redraw=ball_changed;
        if(plunger_changed &&
           (rect_overlaps(next_x,next_y,
                          table.sprites[4].w+2,table.sprites[4].h+2,
                          plunger_x,plunger_y,plunger_sprite->w,plunger_sprite->h) ||
            (old_plunger_y>=0 &&
             rect_overlaps(next_x,next_y,
                           table.sprites[4].w+2,table.sprites[4].h+2,
                           old_plunger_x,old_plunger_y,old_plunger_w,old_plunger_h))))
            ball_redraw=1;
        if(ball_changed)
            restore(old_x,old_y,table.sprites[4].w+2,table.sprites[4].h+2);
        for(i=0;i<2;i++) {
            const Sprite *s;
            int changed=(input^previous_input)&(1<<i);
            if(changed) {
                s=&table.sprites[i*2]; restore(s->x-1,s->y-1,s->w+2,s->h+2);
                s++; restore(s->x-1,s->y-1,s->w+2,s->h+2);
            }
            s=&table.sprites[i*2+((input&(1<<i))!=0)];
            if(frame==1 || changed ||
               (ball_changed && ball_overlaps(s,old_x,old_y,
                            table.sprites[4].w+2,table.sprites[4].h+2)))
                sprite(s,s->x,s->y);
            /* Changing a flipper can erase a stationary ball that overlaps
             * it. Repaint the ball once after the flipper when necessary. */
            if(changed &&
               (ball_overlaps(&table.sprites[i*2],next_x,next_y,
                              table.sprites[4].w+2,table.sprites[4].h+2) ||
                ball_overlaps(&table.sprites[i*2+1],next_x,next_y,
                              table.sprites[4].w+2,table.sprites[4].h+2)))
                ball_redraw=1;
        }
        /* The old ball restore may erase the stationary plunger; only
         * repaint if their rectangles actually overlap. Paint ball last. */
        plunger_redraw=plunger_changed ||
           (ball_changed &&
            rect_overlaps(old_x,old_y,
                          table.sprites[4].w+2,table.sprites[4].h+2,
                          plunger_x,plunger_y,plunger_sprite->w,plunger_sprite->h));
        if(plunger_redraw) sprite(plunger_sprite,plunger_x,plunger_y);
        old_plunger_x=plunger_x; old_plunger_y=plunger_y;
        old_plunger_frame=plunger_frame;
        old_plunger_w=plunger_sprite->w; old_plunger_h=plunger_sprite->h;
        old_x=next_x; old_y=next_y;
        if(!game.over && ball_redraw)
            sprite(&table.sprites[4],old_x+1,old_y+1);
        {
            int flags=(game.tilted?1:0)|(game.tilt_warning?2:0)|
                      (game.kick_delay?4:0)|
                      (game.kick_armed[0]?8:0)|
                      (game.kick_armed[1]?16:0);
            if(ui_dirty || old_score!=game.score || old_balls!=game.balls ||
               old_wait!=game.waiting || old_over!=game.over ||
               old_flags!=flags) {
                draw_panel(view,choice,paused);
                old_flags=flags;
            }
        }
        old_score=game.score; old_balls=game.balls;
        old_wait=game.waiting; old_over=game.over;
        previous_input=input;
    }
    _disable(); _dos_setvect(9,old_keyboard); _enable();
    camera_shift(0);
    silence();
    if(smoke || record) dump_screen();
    mode(old_mode); _ffree(background);
    if(smoke || record) {
        report=fopen(smoke?"SMOKE.TXT":"INPUT.TXT","w");
        if(!report) return 1;
        fprintf(report,"PASS frames=%u steps=%lu score=%lu hits=%lu balls=%d slow_frames=%lu input=%u launches=%u nudges=%u kickbacks=%u tilts=%u full_launches=%u boundary_hits=%u left_hits=%u right_hits=%u left_power=%u right_power=%u\n",
                frame,game.steps,game.score,game.hits,game.balls,slow_frames,
                observed,launches,game.nudges,game.kickbacks,game.tilts,
                full_launches,game.boundary_hits,
                game.flipper_contacts[0],game.flipper_contacts[1],
                game.flipper_power_contacts[0],game.flipper_power_contacts[1]);
        fclose(report);
    }
    if(record) {
        report=fopen("FLIGHT.TXT","w");
        if(report) {
            unsigned j;
            fprintf(report,"last %u rendered frames (~14 seconds maximum); positions and velocities Q6\n",flight_used);
            fprintf(report,"FRAME XQ6 YQ6 VXQ6 VYQ6 BALLS READY TUBE EVENT CHARGE BORDER INPUT LEFT_HIT RIGHT_HIT LEFT_POWER RIGHT_POWER\n");
            for(j=0;j<flight_used;j++) {
                unsigned ix=(flight_next+256-flight_used+j)&255;
                const FlightTrace *sample=&flight[ix];
                fprintf(report,"%u %d %d %d %d %u %u %u %u %u %u %u %u %u %u %u\n",
                        sample->frame,sample->x,sample->y,
                        sample->vx,sample->vy,
                        sample->balls,sample->waiting,sample->in_shooter,
                        sample->event,sample->charge,sample->border,
                        sample->input,sample->flips[0],sample->flips[1],
                        sample->powered[0],sample->powered[1]);
            }
            fclose(report);
        }
    }
    puts("Space Cadet DOS prototype. Original assets; simplified physics/rules.");
    return 0;
}

/* Native 8086 real-mode DOS / Tandy 1000 backend. */
#include "game.h"
#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Table table;
static Game game;
static unsigned char __far *background;
static unsigned char __far *video;
static volatile unsigned char keys[128];
static volatile unsigned char pressed_keys[128];
static void (__interrupt __far *old_keyboard)(void);
static int sound_enabled=1, sound_ticks;

static void __interrupt __far keyboard(void)
{
    unsigned scan=inp(0x60), acknowledge=inp(0x61);
    outp(0x61,acknowledge|0x80); outp(0x61,acknowledge);
    /* XT/V20 make/break scancodes; extended prefixes are ignored. */
    if(scan!=0xe0 && scan!=0xe1) {
        keys[scan&127]=(scan&128)?0:1;
        if(!(scan&128)) pressed_keys[scan&127]=1;
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

/* Original tiny UI font, 3x5. Table art and moving sprites come from the pack. */
static const unsigned short font[]={
    31599,11415,29671,29647,23497,31183,31215,29257,31727,31695,
    23533,27566,31015,27502,31207,31204,31087,23533,29847,4719,
    23469,18727,24557,24557,31599,31716,31609,31724,31183,29842,
    23407,23402,23421,23213,23186,29351
};

static void text(int x,int y,const char *s)
{
    int i,j,index;
    unsigned bits;
    while(*s) {
        index=(*s>='0' && *s<='9')?*s-'0':
              ((*s>='A' && *s<='Z')?*s-'A'+10:-1);
        bits=index<0?0:font[index];
        for(j=0;j<5;j++) for(i=0;i<3;i++)
            pixel(x+i,y+j,(bits&(1U<<(14-j*3-i)))?15:0);
        x+=4; s++;
    }
}

static void hud(void)
{
    char buf[24];
    text(240,8,"SPACE CADET"); text(240,18,"DOS PROTOTYPE");
    text(240,36,"SCORE"); sprintf(buf,"%010lu",game.score); text(240,44,buf);
    sprintf(buf,"BALLS %d",game.balls); text(240,60,buf);
    text(240,80,"Z LEFT"); text(240,88,"SLASH RIGHT");
    text(240,100,"SPACE LAUNCH"); text(240,108,"HOLD RELEASE");
    text(240,124,"N NEW GAME"); text(240,132,"ESC QUIT");
    text(240,140,"X NUDGE");
    text(240,156,game.over?"GAME OVER   ":game.waiting?"READY       ":"PLAY        ");
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
    divisor=event==1?180:event==2?320:600;
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
    int old_new=0,event;
    unsigned observed=0,launches=0;
    union REGS r;
    unsigned char old_mode;
    FILE *report;
    for(i=1;i<argc;i++) {
        if(!strcmp(argv[i],"/smoke")) smoke=1;
        else if(!strcmp(argv[i],"/record")) record=1;
        else if(!strcmp(argv[i],"/nosound")) sound_enabled=0;
        else { puts("Usage: CADET [/smoke] [/record] [/nosound]"); return 1; }
    }
    if(sizeof(short)!=2 || sizeof(long)!=4 || sizeof(Edge)!=18) return 1;
    if(!table_load(&table,"TABLE.DAT")) {
        puts("Missing or invalid TABLE.DAT. Run convert.py or make_demo.py first."); return 1;
    }
    background=_fmalloc(32768UL);
    if(!background) { puts("Not enough conventional memory (32KB background)."); return 1; }
    if(!load_background()) { _ffree(background); puts("Missing/invalid TABLE.TGA (32768 bytes)."); return 1; }
    game_reset(&game,&table);
    memset(&r,0,sizeof(r)); r.h.ah=15; int86(0x10,&r,&r); old_mode=r.h.al;
    mode(9);
    video=(unsigned char __far *)MK_FP(0xb800,0);
    _fmemcpy(video,background,32768U);
    silence();
    old_keyboard=_dos_getvect(9);
    _disable(); _dos_setvect(9,keyboard); _enable();
    last=ticks();
    while(!keys[1] && !pressed_keys[1] && (!smoke || frame<360)) {
        now=ticks();
        if(now==last) continue;
        if((unsigned)(now-last)>1) slow_frames++;
        last=now; frame++;
        /* Latch quick taps that start and end between 18.2 Hz frames. */
        _disable();
        input=((keys[44]||pressed_keys[44])?KEY_LEFT:0)|
              ((keys[53]||pressed_keys[53])?KEY_RIGHT:0)|
              ((keys[57]||pressed_keys[57])?KEY_LAUNCH:0)|
              ((keys[45]||pressed_keys[45])?KEY_NUDGE:0);
        pressed_keys[44]=pressed_keys[53]=pressed_keys[57]=pressed_keys[45]=0;
        _enable();
        if(smoke) input=(frame%48<16?KEY_LAUNCH:0)|
                         (frame%20<10?KEY_LEFT:KEY_RIGHT)|(frame%90==0?KEY_NUDGE:0);
        observed|=input;
        if((keys[49] || pressed_keys[49]) && !old_new) game_reset(&game,&table);
        pressed_keys[49]=0;
        old_new=keys[49];
        event=0;
        for(i=0;i<4;i++) {
            game_step(&game,&table,input);
            if(game.event) { event=game.event; if(event==2) launches++; }
        }
        play_event(event);
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
               ball_overlaps(s,old_x,old_y,
                             table.sprites[4].w+2,table.sprites[4].h+2))
                sprite(s,s->x,s->y);
        }
        old_x=game.x/FP-table.sprites[4].w/2-1;
        old_y=game.y/FP-table.sprites[4].h/2-1;
        if(!game.over) sprite(&table.sprites[4],old_x+1,old_y+1);
        if(old_score!=game.score || old_balls!=game.balls || old_wait!=game.waiting || old_over!=game.over) {
            hud(); old_score=game.score; old_balls=game.balls; old_wait=game.waiting; old_over=game.over;
        }
        previous_input=input;
    }
    _disable(); _dos_setvect(9,old_keyboard); _enable();
    silence();
    if(smoke || record) dump_screen();
    mode(old_mode); _ffree(background);
    if(smoke || record) {
        report=fopen(smoke?"SMOKE.TXT":"INPUT.TXT","w");
        if(!report) return 1;
        fprintf(report,"PASS frames=%u steps=%lu score=%lu hits=%lu balls=%d slow_frames=%lu input=%u launches=%u\n",
                frame,game.steps,game.score,game.hits,game.balls,slow_frames,observed,launches);
        fclose(report);
    }
    puts("Space Cadet DOS prototype. Original assets; simplified physics/rules.");
    return 0;
}

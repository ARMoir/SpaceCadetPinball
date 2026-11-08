/* SOURCE MAP: HUD and menus
 * ORIGINAL: TTextBox.cpp; winmain.cpp
 * STATUS: ADAPTED
 * MATCHES: score, balls, game status, five high-score entries
 * DIVERGES: compact UI/font, reduced single-message TTextBox queue
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* One TTextBox-inspired event at a time; displayed for ~1.4s at 18.2Hz.
 * No redraw of static playfield and no physics code in this module. */
static int hud_event=0,hud_event_frames=0;
static char score_name[4]={'A','A','A',0};
static unsigned char score_name_length=0;
static int score_save_failed=0;

static const char *hud_event_text(int event)
{
    switch(event) {
        case 1: return "BUMPER HIT";
        case 2: return "BALL LAUNCHED";
        case 3: return "BALL LOST";
        case 4: return "KICKBACK";
        case 5: return "TILT";
        default: return "IN PLAY";
    }
}

/* Original tiny UI font, 3x5. Table art and moving sprites come from the pack. */
static const unsigned short font[]={
    31599,11415,29671,29647,23497,31183,31215,29257,31727,31695,
    11245,27566,31015,27502,31207,31204,31087,23533,29847,4719,
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
        if(vga_mode) {
            unsigned a=(unsigned)(y+4)*320U+242U;
            for(x=0;x<MENU_ART_W/2;x++) {
                unsigned char pair=menu_art[y*(MENU_ART_W/2)+x];
                video[a+2*x]=(unsigned char)(pair>>4);
                video[a+2*x+1]=(unsigned char)(pair&15);
            }
        } else {
            unsigned a=address(242,4+y);
            for(x=0;x<MENU_ART_W/2;x++)
                video[a+x]=menu_art[y*(MENU_ART_W/2)+x];
        }
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
        /* The 3x5 font has digits/letters; render the flipper key /
         * explicitly so F1 does not silently omit it. */
        bits=*s=='/'?4772U:(index<0?0:font[index]);
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

/* view: 0=score, 1=menu, 2=controls, 3=top five, 4=initials input.
 * Compact DOS UI substitutes the original Windows dialogs. */
static void draw_panel(int view,int choice,int paused)
{
    static const char *items[]={"NEW GAME","PAUSE","SOUND","CONTROLS","HIGH SCORES","EXIT"};
    int i,y;
    char buf[24];
    restore(240,61,80,139);
    if(view==1) {
        panel_frame(241,63,78,110);
        text_color(245,67,"GAME OPT HELP",13);
        text_color(245,79,"MENU",14);
        for(i=0;i<6;i++) {
            y=89+i*15;
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
        text_color(245,84,"Z OR LEFT CLICK",15);
        text_color(245,96,"/ OR RIGHT CLICK",15);
        text_color(245,108,"SPACE OR MID CLK",15);
        text_color(245,120,"A LEFT BUMP",15);
        text_color(245,132,"D RIGHT BUMP",15);
        text_color(245,144,"X UP BUMP",15);
        text_color(245,156,"F5 SOUND F2 NEW",15);
        text_color(245,168,"F3 PAUSE F8 HELP",15);
        text_color(245,180,"ENTER BACK",7);
    } else if(view==3) {
        panel_frame(241,63,78,129);
        text_color(245,68,"HIGH SCORES",13);
        for(i=0;i<SCORE_COUNT;i++) {
            sprintf(buf,"%d %s",i+1,scores_table[i].initials);
            text_color(245,83+i*17,buf,15);
            sprintf(buf,"%lu",scores_table[i].score);
            text_color(274,83+i*17,buf,7);
        }
        text_color(245,179,score_save_failed?"SAVE FAILED":"ENTER BACK",7);
    } else if(view==4) {
        panel_frame(241,63,78,129);
        text_color(245,68,"NEW HIGH SCORE",13);
        sprintf(buf,"%lu",game.score);
        text_color(245,86,buf,15);
        text_color(245,108,"ENTER INITIALS",14);
        text_color(270,123,score_name,15);
        text_color(245,150,"A-Z BACKSPACE",7);
        text_color(245,165,"ENTER SAVE",15);
        text_color(245,180,"ESC SKIP",7);
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
                      hud_event_frames?hud_event_text(hud_event):
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
        text_color(245,158,sound_enabled?"F5 SOUND ON":"F5 SOUND OFF",15);
        text_color(245,170,"Z / FLIPPERS",15);
        text_color(245,182,"ESC EXIT",7);
    }
}


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

/* A user-supplied Space Cadet title image, converted offline to the
 * original 16-color Tandy RGBI palette. MENU.DAT is optional so older
 * installations still work without another file. SCM1 + 240x200 packed
 * left-side menu background + 76x53 packed score-panel logo. Loading happens
 * before gameplay; drawing happens only on a view transition (F-007). */
#define MENU_BACKGROUND_BYTES 24000U
#define MENU_LOGO_BYTES 2014U
#define MENU_ASSET_BYTES (MENU_BACKGROUND_BYTES+MENU_LOGO_BYTES)
static unsigned char __far *menu_background=0;

static int load_menu_background(void)
{
    FILE *f=fopen("MENU.DAT","rb");
    unsigned char buf[512];
    unsigned offset,n;
    int good=1;
    if(!f) return 0;
    if(fread(buf,1,4,f)!=4 || memcmp(buf,"SCM1",4)) good=0;
    if(good) {
        menu_background=_fmalloc((unsigned long)MENU_ASSET_BYTES);
        if(!menu_background) good=0;
    }
    for(offset=0;good && offset<MENU_ASSET_BYTES;offset+=512U) {
        n=MENU_ASSET_BYTES-offset;
        if(n>512U) n=512U;
        if(fread(buf,1,n,f)!=n) { good=0; break; }
        _fmemcpy(menu_background+offset,buf,n);
    }
    if(good && fgetc(f)!=EOF) good=0;
    fclose(f);
    if(!good && menu_background) {
        _ffree(menu_background); menu_background=0;
    }
    return good;
}

static void draw_menu_background(void)
{
    int y,x;
    if(!menu_background) return;
    for(y=0;y<200;y++) {
        const unsigned char __far *src=menu_background+(unsigned)y*120U;
        if(vga_mode) {
            unsigned a=(unsigned)y*320U;
            for(x=0;x<120;x++) {
                unsigned char pair=src[x];
                video[a+2*x]=(unsigned char)(pair>>4);
                video[a+2*x+1]=(unsigned char)(pair&15);
            }
        } else {
            _fmemcpy(video+address(0,y),src,120U);
        }
    }
}

/* The original Windows layout has a decorated title box, then separate
 * ball/score and message boxes. Both DOS modes show the new sampled
 * title in the 76x53 right-hand box while gameplay stays unchanged. */
static void draw_title(void)
{
    int y,x;
    for(y=0;y<MENU_ART_H;y++) {
        if(vga_mode) {
            unsigned a=(unsigned)(y+4)*320U+242U;
            for(x=0;x<MENU_ART_W/2;x++) {
                unsigned char pair=menu_background?
                    menu_background[MENU_BACKGROUND_BYTES+
                                    (unsigned)y*(MENU_ART_W/2)+x]:
                    menu_art[y*(MENU_ART_W/2)+x];
                video[a+2*x]=(unsigned char)(pair>>4);
                video[a+2*x+1]=(unsigned char)(pair&15);
            }
        } else {
            unsigned a=address(242,4+y);
            for(x=0;x<MENU_ART_W/2;x++)
                video[a+x]=menu_background?
                    menu_background[MENU_BACKGROUND_BYTES+
                                    (unsigned)y*(MENU_ART_W/2)+x]:
                    menu_art[y*(MENU_ART_W/2)+x];
        }
    }
    /* Metallic 2px frame around source title art. The lower BALL
     * inset is drawn by draw_panel() when the ball count changes. */
    for(x=240;x<320;x++) {
        pixel(x,2,7); pixel(x,3,8); pixel(x,58,8);
    }
    for(y=2;y<=58;y++) {
        pixel(240,y,7); pixel(241,y,8);
        pixel(318,y,7); pixel(319,y,8);
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

/* F-007: source-inspired metallic box edges. Two pixels of cool gray
 * highlight/shadow fit the native 80px DOS sidebar in mode09 and mode13
 * without using a new palette, alpha blending or frame-time assets. */
static void panel_frame(int x,int y,int w,int ht)
{
    int i;
    for(i=x;i<x+w;i++) {
        pixel(i,y,7); pixel(i,y+1,8);
        pixel(i,y+ht-2,7); pixel(i,y+ht-1,8);
    }
    for(i=y;i<y+ht;i++) {
        pixel(x,i,7); pixel(x+1,i,8);
        pixel(x+w-2,i,7); pixel(x+w-1,i,8);
    }
}

/* Source FONT.DAT: ten original 15x22 score-digit bitmaps, converted
 * offline to 5x8 RGBI and packed at 2 bits/pixel (100 bytes total).
 * 0 transparent, 1 shadow, 2 lavender, 3 pale highlight.
 * Text still uses the narrower 3x5 alphabet so controls fit. */
static void score_digit(int x,int y,unsigned digit)
{
    static const unsigned char colors[4]={0,1,13,15};
    unsigned i;
    if(digit>9) return;
    for(i=0;i<40;i++) {
        unsigned char b=score_digits[digit][i>>2];
        unsigned c=(b>>(6U-2U*(i&3U)))&3U;
        if(c) pixel(x+(int)(i%5U),y+(int)(i/5U),colors[c]);
    }
}

static void score_number(int right,int y,const char *digits)
{
    unsigned count=(unsigned)strlen(digits);
    int step=(count<=8U)?6:5;
    int x;
    if(!count) return;
    /* Source digit has a shadow edging; leave a gap on normal scores.
     * Very large 9/10-digit scores use compact spacing to fit 52px. */
    x=right-((int)(count-1U)*step+5);
    while(*digits) {
        if(*digits>='0' && *digits<='9')
            score_digit(x,y,(unsigned)(*digits-'0'));
        x+=step;
        digits++;
    }
}

/* Source-styled lavender message with a deep-blue one-pixel shadow.
 * We intentionally keep 3x5 glyphs for 12..17-character instructions. */
static void source_message(int x,int y,const char *label)
{
    text_color(x+1,y+1,label,1);
    text_color(x,y,label,13);
}

/* view: 0=score; 1=menu; 2=controls; 3=top five; 4=initials.
 * The main HUD follows the owner's original Space Cadet screenshot:
 * title/ball, player/score, two-line command, large status box. */
static void draw_panel(int view,int choice,int paused)
{
    static const char *items[]={
        "NEW GAME","PAUSE","SOUND","CONTROLS","HIGH SCORES","EXIT"
    };
    int i,y;
    char buf[24];
    /* Keep the unchanged 76x53 title art; only refresh its lower band
     * and the panels when score/ball/state changes, never every frame. */
    restore(240,59,80,141);
    panel_frame(240,59,80,24);
    text_color(274,68,"BALL",12);
    panel_frame(302,63,15,17);
    sprintf(buf,"%u",(unsigned)(4-game.balls));
    score_number(312,68,buf);

    if(view==1) {
        panel_frame(240,86,80,111);
        source_message(245,91,"OPTIONS");
        for(i=0;i<6;i++) {
            y=105+i*14;
            if(i==choice) {
                pixel(244,y+2,14); pixel(245,y+2,14);
            }
            if(i==1) text_color(251,y,paused?"RESUME":"PAUSE",
                                i==choice?14:15);
            else if(i==2) text_color(251,y,sound_enabled?
                                     "SOUND ON":"SOUND OFF",i==choice?14:15);
            else text_color(251,y,items[i],i==choice?14:15);
        }
        text_color(245,190,"ESC BACK",7);
    } else if(view==2) {
        panel_frame(240,86,80,111);
        source_message(245,90,"CONTROLS");
        text_color(245,103,"Z OR LEFT CLICK",15);
        text_color(245,113,"/ OR RIGHT CLICK",15);
        text_color(245,123,"SPACE OR MID CLK",15);
        text_color(245,133,"A LEFT BUMP",15);
        text_color(245,143,"D RIGHT BUMP",15);
        text_color(245,153,"X UP BUMP",15);
        text_color(245,163,"F5 SOUND F2 NEW",15);
        text_color(245,173,"F3 PAUSE F8 HELP",15);
        text_color(245,186,"ENTER BACK",7);
    } else if(view==3) {
        panel_frame(240,86,80,111);
        source_message(245,90,"HIGH SCORES");
        for(i=0;i<SCORE_COUNT;i++) {
            sprintf(buf,"%d %s",i+1,scores_table[i].initials);
            text_color(245,105+i*15,buf,15);
            sprintf(buf,"%lu",scores_table[i].score);
            text_color(275,105+i*15,buf,7);
        }
        text_color(245,186,score_save_failed?"SAVE FAILED":"ENTER BACK",7);
    } else if(view==4) {
        panel_frame(240,86,80,111);
        source_message(245,90,"NEW HIGH SCORE");
        sprintf(buf,"%lu",game.score);
        text_color(245,103,buf,15);
        source_message(245,121,"ENTER INITIALS");
        text_color(269,133,score_name,15);
        text_color(245,156,"A-Z BACKSPACE",7);
        text_color(245,173,"ENTER SAVE",15);
        text_color(245,186,"ESC SKIP",7);
    } else {
        /* Compact source-style score box: player 1 and the actual score
         * (not ten padded zeros). A full 10-digit score still fits. */
        panel_frame(240,86,80,24);
        panel_frame(241,87,21,22);
        panel_frame(264,87,54,22);
        score_digit(248,94,1);
        sprintf(buf,"%lu",game.score);
        score_number(316,94,buf);

        panel_frame(240,113,80,30);
        if(paused) {
            source_message(245,118,"CONTINUE PLAY");
            source_message(245,128,"F3 RESUME");
        } else if(game.over) {
            source_message(245,118,"GAME OVER");
            source_message(245,128,"F2 NEW GAME");
        } else {
            source_message(245,118,"F2 STARTS NEW");
            source_message(245,128,"GAME");
        }

        panel_frame(240,146,80,52);
        source_message(245,152,"3D PINBALL");
        source_message(245,162,"SPACE CADET");
        if(hud_event_frames) {
            text_color(245,182,hud_event_text(hud_event),15);
        } else if(game.tilted) {
            text_color(245,182,"TILT",12);
        } else if(game.over) {
            text_color(245,182,"GAME OVER",15);
        } else if(game.waiting) {
            text_color(245,180,"AWAITING",7);
            text_color(245,188,"DEPLOYMENT",7);
        } else if(game.kick_delay) {
            text_color(245,182,"KICKBACK",15);
        } else {
            sprintf(buf,"KICK %c %c",game.kick_armed[0]?'L':'-',
                                    game.kick_armed[1]?'R':'-');
            text_color(245,184,buf,7);
        }
    }
}

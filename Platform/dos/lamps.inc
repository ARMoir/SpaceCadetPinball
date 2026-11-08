/* SOURCE MAP: Source-derived playfield lights
 * ORIGINAL: TLight.cpp; TLightGroup.cpp; control.cpp
 * STATUS: ADAPTED
 * MATCHES: timed flash/steady state and render-time bulb replacement
 * DIVERGES: event-driven 16-color dots, not full lamp sprites or missions
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-007: no bitmap allocations, file I/O or whole-table repaint.
 * Index 0..7 are source-derived bumpers, 8/9 kickback indicators,
 * 10/11 source-derived sling hulls and 12 shooter-ready indicator.
 * Only a lamp-state transition or old-ball overlap touches VRAM. */
#define LAMP_COUNT (MAX_BUMPERS+5)
static unsigned char lamp_ticks[LAMP_COUNT],lamp_drawn[LAMP_COUNT];
static unsigned short lamp_sling_seen[2],lamp_kick_seen;
static const unsigned char lamp_colors[LAMP_COUNT]={
    11,14,13,11,14,13,11,14,10,10,12,12,14
};

static void lamps_invalidate(void)
{
    int i;
    for(i=0;i<LAMP_COUNT;i++) lamp_drawn[i]=255;
}
static void lamps_reset(const Game *g)
{
    memset(lamp_ticks,0,sizeof(lamp_ticks));
    lamp_sling_seen[0]=g->sling_rescues[0];
    lamp_sling_seen[1]=g->sling_rescues[1];
    lamp_kick_seen=g->kickbacks;
    lamps_invalidate();
}

/* Timers are display-frame counts; callers freeze them on pause/menu. */
static void lamps_advance(const Game *g,unsigned hit_bumper,int active)
{
    int i;
    if(!active) return;
    if(hit_bumper>0 && hit_bumper<=MAX_BUMPERS)
        lamp_ticks[hit_bumper-1]=12;
    if(g->kickbacks!=lamp_kick_seen) {
        if(g->kick_side>=1 && g->kick_side<=2)
            lamp_ticks[MAX_BUMPERS+g->kick_side-1]=16;
        lamp_kick_seen=g->kickbacks;
    }
    for(i=0;i<2;i++) {
        if(g->sling_rescues[i]!=lamp_sling_seen[i]) {
            lamp_ticks[MAX_BUMPERS+2+i]=10;
            lamp_sling_seen[i]=g->sling_rescues[i];
        }
    }
    for(i=0;i<LAMP_COUNT;i++)
        if(lamp_ticks[i]) lamp_ticks[i]--;
}

static int lamp_position(const Table *t,int i,int *x,int *y)
{
    if(i<MAX_BUMPERS) {
        if(i>=t->bumper_count) return 0;
        *x=t->bumpers[i].x/FP;
        *y=t->bumpers[i].y/FP;
    } else if(i<MAX_BUMPERS+2) {
        int side=i-MAX_BUMPERS;
        *x=t->kickback[side*2];
        *y=t->kickback[side*2+1]-7;
    } else if(i<MAX_BUMPERS+4) {
        const Sling *h=&t->slings[i-MAX_BUMPERS-2];
        if(!h->count) return 0;
        *x=(h->left+h->right)/2;
        *y=h->top+9;
    } else {
        *x=t->launch_x+7; *y=t->launch_y-15;
    }
    return *x>=3 && *x<=236 && *y>=3 && *y<=196;
}

static unsigned lamp_wanted(const Game *g,int i,unsigned frame)
{
    if(g->over) return 0;
    if(i<MAX_BUMPERS)
        return lamp_ticks[i] && (lamp_ticks[i]&2U);
    if(i<MAX_BUMPERS+2) {
        int side=i-MAX_BUMPERS;
        if(g->kick_armed[side]) return 1;
        return lamp_ticks[i] && (lamp_ticks[i]&2U);
    }
    if(i<MAX_BUMPERS+4)
        return lamp_ticks[i] && (lamp_ticks[i]&2U);
    return g->waiting && ((frame&8U)!=0);
}

/* 5x5 compact on-state halo. Colors 0..15 are shared by Tandy mode09
 * and VGA mode13, including VGA1's reserved RGBI HUD entries. */
static void lamp_glow(int x,int y,unsigned color)
{
    pixel(x,y-2,color); pixel(x,y+2,color);
    pixel(x-2,y,color); pixel(x+2,y,color);
    pixel(x-1,y-1,color); pixel(x+1,y-1,color);
    pixel(x-1,y+1,color); pixel(x+1,y+1,color);
    pixel(x,y-1,15); pixel(x-1,y,15);
    pixel(x+1,y,15); pixel(x,y+1,15);
    pixel(x,y,15);
}

/* Return bit0 if lamp restoration covered the NEW ball, bit1 if it
 * covered the plunger. Caller repaints sprites AFTER lamp changes.
 * Lit bulbs erased by the OLD ball's dirty rectangle are repainted.
 * A 13-lamp integer loop at 18Hz is intentionally cheaper than copying
 * the table every frame on the 7.16MHz V20. */
static int lamps_render(const Table *t,const Game *g,unsigned frame,
                        int old_ball_x,int old_ball_y,
                        int new_ball_x,int new_ball_y,
                        int ball_w,int ball_h,int ball_changed,
                        int plunger_x,int plunger_y,
                        int plunger_w,int plunger_h)
{
    int i,x,y,changed,repaint,damage=0;
    unsigned wanted;
    for(i=0;i<LAMP_COUNT;i++) {
        if(!lamp_position(t,i,&x,&y)) continue;
        wanted=lamp_wanted(g,i,frame);
        changed=lamp_drawn[i]!=wanted;
        if(changed) {
            restore(x-2,y-2,5,5);
            if(rect_overlaps(x-2,y-2,5,5,
                             new_ball_x,new_ball_y,ball_w,ball_h))
                damage|=1;
            if(rect_overlaps(x-2,y-2,5,5,
                             plunger_x,plunger_y,plunger_w,plunger_h))
                damage|=2;
            lamp_drawn[i]=(unsigned char)wanted;
        }
        repaint=changed ||
            (ball_changed &&
             rect_overlaps(x-2,y-2,5,5,old_ball_x,old_ball_y,
                           ball_w,ball_h));
        if(wanted && repaint) lamp_glow(x,y,lamp_colors[i]);
    }
    return damage;
}

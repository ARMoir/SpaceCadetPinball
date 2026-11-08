/* SOURCE MAP: Ball feed/initial setup
 * ORIGINAL: TPinballTable.cpp; TFlipper.cpp; TPlunger.cpp
 * STATUS: ADAPTED
 * MATCHES: three-ball feed and source flipper endpoints
 * DIVERGES: fixed-point three-position flipper and reset conventions
 * EXCEPTIONS: F-003 F-006 F-008
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
static void make_edge(Edge *e,int x,int y,int ex,int ey)
{
    long dx=ex-x,dy=ey-y;
    unsigned long len=integer_sqrt((unsigned long)(dx*dx+dy*dy));
    memset(e,0,sizeof(*e));
    e->x=x; e->y=y; e->ex=ex; e->ey=ey;
    if(!len) len=1;
    e->tx=(short)(dx*256L/(long)len); e->ty=(short)(dy*256L/(long)len);
    e->length=(short)(len*FP);
}

static void clear_stuck(Game *g)
{
    /* Do not clear the three lifetime diagnostics on a normal ball feed. */
    g->stuck_ticks=g->stuck_stage_ticks=0;
    g->stuck_stage=g->stuck_progress=0;
}

static void feed(Game *g,const Table *t)
{
    g->x=t->launch_x*FP; g->y=t->launch_y*FP;
    g->vx=g->vy=g->charge=0; g->waiting=1;
    g->in_shooter=1;
    g->kick_delay=0; g->kick_side=0; g->kick_flight=0;
    clear_stuck(g);
    g->sweep[0]=g->sweep[1]=0;
}

void game_reset(Game *g,const Table *t)
{
    int i,j;
    memset(g,0,sizeof(*g)); g->balls=3;
    g->kick_armed[0]=g->kick_armed[1]=1;
    g->plunger_rng=0xa5c3U; /* BIOS ticks supply a session seed on DOS. */
    for(i=0;i<2;i++) {
        const short *p=t->flipper+i*6;
        for(j=0;j<2;j++)
            make_edge(&g->flips[i][j],p[0],p[1],p[2+j*2],p[3+j*2]);
        make_edge(&g->flips[i][2],p[0],p[1],
                  (p[2]+p[4])/2,(p[3]+p[5])/2);
        /* A short extra contact surface at the inner *raised* toe gives
         * the player a chance to reach near-center descents. It is never
         * drawn, does not alter source geometry or table files, and only
         * participates while that flipper is deliberately held up. */
        make_edge(&g->tip_assist[i],p[4],p[5],
                  p[4]+(i? -FLIPPER_TIP_ASSIST_PX:
                             FLIPPER_TIP_ASSIST_PX),p[5]);
    }
    feed(g,t);
}


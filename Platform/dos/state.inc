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

/* The Windows source 601 launch anchor projects to (181,141), but the
 * imported TPlunger REST sprite visibly starts at (184,156). In the
 * flattened DOS projection this leaves the READY ball floating 15px
 * above the mechanical plunger. Use the unmodified source sprite to
 * derive a seated center just above its visible tip. Synthetic/demo
 * tables without a plausible plunger keep their original launch anchor.
 * Both feed and re-feed use the same seat. Release now guides the ball
 * through just the lower tube to the collision-tested launch anchor,
 * with no instantaneous visible position snap. See F-003. */
static void shooter_seat(const Table *t,short *x,short *y)
{
    const Sprite *rod=&t->plunger[0];
    *x=t->launch_x; *y=t->launch_y;
    if(rod->w>=3 && rod->w<=8 && rod->h>=8 &&
       abs(rod->x-t->launch_x)<=8 &&
       rod->y>t->launch_y &&
       rod->y-t->launch_y<=20) {
        *x=(short)(rod->x+rod->w/2);
        *y=(short)(rod->y-3);
    }
}

static void feed(Game *g,const Table *t)
{
    short x,y;
    shooter_seat(t,&x,&y);
    g->sling_chain_side=g->sling_chain_hits=g->sling_chain_time=0;
    g->x=x*FP; g->y=y*FP;
    g->vx=g->vy=g->charge=0; g->waiting=1;
    g->in_shooter=1;
    g->shooter_seat_guide=0;
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


/* SOURCE MAP: Anti-stuck recovery
 * ORIGINAL: control.cpp::UnstuckBall; TBall.h
 * STATUS: ADAPTED
 * MATCHES: upward release, preserve ball count, exclude capture
 * DIVERGES: Q6 progress watchdog, safe local moves
 * EXCEPTIONS: F-008
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-008: source control::UnstuckBall first throws upward outside
 * flipper/plunger capture zones, then feeds a replacement ball if
 * attempts fail. Its StuckCounter bookkeeping depends on the richer
 * original 3D component/ball engine and is not in SCP2. The DOS
 * equivalent uses a rolling 3px positional envelope, not raw velocity:
 * a vibrating trapped ball may report substantial vx/vy but make no
 * actual progress. Every check is constant-time except a rare candidate
 * clearance check after prolonged stuck detection. */
static int stuck_allowed(const Game *g,const Table *t)
{
    int side,px=g->x/FP,py=g->y/FP;
    if(g->over || g->waiting || g->in_shooter || g->kick_delay)
        return 0;
    /* Do not turn a legitimate last-second drain into a free save. */
    if(py>=t->drain_y-10) return 0;
    /* Shooter/READY state already excludes real plunger capture above.
     * A broad coordinate box also excluded the spent RIGHT kicker at
     * (172,155), leaving it permanently immune to recovery (F-001/F-008). */
    for(side=0;side<2;side++) {
        const short *p=t->flipper+6*side;
        int left=minimum(p[0],minimum(p[2],p[4]))-5;
        int right=maximum(p[0],maximum(p[2],p[4]))+5;
        int top=minimum(p[1],minimum(p[3],p[5]))-5;
        int bottom=maximum(p[1],maximum(p[3],p[5]))+5;
        if(px>=left && px<=right && py>=top && py<=bottom)
            return 0;
    }
    return 1;
}

/* Rare F-008 rescue-only position validity. Prefer a local candidate
 * whose ball-radius disk does not intersect imported static walls,
 * radial bumpers, or either filled sling. No sqrt or dynamic memory,
 * and none of these checks are added to ordinary flight. */
static int stuck_clear_at(const Table *t,short x,short y)
{
    int i,k,px=x/FP,py=y/FP,b;
    const long rad=3L*FP,rad2=9L*FP*FP;
    if(px<5 || px>230 || py<5 || py>=t->drain_y-10)
        return 0;
    for(i=0;i<t->bumper_count;i++) {
        const Bumper *p=&t->bumpers[i];
        long dx=(long)x-p->x,dy=(long)y-p->y;
        if(dx>-p->radius && dx<p->radius &&
           dy>-p->radius && dy<p->radius &&
           dx*dx+dy*dy<(long)p->radius*p->radius) return 0;
    }
    for(i=0;i<MAX_SLINGS;i++) {
        const Sling *h=&t->slings[i];
        if(!h->count || px<h->left-3 || px>h->right+3 ||
           py<h->top-3 || py>h->bottom+3) continue;
        for(k=0;k<3;k++) {
            long dx=(long)x-(long)h->x[k]*FP;
            long dy=(long)y-(long)h->y[k]*FP;
            if((dx*h->nx[k]+dy*h->ny[k])/256>=rad) break;
        }
        if(k==3) return 0;
    }
    b=maximum(0,minimum(BANDS-1,py/8));
    for(i=0;i<t->band_count[b];i++) {
        unsigned index=t->band[b][i];
        const Edge *e=&t->edges[index];
        const EdgeBounds *box=&t->bounds[index];
        long dx,dy,along,perp;
        if(px<box->left || px>box->right ||
           py<box->top || py>box->bottom) continue;
        dx=(long)x-(long)e->x*FP;
        dy=(long)y-(long)e->y*FP;
        along=(dx*e->tx+dy*e->ty)/256;
        if(along<0) {
            if(dx*dx+dy*dy<rad2) return 0;
        } else if(along>e->length) {
            dx=(long)x-(long)e->ex*FP;
            dy=(long)y-(long)e->ey*FP;
            if(dx*dx+dy*dy<rad2) return 0;
        } else {
            perp=(-dx*e->ty+dy*e->tx)/256;
            if(perp>-rad && perp<rad) return 0;
        }
    }
    return 1;
}

static void stuck_move_to_clear(Game *g,const Table *t)
{
    /* First try a gentle 2..3px move toward the table's middle and up.
     * If blocked by a wall, try upward or the opposite side. Only
     * positions verified clear of source geometry can be selected. */
    static const signed char offsets[7][2]={
        {2,-2},{0,-3},{3,-3},{-2,-2},{0,-5},{-3,-2},{4,-4}
    };
    int i,sign=g->x<115*FP?1:-1;
    for(i=0;i<7;i++) {
        short x=(short)(g->x+(short)(sign*offsets[i][0])*FP);
        short y=(short)(g->y+(short)offsets[i][1]*FP);
        if(stuck_clear_at(t,x,y)) {
            g->x=x; g->y=y;
            g->vx=(short)(sign*24);
            g->vy=-56;
            g->unstick_moves++;
            return;
        }
    }
    /* No verified local opening: do not blindly embed the ball in
     * another wall. A later watchdog stage will relaunch it safely. */
}

static void watch_stuck(Game *g,const Table *t)
{
    if(!stuck_allowed(g,t)) { clear_stuck(g); return; }
    if(g->stuck_stage) {
        /* Must LEAVE the trapped area for several consecutive substeps,
         * not just jitter a few pixels and reset the rescue sequence. */
        if(abs((int)g->x-g->stuck_x)>=UNSTICK_ESCAPE_PX*FP ||
           abs((int)g->y-g->stuck_y)>=UNSTICK_ESCAPE_PX*FP) {
            if(++g->stuck_progress>=6) { clear_stuck(g); return; }
        } else g->stuck_progress=0;
        if(++g->stuck_stage_ticks<UNSTICK_RETRY_TICKS) return;
        g->stuck_stage_ticks=0;
        if(g->stuck_stage<3) {
            stuck_move_to_clear(g,t);
            g->stuck_stage++;
        } else {
            /* Original source also ends with a free plunger relaunch.
             * Do not decrement balls, discard score, or reset game rules. */
            g->unstick_relaunches++;
            feed(g,t);
        }
        return;
    }
    if(!g->stuck_ticks) {
        g->stuck_minx=g->stuck_maxx=g->x;
        g->stuck_miny=g->stuck_maxy=g->y;
    } else {
        g->stuck_minx=(short)minimum(g->stuck_minx,g->x);
        g->stuck_maxx=(short)maximum(g->stuck_maxx,g->x);
        g->stuck_miny=(short)minimum(g->stuck_miny,g->y);
        g->stuck_maxy=(short)maximum(g->stuck_maxy,g->y);
    }
    if(g->stuck_maxx-g->stuck_minx>UNSTICK_PROGRESS_PX*FP ||
       g->stuck_maxy-g->stuck_miny>UNSTICK_PROGRESS_PX*FP) {
        g->stuck_ticks=0; /* Genuine positional progress. */
        return;
    }
    if(++g->stuck_ticks>=UNSTICK_DETECT_TICKS) {
        int sign=g->x<115*FP?1:-1;
        g->stuck_x=g->x; g->stuck_y=g->y;
        g->stuck_stage=1; g->stuck_stage_ticks=0;
        g->stuck_progress=0;
        /* Source-inspired upward throw, softened for the Q6 DOS table.
         * Do not add a screen-wide teleport or a free score. */
        g->vx=(short)maximum(-160,minimum(160,g->vx+sign*20));
        if(g->vy>-48) g->vy=-48;
        g->unstick_nudges++;
    }
}


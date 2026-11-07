/* SOURCE MAP: Slingshot hull
 * ORIGINAL: TEdgeSegment.cpp; TEdgeManager.cpp
 * STATUS: ADAPTED
 * MATCHES: source-derived outlines and outward response
 * DIVERGES: solid triangle prevents center trapping
 * EXCEPTIONS: F-002
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-002: solid slingshot interior rejection, including the right
 * wedge where real Tandy gameplay repeatedly buried the ball. We use
 * the three Q8 outward half-plane normals of each original-derived
 * triangular core. If all three signed distances are below the ball
 * radius, the ball overlaps the solid. Project it to the nearest face
 * and reflect only incoming velocity. A shallow upward release on an
 * upper/side face avoids a zero-speed jitter trap, without arbitrary
 * random kicks or altering the drain. The source outline is excluded
 * from the generic two-sided wall index to prevent conflicting pushes.
 *
 * 2.75 px maximum gameplay movement per substep is below the 6px
 * collision diameter; a fast moving ball cannot step completely through
 * the expanded core without entering this test. */
static void contain_slingshots(Game *g,const Table *t)
{
    int i,k,best_nx,best_ny;
    long px=g->x,py=g->y;
    if(g->in_shooter) return;
    for(i=0;i<MAX_SLINGS;i++) {
        const Sling *h=&t->slings[i];
        long best=-32767L,d,rx,ry,dot,impulse,push;
        if(!h->count || px<(h->left-3L)*FP ||
           px>(h->right+3L)*FP ||
           py<(h->top-3L)*FP || py>(h->bottom+3L)*FP)
            continue;
        best_nx=best_ny=0;
        for(k=0;k<3;k++) {
            rx=px-(long)h->x[k]*FP;
            ry=py-(long)h->y[k]*FP;
            d=(rx*h->nx[k]+ry*h->ny[k])/256;
            /* Outside a face by one whole ball radius: not inside. */
            if(d>=3*FP) break;
            if(d>best) {
                best=d;
                best_nx=h->nx[k];
                best_ny=h->ny[k];
            }
        }
        if(k<3) continue;
        /* 1/4-pixel clearance avoids resettling on the exact edge.
         * The maximum inside correction is only a few screen pixels. */
        push=3L*FP-best+16;
        g->x+=(short)(push*best_nx/256);
        g->y+=(short)(push*best_ny/256);
        dot=((long)g->vx*best_nx+(long)g->vy*best_ny)/256;
        if(dot<0) {
            impulse=-(dot*7)/4;
            g->vx+=(short)(impulse*best_nx/256);
            g->vy+=(short)(impulse*best_ny/256);
        }
        if(best_ny<0 && g->vy>-24)
            g->vy=-24; /* tip/upper side: avoid sitting on the ridge */
        g->vx=(short)maximum(-160,minimum(160,g->vx));
        g->vy=(short)maximum(-176,minimum(176,g->vy));
        g->sling_rescues[i]++;
        px=g->x; py=g->y;
    }
}


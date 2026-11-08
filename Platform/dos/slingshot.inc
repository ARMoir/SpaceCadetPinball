/* SOURCE MAP: Slingshot hull
 * ORIGINAL: TEdgeSegment.cpp; TEdgeManager.cpp
 * STATUS: ADAPTED
 * MATCHES: source-derived outlines and outward response
 * DIVERGES: solid triangle prevents center trapping
 * EXCEPTIONS: F-002
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-008: the original UnstuckBall-inspired 3px position watchdog cannot
 * detect a ball that repeatedly travels from left to right slingshot and
 * back. Count actual incoming contacts with ALTERNATING hulls, not mere
 * near-sling positions or repeated overlap corrections. Four distinct,
 * alternating rebounds, each no more than ~2 seconds after the last,
 * get one
 * deterministic upward/inward impulse. A real volley can still happen:
 * up to three alternating hits are left completely alone.
 * No free balls, teleport, scoring, mission change or timer overhead.
 * See F-002 / F-008: this is a DOS flattened-layer playability guard,
 * not a source TLight or TEdgeSegment rule. */
#define SLING_LOOP_HITS 4
#define SLING_LOOP_WINDOW 144
static void sling_contact(Game *g,int side)
{
    unsigned char current=(unsigned char)(side+1);
    if(g->tilted || g->in_shooter || g->waiting) return;
    if(g->y<105*FP || g->y>165*FP) return;
    if(g->sling_chain_side==current) return; /* Same hull, no alternation. */
    if(g->sling_chain_hits && g->sling_chain_time)
        g->sling_chain_hits++;
    else
        g->sling_chain_hits=1;
    g->sling_chain_side=current;
    g->sling_chain_time=SLING_LOOP_WINDOW;
    if(g->sling_chain_hits<SLING_LOOP_HITS) return;
    /* Lift the ball toward the upper center while leaving its exact
     * source collision-resolved position intact. It is an impulse, NOT
     * a scripted/safe-corridor kickback, and ordinary walls stay active. */
    g->vx=(short)(side?-48:48);
    if(g->vy>-144) g->vy=-144;
    g->sling_loop_breaks++;
    g->sling_chain_side=g->sling_chain_hits=g->sling_chain_time=0;
}

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
            sling_contact(g,i);
        }
        if(best_ny<0 && g->vy>-24)
            g->vy=-24; /* tip/upper side: avoid sitting on the ridge */
        g->vx=(short)maximum(-160,minimum(160,g->vx));
        g->vy=(short)maximum(-176,minimum(176,g->vy));
        g->sling_rescues[i]++;
        px=g->x; py=g->y;
    }
}


/* SOURCE MAP: Outer rail/gate constraints
 * ORIGINAL: TEdgeManager.cpp; TOneway.cpp; TGate.cpp
 * STATUS: ADAPTED
 * MATCHES: source rail positions and one-way intent
 * DIVERGES: DOS-only rail guards, ramp crest assist, sling one-way apron and upper chute
 * EXCEPTIONS: F-002
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-002: source-projected outer-right rail escape guard. Imported edges
 * can be parallel and only two pixels apart, causing narrow capsule
 * corrections to eject a fast ball THROUGH the wall. The owner's flight
 * record showed x=235 at y=113 after passing the true rail near x=184.
 * Run this guard only near the long source rail, after ordinary static
 * and bumper collisions. It is one-sided: preserve interior play and
 * reflect only outward movement, without a random nudge or teleport.
 * A 2-pixel ball margin prevents the sprite overlapping the rail. */
static void contain_outer_right(Game *g,const Table *t)
{
    const Edge *e;
    long limit,dy,dot,impulse;
    int nx,ny,py=g->y/FP;
    if(g->in_shooter || t->outer_right_rail<0) return;
    e=&t->edges[t->outer_right_rail];
    if(py<minimum(e->y,e->ey) || py>maximum(e->y,e->ey) ||
       e->ey==e->y) return;
    /* F-002/V20: reject the inboard majority BEFORE 32-bit division. */
    if(g->x<=(minimum(e->x,e->ex)-2)*FP) return;
    dy=(long)g->y-(long)e->y*FP;
    limit=(long)e->x*FP+
          dy*(e->ex-e->x)/(e->ey-e->y)-2*FP;
    if((long)g->x<=limit) return;
    /* Project right-facing normal from the wall direction, Q8. */
    nx=abs(e->ty);
    ny=e->ty>=0?-e->tx:e->tx;
    dot=((long)g->vx*nx+(long)g->vy*ny)/256;
    if(dot>0) {
        impulse=dot*7/4;
        g->vx-=(short)(impulse*nx/256);
        g->vy-=(short)(impulse*ny/256);
    }
    g->x=(short)limit;
    g->right_rail_saves++;
}

/* F-002: matching source-derived LEFT outer wall. The single-plane
 * import can let a fast ball slip through the two-sided rail in the same
 * way previously observed on the right. Keep it inside the real playfield
 * without adding random motion or changing shooter/ramp geometry.
 * No work occurs outside the imported y interval; shooter remains exempt. */
static void contain_outer_left(Game *g,const Table *t)
{
    const Edge *e;
    long limit,dy,dot,impulse;
    int nx,ny,py=g->y/FP;
    if(g->in_shooter || t->outer_left_rail<0) return;
    e=&t->edges[t->outer_left_rail];
    if(py<minimum(e->y,e->ey) || py>maximum(e->y,e->ey) ||
       e->ey==e->y) return;
    /* The normal game costs only a small integer comparison; divide
     * to interpolate the rail slope only for a plausible outer escape. */
    if(g->x>=(maximum(e->x,e->ex)+2)*FP) return;
    dy=(long)g->y-(long)e->y*FP;
    limit=(long)e->x*FP+
          dy*(e->ex-e->x)/(e->ey-e->y)+2*FP;
    if((long)g->x>=limit) return;
    /* Left-facing exterior normal; reflect only outward travel. */
    nx=-abs(e->ty);
    ny=e->ty>=0?e->tx:-e->tx;
    dot=((long)g->vx*nx+(long)g->vy*ny)/256;
    if(dot>0) {
        impulse=dot*7/4;
        g->vx-=(short)(impulse*nx/256);
        g->vy-=(short)(impulse*ny/256);
    }
    g->x=(short)limit;
    g->left_rail_saves++;
}

/* F-002: original TRamp::FieldEffect provides speed/plane forces which
 * SCP2's flattened projection cannot reproduce. At the last ~10-15%
 * of the upper left/right ramp, a true upward crest crossing gets one
 * mild velocity floor. No repositioning or repeated boost; a strongly
 * moving ball keeps its existing speed. Use source ramp contour bounds,
 * and enable only on the original indexed source table shape. */
static void assist_ramp_crest(Game *g,const Table *t,short old_y)
{
    int px=g->x/FP,target_vy,target_vx;
    if(t->sling_count!=2 || t->outer_left_rail<0 ||
       t->outer_right_rail<0 || g->waiting || g->over ||
       g->in_shooter || g->kick_delay || g->kick_flight ||
       g->vy>=0 || g->vx>=-12) return;
    if(old_y<38*FP || g->y>=38*FP) return;
    if(px>=139 && px<=166) {
        target_vy=-96; target_vx=-40; /* right ramp (x133..166) */
    } else if(px>=62 && px<=83) {
        target_vy=-88; target_vx=-28; /* left ramp (x62..82) */
    } else return;
    if(g->vy>target_vy || g->vx>target_vx) {
        if(g->vy>target_vy) g->vy=(short)target_vy;
        if(g->vx>target_vx) g->vx=(short)target_vx;
        g->ramp_crest_assists++;
    }
}

/* F-002: one-way deflector at the two source-derived sling aprons.
 * Original TOneway relies on missing 3D collision-group metadata.
 * Stop flipper shots climbing OUTWARD into the backside return tubes;
 * descending balls, inward shots, the shooter and valid kickbacks
 * pass unchanged. The horizontal velocity is steered toward the open
 * playfield, NOT kicked harder or position-snapped through walls.
 * Check a narrow upward lip crossing, or immediately after a genuine
 * flipper collision above that lip (which can reverse velocity midstep).
 * The source sling hull bounds determine each deflector's footprint. */
static void guard_sling_backflow(Game *g,const Table *t,
                                short old_y,int from_flipper)
{
    int side,px=g->x/FP,py=g->y/FP;
    if(g->waiting || g->over || g->in_shooter ||
       g->kick_delay || g->kick_flight || g->vy>=-24) return;
    for(side=0;side<MAX_SLINGS;side++) {
        const Sling *h=&t->slings[side];
        int gate_y;
        if(!h->count) continue;
        gate_y=h->bottom+10;
        if(from_flipper) {
            if(py<h->bottom || py>h->bottom+17) continue;
        } else if(old_y<gate_y*FP || g->y>=gate_y*FP) continue;
        if(side==0) {
            if(px<h->left-10 || px>h->right+9 || g->vx>=-40)
                continue;
            g->vx=28;
        } else {
            if(px<h->left-9 || px>h->right+12 || g->vx<=40)
                continue;
            g->vx=-28;
        }
        g->sling_backflow_redirects++;
        break;
    }
}

/* The upper-left skill-shot chute includes one-way passages in the
 * original (TOneway / skill-shot gates). The current SCP2 pack flattened
 * adjacent ramp edges into two-sided walls. Let the ball pass downhill
 * through the *interior* upper-left throat, but keep the long external
 * boundaries and all ordinary playfield walls solid. This can be replaced
 * by imported one-way edges in the next data format. */
static int upper_chute_pass(const Game *g,const Edge *e,int px,int py)
{
    /* Skip only the short one-way throat adjacent to the upper-left
     * skill-shot gate. Broadly skipping this corner's walls makes the
     * launch lose its intended path and bumper scoring. */
    if(g->vy<=0 || px<90 || px>105 || py<26 || py>37)
        return 0;
    return minimum(e->x,e->ex)>=89 &&
           maximum(e->x,e->ex)<=106 &&
           minimum(e->y,e->ey)>=24 &&
           maximum(e->y,e->ey)<=37;
}


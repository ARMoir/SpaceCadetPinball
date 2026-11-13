/* SOURCE MAP: Outer rail/gate constraints
 * ORIGINAL: TEdgeManager.cpp; TOneway.cpp; TGate.cpp
 * STATUS: ADAPTED
 * MATCHES: source rail positions, imported TOneway directions and ramp paths
 * DIVERGES: ground-only lines, DOS crest policy, rail/apron/chute guards
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

/* F-002: TRamp 1300 triangles -> shared-edge midpoint path, 1301/1302
 * endpoints. This is a bounded forgiveness policy, NOT FieldEffect parity.
 * Require entry in first quarter, forward progress across 90%, rising path,
 * and at most 220 ticks in a 3px source group101 / 5px group105
 * ball-width corridor. No position or score changes. */
static void assist_ramp_crest(Game *g,const Table *t)
{
    int i,j,found=-1,forward=0;
    long best2=5L*FP*5L*FP+1L;
    short progress=0,tx=0,ty=0,side=0;
    if(g->ramp_exit_ticks) g->ramp_exit_ticks--;
    if(g->waiting || g->over || g->tilted || g->in_shooter ||
       g->kick_delay || g->kick_flight) {
        g->ramp_active=g->ramp_used=g->ramp_exit_ticks=0;
        g->ramp_crest_seen=0;
        g->ramp_ticks=0; return;
    }
    for(i=0;i<t->ramp_count;i++) {
        const RampPath *r=&t->ramps[i];
        if(g->x<r->left || g->x>r->right || g->y<r->top || g->y>r->bottom) continue;
        for(j=0;j<r->count;j++) {
            const RampSegment *e=&r->segments[j];
            long dx=(long)g->x-e->x,dy=(long)g->y-e->y;
            long along=(dx*e->tx+dy*e->ty)/256;
            long limit=(r->group==105?5L:3L)*FP;
            long distance2;
            /* F-002: the rising ramp has 18 very short triangle portals.
             * Their straight-line projections meet at sharp corners.
             * Rejecting along<0 or along>length throws away a valid ball
             * near a JOIN even when its center is within one ball radius
             * of the actual finite source path. Clamp the nearest point
             * to an endpoint, then compare squared distance (no runtime
             * square root or fabricated movement). Avoid treating the
             * infinite extension of a segment as part of the ramp. */
            if(along<0 || along>e->length) {
                long cx=along<0?e->x:e->ex;
                long cy=along<0?e->y:e->ey;
                long sx=(long)g->x-cx,sy=(long)g->y-cy;
                distance2=sx*sx+sy*sy;
                along=along<0?0:e->length;
            } else {
                long cross=(dx*e->ty-dy*e->tx)/256;
                distance2=cross*cross;
            }
            /* 3px group101 rising and 5px group105 elevated corridors
             * match the original bounded source midline policy. */
            if(distance2>limit*limit || distance2>=best2) continue;
            best2=distance2; found=i; progress=(short)(e->start+along);
            tx=e->tx; ty=e->ty;
            side=(short)((dx*e->ty-dy*e->tx)/256);
            forward=(int)(((long)g->vx*tx+(long)g->vy*ty)/256);
        }
    }
    if(found<0) {
        /* Source s_ramp9 is a FLAT upper plane at z=-5. Its 1301
         * exit changes collision mask. In our 2D projection retain
         * that raised state briefly after leaving the last segment,
         * instead of colliding with overlapping ground decoration. */
        if(g->ramp_active>0 && g->ramp_active<=t->ramp_count &&
           t->ramps[g->ramp_active-1].group==105 &&
           (long)g->ramp_progress*10>=
             (long)t->ramps[g->ramp_active-1].length*9 &&
           g->vy<0)
            g->ramp_exit_ticks=8;
        g->ramp_active=g->ramp_used=0; g->ramp_ticks=0;
        g->ramp_crest_seen=0; return;
    }
    if(g->ramp_active!=found+1) {
        g->ramp_active=g->ramp_used=0; g->ramp_ticks=0;
        g->ramp_exit_ticks=g->ramp_crest_seen=0;
        if(progress>t->ramps[found].length/4 || forward<=0) return;
        g->ramp_active=(unsigned char)(found+1);
        if(g->ramp_entries<65535U) g->ramp_entries++;
        if(g->ramp_entry_by_group[found]<65535U)
            g->ramp_entry_by_group[found]++;
        g->ramp_progress=progress;
    }
    if(++g->ramp_ticks>220) { g->ramp_used=1; return; }
    /* F-002: crossing the actual source crest is distinct from applying
     * a weak-shot boost. Both TRamps report one successful crossing per
     * entry (the flat 105 has no boost), allowing real V20 diagnosis. */
    if(!g->ramp_crest_seen && forward>0 &&
       (long)progress*10>=t->ramps[found].length*9L &&
       (long)g->ramp_progress*10<t->ramps[found].length*9L) {
        g->ramp_crest_seen=1;
        if(g->ramp_crest_by_group[found]<65535U)
            g->ramp_crest_by_group[found]++;
    }
    if(!g->ramp_used && t->ramps[found].assist && forward>0 &&
       (long)progress*10>=t->ramps[found].length*9L &&
       (long)g->ramp_progress*10<t->ramps[found].length*9L) {
        int boost=minimum(64,96-forward);
        g->ramp_used=1;
        if(boost>0) {
            g->vx+=(short)((long)boost*tx/256);
            g->vy+=(short)((long)boost*ty/256);
            if(g->ramp_crest_assists<65535U) g->ramp_crest_assists++;
        }
    }
    /* F-002: small source-guided COURSE correction, only on the curved
     * 18-plane rising ramp. The Windows TRamp uses side-edge collisions
     * plus FieldEffect/plane damping to steer the ball. Flattened DOS
     * has neither; without a directional constraint its initial steep
     * heading flies straight off the rapidly curving ramp after 4 ticks.
     * Remove at most 24 Q6 units of sideways speed per substep, with
     * a quarter of source-path lateral error to help centered shots
     * negotiate tight triangle joins. This rotates velocity, not position;
     * it adds no uphill impulse, scores, new collisions or teleports.
     * Flat ramp105 NEVER steers. Retreat/tilt/shooter/kickback exclude it.
     * Changes saturate the diagnostic counter at 65535. */
    if(t->ramps[found].group==101 && forward>0 && !g->ramp_used) {
        int lateral=(int)(((long)g->vx*ty-(long)g->vy*tx)/256);
        int steer=lateral/2+side/4;
        steer=maximum(-24,minimum(24,steer));
        if(steer<=-2 || steer>=2) {
            g->vx-=(short)((long)steer*ty/256);
            g->vy+=(short)((long)steer*tx/256);
            if(g->ramp_steer_ticks<65535U) g->ramp_steer_ticks++;
        }
    }
    g->ramp_progress=progress;
}

/* F-002: while traversing the actual source TRamp 105 (s_ramp9),
 * the ball's collision group is elevated. Its original flat z=-5
 * planes overlap ground walls 282/283/296/304 in projection.
 * Those ground walls should NOT become an invisible crest barrier.
 * When legitimately exiting above 90% progress, a maximum eight
 * substeps within 12px of the original 1301 exit preserve clearance.
 * Other ramps, ground shots and genuine returns retain full collision.
 * Upper-layer sidewall/collision-height physics is still incomplete. */
static int ramp_above_ground(const Game *g,const Table *t)
{
    int i;
    const RampPath *r;
    const RampSegment *end;
    if(g->waiting || g->over || g->in_shooter ||
       g->kick_delay || g->kick_flight || g->tilted) return 0;
    /* Both original TRamp 101 (18 rising planes) and TRamp 105
     * (two flat upper planes) switch CollisionMask to the ramp's
     * own elevated CollisionGroup on plane contact. The old port only
     * suppressed ground-wall collisions on 105, so a valid shot on 101
     * was reflected by a projected ground rail almost immediately.
     * Do not grant this clearance without legitimate source-path entry. */
    if(g->ramp_active>0 && g->ramp_active<=t->ramp_count &&
       (t->ramps[g->ramp_active-1].group==101 ||
        t->ramps[g->ramp_active-1].group==105)) return 1;
    if(!g->ramp_exit_ticks || g->vy>=0) return 0;
    for(i=0;i<t->ramp_count;i++) {
        r=&t->ramps[i];
        if(r->group!=105 || !r->count) continue;
        end=&r->segments[r->count-1];
        return labs((long)g->x-end->ex)<=12L*FP &&
               labs((long)g->y-end->ey)<=12L*FP;
    }
    return 0;
}

/* TOneway::Collision: front line reflects, reverse line passes. Both are
 * offset in WORLD space before projection, including the source 0.8 factor.
 * Source ground mask only: elevated group is retained but not made solid
 * in the flattened playfield. Tilt suppresses pass events, never solidity.
 * F-002: passive DOS restitution and no original control/scoring dispatch. */
static void collide_oneways(Game *g,const Table *t,short old_x,short old_y)
{
    int i,above=ramp_above_ground(g,t);
    /* F-003: preserve weak-shot returns in the still-flattened shooter. */
    if(g->in_shooter) return;
    for(i=0;i<t->directional_count;i++) {
        const DirectionalLine *e=&t->directional[i];
        long before,after,den,cx,cy,along,length2,dot;
        if(!(e->mask&(above?2:1)) || g->x<e->left || g->x>e->right ||
           g->y<e->top || g->y>e->bottom) continue;
        before=((long)old_x-e->x)*e->nx+((long)old_y-e->y)*e->ny;
        after=((long)g->x-e->x)*e->nx+((long)g->y-e->y)*e->ny;
        if(before<0 || after>=0) continue;
        den=before-after;
        /* Divide before multiplication: Q8 fraction keeps products 32-bit. */
        dot=before*256/den;
        cx=old_x+((long)g->x-old_x)*dot/256;
        cy=old_y+((long)g->y-old_y)*dot/256;
        along=(cx-e->x)*(e->ex-e->x)+(cy-e->y)*(e->ey-e->y);
        length2=(long)(e->ex-e->x)*(e->ex-e->x)+(long)(e->ey-e->y)*(e->ey-e->y);
        if(along<0 || along>length2) continue;
        if(!e->solid) {
            if(!g->tilted && g->oneway_passes<65535U) g->oneway_passes++;
            continue;
        }
        dot=((long)g->vx*e->nx+(long)g->vy*e->ny)/256;
        if(dot<0) {
            g->vx-=(short)(dot*7/4*e->nx/256);
            g->vy-=(short)(dot*7/4*e->ny/256);
        }
        /* 1/4 pixel on approached side, no infinite endpoint barrier. */
        g->x=(short)(cx+e->nx/16); g->y=(short)(cy+e->ny/16);
        if(g->oneway_blocks<65535U) g->oneway_blocks++;
        break; /* Other lines must not reuse a ray changed by this contact. */
    }
}

/* F-002: one-way deflector at the two source-derived sling aprons.
 * DOS-only apron safeguard, distinct from imported TOneway components.
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


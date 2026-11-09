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
    int i,j,found=-1,best=5*FP+1,forward=0;
    short progress=0,tx=0,ty=0;
    if(g->ramp_exit_ticks) g->ramp_exit_ticks--;
    if(g->waiting || g->over || g->tilted || g->in_shooter ||
       g->kick_delay || g->kick_flight) {
        g->ramp_active=g->ramp_used=g->ramp_exit_ticks=0;
        g->ramp_ticks=0; return;
    }
    for(i=0;i<t->ramp_count;i++) {
        const RampPath *r=&t->ramps[i];
        if(g->x<r->left || g->x>r->right || g->y<r->top || g->y>r->bottom) continue;
        for(j=0;j<r->count;j++) {
            const RampSegment *e=&r->segments[j];
            long dx=(long)g->x-e->x,dy=(long)g->y-e->y;
            long along=(dx*e->tx+dy*e->ty)/256;
            int distance;
            if(along<0 || along>e->length) continue;
            distance=(int)labs((dx*e->ty-dy*e->tx)/256);
            /* The original group105 path is 7.4px wide at entry and
             * 4.8px at exit. A ~3px-radius ball can have its CENTER
             * up to 5px off the middle yet remain on the raised plane.
             * Keep the original 3px detector for rising group101. */
            if(distance>(r->group==105?5*FP:3*FP) ||
               distance>=best) continue;
            best=distance; found=i; progress=(short)(e->start+along);
            tx=e->tx; ty=e->ty;
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
        g->ramp_active=g->ramp_used=0; g->ramp_ticks=0; return;
    }
    if(g->ramp_active!=found+1) {
        g->ramp_active=g->ramp_used=0; g->ramp_ticks=0;
        g->ramp_exit_ticks=0;
        if(progress>t->ramps[found].length/4 || forward<=0) return;
        g->ramp_active=(unsigned char)(found+1);
        if(g->ramp_entries<65535U) g->ramp_entries++;
        g->ramp_progress=progress;
    }
    if(++g->ramp_ticks>220) { g->ramp_used=1; return; }
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
    if(g->ramp_active>0 && g->ramp_active<=t->ramp_count &&
       t->ramps[g->ramp_active-1].group==105) return 1;
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


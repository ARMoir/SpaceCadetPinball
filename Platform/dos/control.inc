/* SOURCE MAP: Fixed-step game control
 * ORIGINAL: TPinballTable.cpp; pb.cpp; control.cpp
 * STATUS: ADAPTED
 * MATCHES: shot, scoring, flipper, feed, drain ordering
 * DIVERGES: single ball, 2D fixed-point, incomplete missions
 * EXCEPTIONS: F-001 F-002 F-003 F-004 F-005 F-006 F-007 F-008
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
void game_step(Game *g,const Table *t,unsigned keys)
{
    int i,b,pressed,px,py,shooter_inner_x,edge_rescue=0;
    short old_x=g->x,old_y=g->y;
    g->steps++; g->event=0;
    if(g->over) {
        clear_stuck(g);
        g->shake_frames=0; /* Never leave display offset during game over. */
        g->previous=(short)keys;
        return;
    }
    if(g->hit_delay) g->hit_delay--;
    if(g->nudge_delay) g->nudge_delay--;
    if(g->shake_frames) g->shake_frames--;
    if(g->nudge_return_ticks && !--g->nudge_return_ticks) {
        g->vx-=g->nudge_return_x; g->vy-=g->nudge_return_y;
        g->nudge_return_x=g->nudge_return_y=0;
    }
    if(++g->nudge_decay_ticks>=12) {
        g->nudge_decay_ticks=0;
        if(g->nudge_heat) g->nudge_heat--;
        g->tilt_warning=(unsigned char)(g->nudge_heat>=45);
    }
    pressed=(int)(keys & ~g->previous);
    if(g->waiting) {
        clear_stuck(g);
#include "plunger.inc"
        g->previous=(short)keys;
        return;
    }
    if((pressed&(KEY_NUDGE|KEY_NUDGE_LEFT|KEY_NUDGE_RIGHT)) &&
       !g->nudge_delay && !g->tilted && !g->in_shooter) {
        if(pressed&KEY_NUDGE_LEFT) apply_nudge(g,KEY_NUDGE_LEFT);
        else if(pressed&KEY_NUDGE_RIGHT) apply_nudge(g,KEY_NUDGE_RIGHT);
        else apply_nudge(g,KEY_NUDGE);
    }
    if(g->kick_delay) {
        clear_stuck(g);
        if(!--g->kick_delay) {
            /* Source strike centers are inside the outlane. Keep the walls
             * solid throughout a mainly upward, slightly inward return. */
            g->vx=(short)(g->kick_side==1?16:-16);
            g->vy=-126;
            g->shake_frames=16;
            g->event=4;
        }
        g->previous=(short)keys;
        return;
    }
    /* Half a Q6 unit/substep on average: about 41 px/s^2 at 72.8 Hz.
     * A full launch must clear the curved lane despite wall restitution. */
    if(g->steps&1) g->vy++;
    /* F-006: the old universal -132 Q6 cap clipped powered flipper
     * shots to the same trajectory as full-strength launcher shots.
     * Keep the exact tuned shooter limits; permit faster playfield hits
     * with <=2.75 pixels of motion per physics substep (bounds=4px).
     * Physical V20 performance and missed-wall tests remain required. */
    g->vx=(short)maximum(g->in_shooter?-128:-160,
                         minimum(g->in_shooter?128:160,g->vx));
    g->vy=(short)maximum(g->in_shooter?-132:-176,
                         minimum(g->in_shooter?132:176,g->vy));
    g->x+=g->vx; g->y+=g->vy;
    /* Capture an incoming ball at the strike face before nearby static
     * rail contacts reverse its velocity. Walls remain active after firing. */
    if(catch_kickback(g,t)) {
        g->previous=(short)keys;
        return;
    }
    py=g->y/FP;
    px=g->x/FP;
    b=maximum(0,minimum(BANDS-1,py/8));
    for(i=0;i<t->band_count[b];i++) {
        unsigned short index=t->band[b][i];
        const EdgeBounds *box=&t->bounds[index];
        if(px<box->left || px>box->right ||
           py<box->top || py>box->bottom) continue;
        if(upper_chute_pass(g,&t->edges[index],px,py))
            continue;
        if(!guard_wall_crossing(g,&t->edges[index],old_x,old_y))
            collide(g,&t->edges[index],96,0);
        /* Contact correction can move the ball; refresh only for actual
         * nearby candidates instead of dividing for every indexed edge. */
        px=g->x/FP; py=g->y/FP;
    }
    /* F-002/F-004: enforce radial bumper separation AFTER neighboring
     * flattened static rails. Rails and upper-layer outlines overlap near
     * the upper-left bumper; a second wall correction must not leave the
     * ball buried inside its cap until the following physics step. */
    collide_bumpers(g,t);
    /* Resolve closed slingshots as SOLID wedges, after wall corrections. */
    contain_slingshots(g,t);
    /* Restore the source exterior after all two-sided wall corrections.
     * This catches the actual off-right-table flight seen on hardware. */
    contain_outer_right(g,t);
    contain_outer_left(g,t);
    /* F-002: a paddle contact can itself correct the ball into a nearby
     * solid sling or outside a rail. Recheck only when it moves position. */
    old_x=g->x; old_y=g->y;
#include "flipper.inc"
    if(g->x!=old_x || g->y!=old_y) {
        contain_slingshots(g,t);
        contain_outer_right(g,t);
        contain_outer_left(g,t);
    }
    /* The tube bends left above the feed point. A constant X cutoff marked
     * partial shots as in-play while they were still climbing that bend.
     * Use a conservative sloped inner boundary above the feed, preserving
     * the lower-table cutoff so ordinary right-side drains still count.
     * This is an SCP2 approximation, not imported one-way gate geometry. */
    shooter_inner_x=t->launch_x-10;
    if(g->y/FP<t->launch_y)
        shooter_inner_x-=(t->launch_y-g->y/FP)/4;
    if(g->in_shooter && g->x<shooter_inner_x*FP)
        g->in_shooter=0;

    /* F-002: a ramp/one-way collision group may still be missing from the
     * flattened SCP2 table. The top and side SCREEN limits are never
     * legitimate ball drains in the original game; only the drain below
     * the flippers should lose a ball. Contain rare exits at these outer
     * limits with a passive 3/4-energy reflection, not a teleport or
     * random kick. Imported nearby rails still do the normal collision. */
    if(g->x<2*FP) {
        g->x=2*FP; edge_rescue=1;
        if(g->vx<0) g->vx=(short)(-(long)g->vx*3/4);
    } else if(g->x>237*FP) {
        g->x=237*FP; edge_rescue=1;
        if(g->vx>0) g->vx=(short)(-(long)g->vx*3/4);
    }
    if(g->y<2*FP) {
        g->y=2*FP; edge_rescue=1;
        if(g->vy<0) g->vy=(short)(-(long)g->vy*3/4);
    }
    if(edge_rescue) g->boundary_hits++;

    /* The original plunger can catch/relaunch weak shots returning down
     * the shooter tube. A simple X+-8 check at drain height missed the
     * diagonal lower end of that tube and let the ball leave the screen.
     * Reset once it comes back to the *bottom* of the shooter, before
     * it reaches the shared playfield drain line. */
    if(g->in_shooter && g->vy>0 &&
              g->x>(t->launch_x-18)*FP &&
              g->y>=(t->drain_y-12)*FP) {
        feed(g,t);
    } else if(g->y/FP>t->drain_y) {
        g->balls--; g->event=3;
        if(!g->balls) g->over=1;
        else {
            feed(g,t);
            /* New ball, fresh kicker activations and tilt reset. */
            g->kick_armed[0]=g->kick_armed[1]=1;
            g->tilted=0; g->tilt_warning=0;
            g->nudge_heat=0; g->nudge_return_ticks=0;
            g->nudge_return_x=g->nudge_return_y=0;
        }
    }
    watch_stuck(g,t); /* F-008: once per physics tick, after all collisions. */
    g->previous=(short)keys;
}

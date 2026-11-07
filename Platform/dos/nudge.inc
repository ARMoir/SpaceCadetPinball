/* SOURCE MAP: Nudge/tilt
 * ORIGINAL: nudge.cpp; pb.cpp
 * STATUS: ADAPTED
 * MATCHES: additive impulse with delayed inverse
 * DIVERGES: tuned Q6 impulses, heat and thresholds
 * EXCEPTIONS: F-005
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* Original nudge::_nudge adds direction-relative velocity, shifts the
 * render, and applies its inverse after 0.4 seconds; repeated bumps feed
 * nudge_count and cause a tilt. This uses equivalent additive/reversal
 * mechanics but Q6 velocities, a short heat meter, and integer timing.
 * The table camera shake is handled by the DOS video backend. */
static void apply_nudge(Game *g,int direction)
{
    short dx=0,dy=0;
    if(direction==KEY_NUDGE_LEFT) dx=26;
    else if(direction==KEY_NUDGE_RIGHT) dx=-26;
    else if(direction==KEY_NUDGE) dy=-28;
    if(g->nudge_return_ticks) {
        /* Cancel the old pending inverse before starting a new impulse. */
        g->vx-=g->nudge_return_x;
        g->vy-=g->nudge_return_y;
    }
    g->vx=(short)maximum(-128,minimum(128,g->vx+dx));
    g->vy=(short)maximum(-132,minimum(132,g->vy+dy));
    g->nudge_return_x=dx; g->nudge_return_y=dy;
    g->nudge_return_ticks=29; /* About 0.4 seconds */
    g->nudge_delay=9; /* suppress key bounce; unlike old 1s lockout */
    g->shake_frames=20;
    g->nudges++;
    g->nudge_heat=(short)minimum(100,g->nudge_heat+29);
    g->tilt_warning=(unsigned char)(g->nudge_heat>=45);
    if(g->nudge_heat>=78) {
        g->tilted=1;
        g->tilts++;
        g->event=5;
    }
}


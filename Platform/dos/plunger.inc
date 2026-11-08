/* SOURCE MAP: In-place plunger charge/release
 * ORIGINAL: TPlunger.cpp
 * STATUS: ADAPTED
 * MATCHES: hold-to-charge and release-to-launch
 * DIVERGES: shorter charge and lower random launch strength
 * EXCEPTIONS: F-003
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
        if(keys&KEY_LAUNCH) {
            if(g->charge<PLUNGER_MAX_CHARGE) g->charge++;
        } else if(g->previous&KEY_LAUNCH) {
            int base,variation;
            /* TPlunger::PullbackTimer grows Boost during a 25ms repeat
             * and its collision adds up to 10% random strength. Preserve
             * the source's charged-release model, but limit the random
             * component to about 1%: the projected DOS shooter geometry
             * is much more sensitive to velocity than the original ramps.
             * The 128..129 Q6 full-charge range reaches the playfield in
             * our original-table emulator tests. No floating point. */
            g->plunger_rng=(unsigned short)(g->plunger_rng*25173U+13849U);
            base=44+(g->charge*84)/PLUNGER_MAX_CHARGE;
            variation=(base*(int)(g->plunger_rng&3U))/256;
            g->waiting=0;
            g->vx=0;
            g->vy=(short)-(base+variation);
            /* The visually seated ball lies below the original
             * collision-tested shooter anchor. Do not let the flattened
             * lower rails deflect the shot before it reaches that anchor.
             * This does not alter the chosen plunger impulse. F-003. */
            g->shooter_seat_guide=(unsigned char)
                (g->in_shooter && g->y>t->launch_y*FP?
                 SHOOTER_SEAT_GUIDE_TICKS:0);
            g->event=2;
        }

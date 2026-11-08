/* SOURCE MAP: In-place flipper contact/sweep
 * ORIGINAL: TFlipper.cpp; TFlipperEdge.cpp
 * STATUS: ADAPTED
 * MATCHES: source endpoints, left/right flipper response
 * DIVERGES: sampled sweep, Q6 front-face ball path and held 5px tip
 * EXCEPTIONS: F-006
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
    for(i=0;i<2;i++) {
        int key=1<<i,hit=0;
        if(g->tilted) {
            g->sweep[i]=0;
            hit=collide(g,&g->flips[i][0],192,0);
        } else {
            /* The original TFlipper rotates continuously and sweeps into
             * the ball. At 72.8 substeps, 7 steps span about 96 ms.
             * The previous 3-step pulse (41 ms) ended within a single
             * 18 Hz input/render frame and missed real tip contacts. */
            if((keys^g->previous)&key) g->sweep[i]=7;
            if(g->sweep[i]) {
                int extending=(keys&key)!=0;
                /* The incoming ball path must be tested BEFORE any
                 * sampled edge pushes a crossing onto the underside.
                 * Keep full moving boost through the raised position. */
                hit=guard_flipper_crossing(g,&g->flips[i][extending?0:1],
                                             flight_x,flight_y,192,extending);
                if(!hit) hit=guard_flipper_crossing(g,&g->flips[i][2],
                                                     flight_x,flight_y,192,extending);
                if(!hit) hit=guard_flipper_crossing(g,&g->flips[i][extending?1:0],
                                                     flight_x,flight_y,192,extending);
                if(!hit) hit=collide(g,&g->flips[i][extending?0:1],
                                     192,extending);
                if(!hit) hit=collide(g,&g->flips[i][2],192,extending);
                if(!hit) hit=collide(g,&g->flips[i][extending?1:0],
                                     192,extending);
                g->sweep[i]--;
            }
            if(!hit) hit=guard_flipper_crossing(g,&g->flips[i][(keys&key)!=0],
                                                 flight_x,flight_y,192,
                                                 (keys&key)!=0?2:0);
            if(!hit)
                hit=collide(g,&g->flips[i][(keys&key)!=0],192,
                            (keys&key)!=0?2:0);
            /* F-006: active toe catch is a deliberately forgiving DOS
             * playability adaptation, not an invisible permanent wall.
             * Apply only to descending balls approaching from ABOVE a
             * raised/held flipper. Released, tilted and returning balls
             * retain the original drain and ordinary passive contacts.
             * Cheap comparisons avoid extra 32-bit collision math for
             * the overwhelming majority of physics ticks. */
            /* Held-only inner toe catches fast crossings too, without
             * inventing a permanent center-gap barrier. */
            if(!hit && (keys&key) && !g->in_shooter && g->vy>0 &&
               flight_y<=g->tip_assist[i].y*FP)
                hit=guard_flipper_crossing(g,&g->tip_assist[i],
                                            flight_x,flight_y,192,2);
            if(!hit && (keys&key) && !g->in_shooter && g->vy>0 &&
               g->y<=g->tip_assist[i].y*FP &&
               g->y>=(g->tip_assist[i].y-4)*FP &&
               g->x>=(minimum(g->tip_assist[i].x,
                              g->tip_assist[i].ex)-3)*FP &&
               g->x<=(maximum(g->tip_assist[i].x,
                              g->tip_assist[i].ex)+3)*FP)
                hit=collide(g,&g->tip_assist[i],192,2);
        }
        if(hit) {
            g->flipper_contacts[i]++;
            if(hit==2) g->flipper_power_contacts[i]++;
        }
    }

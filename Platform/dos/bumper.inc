/* SOURCE MAP: Bumper contact
 * ORIGINAL: TBumper.cpp; control.cpp
 * STATUS: ADAPTED
 * MATCHES: radial contact and score events
 * DIVERGES: single layer, fixed score, shared delay
 * EXCEPTIONS: F-004
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-004: Source TBumper responds along a radial collision normal. At close
 * range an eight-sided generic capsule can correct the ball toward the
 * INTERIOR of its closed loop, leaving it trapped against nearby rails.
 * Test the original cap radius instead. The expensive sqrt is only called
 * on an actual nearby bumper candidate, not every physics tick. */
static void collide_bumpers(Game *g,const Table *t)
{
    int i,pass,moved;
    /* F-004: some real source bumper caps overlap in the flattened
     * plane (notably upper-left pair 5/6). A later radial separation
     * can push the ball into an earlier cap. A bounded second pass
     * resolves the earlier cap; ordinary non-contact ticks still use
     * only the original single scan. */
    for(pass=0;pass<2;pass++) {
    moved=0;
    for(i=0;i<t->bumper_count;i++) {
        const Bumper *b=&t->bumpers[i];
        long dx=(long)g->x-b->x,dy=(long)g->y-b->y;
        unsigned long radius2,dist2,dist;
        int nx,ny;
        long dot,impulse;
        if(dx<=-(long)b->radius || dx>=b->radius ||
           dy<=-(long)b->radius || dy>=b->radius) continue;
        radius2=(unsigned long)b->radius*b->radius;
        dist2=(unsigned long)(dx*dx+dy*dy);
        if(dist2>=radius2) continue;
        dist=integer_sqrt(dist2);
        moved=1;
        if(!dist) {
            unsigned long speed=integer_sqrt((unsigned long)(
                (long)g->vx*g->vx+(long)g->vy*g->vy));
            if(speed) {
                nx=(int)(-(long)g->vx*256/(long)speed);
                ny=(int)(-(long)g->vy*256/(long)speed);
            } else { nx=0; ny=-256; }
        } else {
            nx=(int)(dx*256/(long)dist);
            ny=(int)(dy*256/(long)dist);
        }
        /* Reposition outside the cap to prevent another internal hit. */
        g->x+=(short)(((long)(b->radius-(short)dist+12)*nx)/256);
        g->y+=(short)(((long)(b->radius-(short)dist+12)*ny)/256);
        dot=((long)g->vx*nx+(long)g->vy*ny)/256;
        if(dot<0) {
            impulse=-(dot*7)/4+b->boost;
            g->vx+=(short)(impulse*nx/256);
            g->vy+=(short)(impulse*ny/256);
            if(!pass) vary_powered_contact(g,nx,ny,dot);
            if(!g->hit_delay && !g->tilted) {
                g->score+=b->score; g->hits++;
                g->hit_delay=8; g->event=1;
                g->event_bumper=(unsigned char)(i+1);
            }
        }
    }
    if(!moved) break;
    }
}


/* SOURCE MAP: Ball collision
 * ORIGINAL: TBall.cpp; TEdgeSegment.cpp; maths.cpp
 * STATUS: ADAPTED
 * MATCHES: contact normal, response impulse and correction
 * DIVERGES: Q6/Q8 capsules, top-face swept flipper guard and sampled impulses
 * EXCEPTIONS: F-002 F-006
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-006: the source models material-dependent smoothness and per-contact
 * angular dynamics, while the DOS prototype has fixed 3/4 restitution.
 * A gentle, one-time tangential bias on genuine bumper/active-flipper
 * rebounds approximates missing spin/surface variation. No random noise
 * on passive walls, per-frame integration, shooter return or kickback.
 * Q6 +/-8 max is less than 0.13 pixels/physics substep. Seeded once
 * per interactive session, deterministic in tests. V20 budget unverified. */
static void vary_powered_contact(Game *g,int nx,int ny,long incoming)
{
    int jitter;
    long vx,vy;
    if(incoming>=-24 || g->physics_variation_cooldown ||
       g->waiting || g->over || g->in_shooter || g->tilted ||
       g->kick_delay || g->kick_flight) return;
    g->physics_rng=(unsigned short)(g->physics_rng*25173U+13849U);
    jitter=(int)((g->physics_rng>>12)&15U)-8;
    g->physics_variation_cooldown=6; /* one change per true contact */
    if(!jitter) return;
    vx=(long)g->vx-(long)jitter*ny/256;
    vy=(long)g->vy+(long)jitter*nx/256;
    g->vx=(short)maximum(-160,minimum(160,(int)vx));
    g->vy=(short)maximum(-176,minimum(176,(int)vy));
    if(g->physics_variations<65535U) g->physics_variations++;
}

/* Closest-point capsule collision, with Q6 position and Q8 unit tangent.
 * Reflection uses the normal-component decomposition in maths::basic_collision.
 * Restitution is a fixed 3/4; original per-material and spin terms are not used.
 */
/* Flipper kick mode: 0 passive; 1 powered rotating stroke; 2 modest
 * top-face assistance while held. Return: 0 no approach, 1 passive hit,
 * 2 powered hit, so per-side diagnostics distinguish the two. */
static int collide(Game *g,const Edge *e,int radius,int kick)
{
    long rx,ry,along,dx,dy,dist,dot,impulse;
    int nx,ny,powered=0,px=g->x/FP,py=g->y/FP;
    if(px<minimum(e->x,e->ex)-4 || px>maximum(e->x,e->ex)+4 ||
       py<minimum(e->y,e->ey)-4 || py>maximum(e->y,e->ey)+4) return 0;
    rx=(long)g->x-e->x*FP; ry=(long)g->y-e->y*FP;
    along=(rx*e->tx+ry*e->ty)/256;
    if(along<0 || along>e->length) {
        if(along>e->length) { rx=(long)g->x-e->ex*FP; ry=(long)g->y-e->ey*FP; }
        if(rx*rx+ry*ry>=(long)radius*radius) return 0;
        dist=(long)integer_sqrt((unsigned long)(rx*rx+ry*ry));
        if(!dist) { nx=0; ny=-256; dist=1; }
        else { nx=(int)(rx*256/dist); ny=(int)(ry*256/dist); }
    } else {
        nx=-e->ty; ny=e->tx;
        dist=(rx*nx+ry*ny)/256;
        if(dist<0) { dist=-dist; nx=-nx; ny=-ny; }
        if(dist>=radius) return 0;
    }
    dot=((long)g->vx*nx+(long)g->vy*ny)/256;
    dx=(radius-dist+1)*nx/256; dy=(radius-dist+1)*ny/256;
    g->x+=(short)dx; g->y+=(short)dy;
    if(dot>=0 && kick!=1) return 0;
    impulse=-(dot*7)/4+e->boost;
    if(dot<0) { g->vx+=(short)(impulse*nx/256); g->vy+=(short)(impulse*ny/256); }
    if(kick) {
        /* F-006: TFlipperEdge::EdgeCollision applies contact-direction and
         * pivot-distance angular boost during motion. At 18 Hz input scans
         * the previous 3-substep DOS sweep ended too soon. The extended
         * moving sweep gets full boost on ANY of its three interpolated
         * surfaces; otherwise the outer raised edge could only produce a
         * weak passive bounce. A held raised paddle gets a *smaller* boost
         * only for incoming top-face balls, a documented V20 playability
         * concession because original stationary paddles are passive.
         * Never give underside/retreating contacts the held boost. */
        int lever=(int)(along*20/e->length);
        int power,nfx=nx,nfy=ny;
        if(lever<0) lever=0;
        if(lever>20) lever=20;
        if(kick==1 || (nfy<0 && dot<0)) {
            if(nfy>0) { nfx=-nfx; nfy=-nfy; }
            power=kick==1?90+lever*4:36+lever*2;
            g->vx+=(short)((long)power*nfx/256);
            g->vy+=(short)((long)power*nfy/256);
            g->vx=(short)maximum(-160,minimum(160,g->vx));
            g->vy=(short)maximum(-176,minimum(176,g->vy));
            powered=1;
            vary_powered_contact(g,nx,ny,dot);
        }
    }
    if(e->score && !g->hit_delay && !g->tilted) {
        g->score+=e->score; g->hits++; g->hit_delay=8; g->event=1;
    }
    return powered?2:1;
}


/* F-006: A fast ball can cross a thin paddle between physics samples;
 * a normal capsule test then sees its UNDERSIDE and pushes it down.
 * TFlipper::FlipperCollision similarly considers a swept approach ray.
 * Preserve a genuine incoming top-face contact and reuse the existing
 * collide() restitution/kick. Never turn the finite paddle into a wall
 * for balls coming from underneath, missing the tip, or moving away.
 * Almost all ticks bail out before any costly 32-bit division. */
static int guard_flipper_crossing(Game *g,const Edge *e,
                                  short old_x,short old_y,
                                  int radius,int kick)
{
    long d0,d1,along,x,y,span,dot;
    int nx=-e->ty,ny=e->tx,hit;
    short saved_x=g->x,saved_y=g->y;
    if(g->x<(minimum(e->x,e->ex)-4)*FP ||
       g->x>(maximum(e->x,e->ex)+4)*FP ||
       g->y<(minimum(e->y,e->ey)-4)*FP ||
       g->y>(maximum(e->y,e->ey)+4)*FP) return 0;
    if(ny>0) { nx=-nx; ny=-ny; }
    d0=(((long)old_x-(long)e->x*FP)*nx+
        ((long)old_y-(long)e->y*FP)*ny)/256;
    d1=(((long)saved_x-(long)e->x*FP)*nx+
        ((long)saved_y-(long)e->y*FP)*ny)/256;
    if(d0<=0 || d1>=radius || d1>=d0) return 0;
    dot=((long)g->vx*nx+(long)g->vy*ny)/256;
    if(dot>=0) return 0;
    /* Aim for a shallow front-side overlap, NEVER the centerline.
     * The standard capsule response resolves the final ~0.25px. */
    if(d0>radius-16) {
        span=d0-d1;
        x=(long)old_x+((long)saved_x-old_x)*(d0-(radius-16))/span;
        y=(long)old_y+((long)saved_y-old_y)*(d0-(radius-16))/span;
    } else {
        x=(long)old_x+((long)(radius-16)-d0)*nx/256;
        y=(long)old_y+((long)(radius-16)-d0)*ny/256;
    }
    along=((x-(long)e->x*FP)*e->tx+
           (y-(long)e->y*FP)*e->ty)/256;
    if(along<0 || along>e->length) return 0;
    g->x=(short)x; g->y=(short)y;
    hit=collide(g,e,radius,kick);
    if(!hit) { g->x=saved_x; g->y=saved_y; }
    return hit;
}


/* F-002: TLine::FindCollisionDistance tests a source movement ray rather
 * than just its endpoint. Preserve the approached side when a legal DOS
 * substep crosses an indexed static segment. Existing one-way exclusions
 * are applied by the caller. This is a bounded Q6 supplement, not the
 * source layer/mask/time-of-impact engine. Non-crossings avoid division;
 * no allocation or second scan of the table is required. */
static int guard_wall_crossing(Game *g,const Edge *e,short old_x,short old_y)
{
    int nx=-e->ty,ny=e->tx;
    long d0,d1,den,x,y,along,dot,impulse;
    d0=(((long)old_x-(long)e->x*FP)*nx+
        ((long)old_y-(long)e->y*FP)*ny)/256;
    d1=(((long)g->x-(long)e->x*FP)*nx+
        ((long)g->y-(long)e->y*FP)*ny)/256;
    if(!d0 || (d0>0 && d1>0) || (d0<0 && d1<0)) return 0;
    den=d0-d1;
    x=(long)old_x+((long)g->x-old_x)*d0/den;
    y=(long)old_y+((long)g->y-old_y)*d0/den;
    along=((x-(long)e->x*FP)*e->tx+(y-(long)e->y*FP)*e->ty)/256;
    if(along<0 || along>e->length) return 0;
    if(d0<0) { nx=-nx; ny=-ny; }
    g->x=(short)(x+97L*nx/256);
    g->y=(short)(y+97L*ny/256);
    dot=((long)g->vx*nx+(long)g->vy*ny)/256;
    if(dot<0) {
        impulse=-dot*7/4;
        g->vx+=(short)(impulse*nx/256);
        g->vy+=(short)(impulse*ny/256);
    }
    g->wall_crossings++;
    return 1;
}

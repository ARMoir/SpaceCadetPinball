/* SOURCE MAP: Ball collision
 * ORIGINAL: TBall.cpp; TEdgeSegment.cpp; maths.cpp
 * STATUS: ADAPTED
 * MATCHES: contact normal, response impulse and correction
 * DIVERGES: Q6/Q8 2D capsules and sampled impulses
 * EXCEPTIONS: F-002 F-006
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
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
        }
    }
    if(e->score && !g->hit_delay && !g->tilted) {
        g->score+=e->score; g->hits++; g->hit_delay=8; g->event=1;
    }
    return powered?2:1;
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

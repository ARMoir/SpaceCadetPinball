#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int minimum(int a,int b) { return a<b?a:b; }
static int maximum(int a,int b) { return a>b?a:b; }

unsigned long integer_sqrt(unsigned long n)
{
    unsigned long root=0, bit=1UL<<30;
    while (bit>n) bit>>=2;
    while (bit) {
        if (n>=root+bit) { n-=root+bit; root=(root>>1)+bit; }
        else root>>=1;
        bit>>=2;
    }
    return root;
}

static int read_short(FILE *f,short *out)
{
    int a=fgetc(f), b=fgetc(f);
    if (a==EOF || b==EOF) return 0;
    *out=(short)((unsigned)a|((unsigned)b<<8));
    return 1;
}

int table_index(Table *t)
{
    int i,b,lo,hi,k,minx,maxx,miny,maxy,diameter;
    memset(t->band_count,0,sizeof(t->band_count));
    t->bumper_count=0;
    /* F-004: Source TBumper uses a radial normal. Reassemble closed eight-
     * edge source caps once, replacing the generic inward-facing polygons. */
    for(i=0;i+7<t->count;i++) {
        const Edge *e=&t->edges[i];
        Bumper *bumper;
        if(e->score==0) continue;
        minx=maxx=e->x; miny=maxy=e->y;
        for(k=0;k<8;k++) {
            const Edge *p=&t->edges[i+k];
            const Edge *next=&t->edges[i+((k+1)&7)];
            if(p->score!=e->score || p->boost!=e->boost ||
               p->ex!=next->x || p->ey!=next->y) break;
            minx=minimum(minx,minimum(p->x,p->ex));
            maxx=maximum(maxx,maximum(p->x,p->ex));
            miny=minimum(miny,minimum(p->y,p->ey));
            maxy=maximum(maxy,maximum(p->y,p->ey));
        }
        if(k!=8) continue;
        if(t->bumper_count>=MAX_BUMPERS) return 0;
        bumper=&t->bumpers[t->bumper_count++];
        bumper->first=(short)i;
        bumper->x=(short)((minx+maxx)*FP/2);
        bumper->y=(short)((miny+maxy)*FP/2);
        diameter=maximum(maxx-minx,maxy-miny);
        bumper->radius=(short)(diameter*FP/2+96);
        bumper->score=e->score;
        bumper->boost=e->boost;
        i+=7;
    }
    for(i=0;i<t->count;i++) {
        Edge *e=&t->edges[i];
        if(e->length<=0 || abs(e->tx)>256 || abs(e->ty)>256 ||
           e->x < -32 || e->x>350 || e->ex < -32 || e->ex>350 ||
           e->y < -32 || e->y>350 || e->ey < -32 || e->ey>350 ||
           e->score<0 || e->boost<0 || e->boost>128) return 0;
        /* Each radial bumper is tested once instead of eight conflicting
         * side capsules. Preserve ordinary scored line fixtures. */
        for(k=0;k<t->bumper_count;k++)
            if(i>=t->bumpers[k].first &&
               i<t->bumpers[k].first+8) break;
        if(k<t->bumper_count) continue;
        /* Precompute tight screen-space rejection bounds once per table.
         * 8-pixel bands cut candidate checks on a 7 MHz V20. */
        t->bounds[i].left=(short)(minimum(e->x,e->ex)-4);
        t->bounds[i].right=(short)(maximum(e->x,e->ex)+4);
        t->bounds[i].top=(short)(minimum(e->y,e->ey)-4);
        t->bounds[i].bottom=(short)(maximum(e->y,e->ey)+4);
        lo=maximum(0,(minimum(e->y,e->ey)-3)/8);
        hi=minimum(BANDS-1,(maximum(e->y,e->ey)+3)/8);
        for(b=lo;b<=hi;b++) {
            if(t->band_count[b]>=BAND_EDGES) return 0;
            t->band[b][t->band_count[b]++]=(unsigned short)i;
        }
    }
    return 1;
}

int table_load(Table *t,const char *path)
{
    FILE *f=fopen(path,"rb");
    char magic[4];
    int i,j,ok=0;
    short values[9];
    if(!f) return 0;
    if(fread(magic,1,4,f)!=4 || memcmp(magic,"SCP2",4)) goto done;
    if(!read_short(f,&t->count) || t->count<1 || t->count>MAX_EDGES) goto done;
    if(!read_short(f,&t->launch_x) || !read_short(f,&t->launch_y) ||
       !read_short(f,&t->drain_y)) goto done;
    if(t->launch_x<2 || t->launch_x>230 || t->launch_y<2 ||
       t->launch_y>195 || t->drain_y<10 || t->drain_y>198) goto done;
    for(i=0;i<12;i++) {
        if(!read_short(f,&t->flipper[i]) || t->flipper[i]<0 ||
           t->flipper[i]>(i%2?199:239)) goto done;
    }
    for(i=0;i<4;i++) {
        if(!read_short(f,&t->kickback[i]) || t->kickback[i]<0 ||
           t->kickback[i]>(i%2?199:239)) goto done;
    }
    for(i=0;i<t->count;i++) {
        for(j=0;j<9;j++) if(!read_short(f,&values[j])) goto done;
        memcpy(&t->edges[i],values,sizeof(values));
    }
    for(i=0;i<13;i++) {
        Sprite *s=i<5?&t->sprites[i]:&t->plunger[i-5];
        if(!read_short(f,&s->x) || !read_short(f,&s->y) ||
           !read_short(f,&s->w) || !read_short(f,&s->h)) goto done;
        if(s->w<1 || s->h<1 || s->w>64 || s->h>64 ||
           (long)s->w*s->h>1024L || s->x<0 || s->y<0 ||
           s->x+s->w>240 || s->y+s->h>200) goto done;
        if(fread(s->pixels,1,s->w*s->h,f)!=(unsigned)(s->w*s->h)) goto done;
        for(j=0;j<s->w*s->h;j++)
            if(s->pixels[j]>15 && s->pixels[j]!=255) goto done;
    }
    if(fgetc(f)!=EOF) goto done;
    ok=table_index(t);
done:
    fclose(f);
    return ok;
}

static void make_edge(Edge *e,int x,int y,int ex,int ey)
{
    long dx=ex-x,dy=ey-y;
    unsigned long len=integer_sqrt((unsigned long)(dx*dx+dy*dy));
    memset(e,0,sizeof(*e));
    e->x=x; e->y=y; e->ex=ex; e->ey=ey;
    if(!len) len=1;
    e->tx=(short)(dx*256L/(long)len); e->ty=(short)(dy*256L/(long)len);
    e->length=(short)(len*FP);
}

static void feed(Game *g,const Table *t)
{
    g->x=t->launch_x*FP; g->y=t->launch_y*FP;
    g->vx=g->vy=g->charge=0; g->waiting=1;
    g->in_shooter=1;
    g->sweep[0]=g->sweep[1]=0;
}

void game_reset(Game *g,const Table *t)
{
    int i,j;
    memset(g,0,sizeof(*g)); g->balls=3;
    g->kick_armed[0]=g->kick_armed[1]=1;
    g->plunger_rng=0xa5c3U; /* BIOS ticks supply a session seed on DOS. */
    for(i=0;i<2;i++) {
        const short *p=t->flipper+i*6;
        for(j=0;j<2;j++)
            make_edge(&g->flips[i][j],p[0],p[1],p[2+j*2],p[3+j*2]);
        make_edge(&g->flips[i][2],p[0],p[1],
                  (p[2]+p[4])/2,(p[3]+p[5])/2);
    }
    feed(g,t);
}

/* Closest-point capsule collision, with Q6 position and Q8 unit tangent.
 * Reflection uses the normal-component decomposition in maths::basic_collision.
 * Restitution is a fixed 3/4; original per-material and spin terms are not used.
 */
static int collide(Game *g,const Edge *e,int radius,int kick)
{
    long rx,ry,along,dx,dy,dist,dot,impulse;
    int nx,ny,px=g->x/FP,py=g->y/FP;
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
    if(dot>=0 && !kick) return 0;
    impulse=-(dot*7)/4+e->boost;
    if(dot<0) { g->vx+=(short)(impulse*nx/256); g->vy+=(short)(impulse*ny/256); }
    if(kick) {
        /* F-006 / TFlipperEdge::EdgeCollision:
         * The source adds flipper angular-motion boost to the reflected
         * ball vector, scaled by pivot distance and collision direction.
         * The previous DOS code instead replaced both velocity components
         * with nearly the same scripted shot at each contact.
         * Keep the reflected/incoming momentum and add a contact-normal
         * impulse. During a moving sweep the ball may lie exactly on an
         * interpolated contact line: orient the active impulse toward the
         * playfield rather than using an arbitrary zero-distance side.
         * Stationary flipper contact remains passive.
         * Q6 / Q8 integers only; no trig on normal line contact. */
        int lever=(int)(along*20/e->length);
        int power,nfx=nx,nfy=ny;
        if(lever<0) lever=0;
        if(lever>20) lever=20;
        if(nfy>0) { nfx=-nfx; nfy=-nfy; }
        power=90+lever*4;
        g->vx+=(short)((long)power*nfx/256);
        g->vy+=(short)((long)power*nfy/256);
        g->vx=(short)maximum(-160,minimum(160,g->vx));
        g->vy=(short)maximum(-176,minimum(176,g->vy));
    }
    if(e->score && !g->hit_delay && !g->tilted) {
        g->score+=e->score; g->hits++; g->hit_delay=8; g->event=1;
    }
    return 1;
}

/* F-004: Source TBumper responds along a radial collision normal. At close
 * range an eight-sided generic capsule can correct the ball toward the
 * INTERIOR of its closed loop, leaving it trapped against nearby rails.
 * Test the original cap radius instead. The expensive sqrt is only called
 * on an actual nearby bumper candidate, not every physics tick. */
static void collide_bumpers(Game *g,const Table *t)
{
    int i;
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
            if(!g->hit_delay && !g->tilted) {
                g->score+=b->score; g->hits++;
                g->hit_delay=8; g->event=1;
            }
        }
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

/* The source TKickback winds up before striking, then its gate closes.
 * In DOS the original target triplets are not imported yet: allow one
 * out-lane save per side per ball, not endless free saves. The centers are
 * imported from the source strike segments. Trigger widths, timing and
 * impulses remain approximations until original target-state gates exist. */
static int catch_kickback(Game *g,const Table *t)
{
    int i,px=g->x/FP,py=g->y/FP;
    if(g->in_shooter || g->tilted || g->vy<=0)
        return 0;
    for(i=0;i<2;i++) {
        int cx=t->kickback[i*2],cy=t->kickback[i*2+1];
        if(g->kick_armed[i] && abs(px-cx)<=7 && abs(py-cy)<=5) {
            g->kick_armed[i]=0;
            g->kickbacks++;
            g->kick_side=(unsigned char)(i+1);
            g->kick_delay=9; /* 0.12s / 72.8Hz, short 8086-friendly wind-up */
            g->x=(short)(cx*FP); g->y=(short)(cy*FP);
            g->vx=g->vy=0;
            return 1;
        }
    }
    return 0;
}

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

void game_step(Game *g,const Table *t,unsigned keys)
{
    int i,b,pressed,px,py,shooter_inner_x;
    g->steps++; g->event=0;
    if(g->over) {
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
            g->event=2;
        }
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
    for(i=0;i<2;i++) {
        int key=1<<i,hit=0;
        if(g->tilted) {
            g->sweep[i]=0;
            collide(g,&g->flips[i][0],192,0);
            continue;
        }
        if((keys^g->previous)&key) g->sweep[i]=3;
        /* The original flipper is an angular swept capsule. Approximate it
         * with rest, midpoint and raised edges over three substeps. No
         * per-frame trigonometry, allocation or floating-point calculation. */
        if(g->sweep[i]) {
            int extending=(keys&key)!=0;
            hit=collide(g,&g->flips[i][extending?0:1],192,extending);
            if(!hit) hit=collide(g,&g->flips[i][2],192,extending);
            g->sweep[i]--;
        }
        if(!hit)
            collide(g,&g->flips[i][(keys&key)!=0],192,0);
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
        g->x=2*FP;
        if(g->vx<0) g->vx=(short)(-(long)g->vx*3/4);
    } else if(g->x>237*FP) {
        g->x=237*FP;
        if(g->vx>0) g->vx=(short)(-(long)g->vx*3/4);
    }
    if(g->y<2*FP) {
        g->y=2*FP;
        if(g->vy<0) g->vy=(short)(-(long)g->vy*3/4);
    }

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
    g->previous=(short)keys;
}

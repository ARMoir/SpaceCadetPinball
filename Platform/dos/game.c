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
    int i,b,lo,hi;
    memset(t->band_count,0,sizeof(t->band_count));
    for(i=0;i<t->count;i++) {
        Edge *e=&t->edges[i];
        if(e->length<=0 || abs(e->tx)>256 || abs(e->ty)>256 ||
           e->x < -32 || e->x>350 || e->ex < -32 || e->ex>350 ||
           e->y < -32 || e->y>350 || e->ey < -32 || e->ey>350 ||
           e->score<0 || e->boost<0 || e->boost>128) return 0;
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
    if(fread(magic,1,4,f)!=4 || memcmp(magic,"SCP1",4)) goto done;
    if(!read_short(f,&t->count) || t->count<1 || t->count>MAX_EDGES) goto done;
    if(!read_short(f,&t->launch_x) || !read_short(f,&t->launch_y) ||
       !read_short(f,&t->drain_y)) goto done;
    if(t->launch_x<2 || t->launch_x>230 || t->launch_y<2 ||
       t->launch_y>195 || t->drain_y<10 || t->drain_y>198) goto done;
    for(i=0;i<12;i++) {
        if(!read_short(f,&t->flipper[i]) || t->flipper[i]<0 ||
           t->flipper[i]>(i%2?199:239)) goto done;
    }
    for(i=0;i<t->count;i++) {
        for(j=0;j<9;j++) if(!read_short(f,&values[j])) goto done;
        memcpy(&t->edges[i],values,sizeof(values));
    }
    for(i=0;i<5;i++) {
        Sprite *s=&t->sprites[i];
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
    g->sweep[0]=g->sweep[1]=0;
}

void game_reset(Game *g,const Table *t)
{
    int i,j;
    memset(g,0,sizeof(*g)); g->balls=3;
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
        /* Tip contact has greater leverage than a hit near the pivot;
         * retain some incoming horizontal motion instead of a fixed shot. */
        int lever=abs(g->x/FP-e->x);
        if(lever>20) lever=20;
        g->vy=(short)(-88-lever*2);
        g->vx=(short)((g->x/FP-e->x)*3+g->vx/4);
    }
    if(e->score && !g->hit_delay) {
        g->score+=e->score; g->hits++; g->hit_delay=8; g->event=1;
    }
    return 1;
}

void game_step(Game *g,const Table *t,unsigned keys)
{
    int i,b,pressed,px,py;
    g->steps++; g->event=0;
    if(g->over) { g->previous=(short)keys; return; }
    if(g->hit_delay) g->hit_delay--;
    if(g->nudge_delay) g->nudge_delay--;
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
    if((pressed&KEY_NUDGE) && !g->nudge_delay) {
        g->vy=-64; g->vx=(short)(g->x/FP>116?-48:48);
        g->nudge_delay=72;
    }
    /* Half a Q6 unit/substep on average: about 41 px/s^2 at 72.8 Hz.
     * A full launch must clear the curved lane despite wall restitution. */
    if(g->steps&1) g->vy++;
    g->vx=(short)maximum(-128,minimum(128,g->vx));
    g->vy=(short)maximum(-132,minimum(132,g->vy));
    g->x+=g->vx; g->y+=g->vy;
    py=g->y/FP;
    px=g->x/FP;
    b=maximum(0,minimum(BANDS-1,py/8));
    for(i=0;i<t->band_count[b];i++) {
        unsigned short index=t->band[b][i];
        const EdgeBounds *box=&t->bounds[index];
        if(px<box->left || px>box->right ||
           py<box->top || py>box->bottom) continue;
        collide(g,&t->edges[index],96,0);
        /* Contact correction can move the ball; refresh only for actual
         * nearby candidates instead of dividing for every indexed edge. */
        px=g->x/FP; py=g->y/FP;
    }
    for(i=0;i<2;i++) {
        int key=1<<i,hit=0;
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
    if(g->y/FP>t->drain_y || g->x<0 || g->x>239*FP || g->y<0) {
        /* The bottom of the shooter lane shares the table's drain Y.
         * A missed shot rolling back down that narrow lane returns to the
         * plunger without using a ball. Main-playfield drains still count. */
        if(g->y/FP>t->drain_y && g->vy>0 &&
           g->x>=0 && g->x<=239*FP &&
           g->x/FP>=t->launch_x-8 && g->x/FP<=t->launch_x+8) {
            feed(g,t);
        } else {
            g->balls--; g->event=3;
            if(!g->balls) g->over=1;
            else feed(g,t);
        }
    }
    g->previous=(short)keys;
}

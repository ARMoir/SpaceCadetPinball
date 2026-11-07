#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int minimum(int a,int b) { return a<b?a:b; }
static int maximum(int a,int b) { return a>b?a:b; }

/* SCP2 read and source-derived index construction. The real-time ball
 * square root remains in game.c; this setup-only file calls it by API. */
static int read_short(FILE *f,short *out)
{
    int a=fgetc(f), b=fgetc(f);
    if (a==EOF || b==EOF) return 0;
    *out=(short)((unsigned)a|((unsigned)b<<8));
    return 1;
}

/* F-002 solid slingshot cores. SCP2 has two small, closed, irregular
 * triangle outlines directly above the source flipper pivots. Treating
 * all 14 perimeter edges as unrelated two-sided capsules can push balls
 * into their hollow centers. Recover each triangle ONCE from source
 * segments and use an outward-facing solid hull instead.
 *
 * The source loops have ~15x25 pixel bounds, and their positions are
 * checked relative to the imported flipper pivots, not hardcoded indices.
 * Synthetic/demo tables without the original slings still work normally.
 * This scan costs nothing in the 72.8 Hz physics loop. */
static void index_slings(Table *t)
{
    int side,i,j,k,px,py,minx,maxx,miny,maxy,top_l,top_r;
    int left_y,right_y,dx,dy;
    unsigned long len;
    Sling *h;
    const Edge *e,*first;
    memset(t->slings,0,sizeof(t->slings));
    t->sling_count=0;
    for(side=0;side<MAX_SLINGS;side++) {
        px=t->flipper[side*6];
        py=t->flipper[side*6+1];
        h=&t->slings[side];
        for(i=0;i<t->count && !h->count;i++) {
            first=&t->edges[i];
            if(first->score) continue;
            minx=maxx=first->x;
            miny=maxy=first->y;
            for(j=i;j<t->count && j<i+24;j++) {
                e=&t->edges[j];
                if(e->score || (j>i &&
                   (t->edges[j-1].ex!=e->x ||
                    t->edges[j-1].ey!=e->y))) break;
                minx=minimum(minx,minimum(e->x,e->ex));
                maxx=maximum(maxx,maximum(e->x,e->ex));
                miny=minimum(miny,minimum(e->y,e->ey));
                maxy=maximum(maxy,maximum(e->y,e->ey));
                if(j<i+2 || e->ex!=first->x || e->ey!=first->y)
                    continue;
                if(maxx-minx<12 || maxx-minx>18 ||
                   maxy-miny<22 || maxy-miny>29 ||
                   miny<py-48 || miny>py-32 ||
                   maxy<py-22 || maxy>py-10 ||
                   (side==0 && (minx<px-24 || maxx>px+2)) ||
                   (side==1 && (minx<px-2 || maxx>px+24)))
                    break;
                /* Source top is a short horizontal ridge; each bottom
                 * corner is the extreme x endpoint of the outline. */
                top_l=32767; top_r=-32767;
                left_y=right_y=-1;
                for(k=i;k<=j;k++) {
                    const Edge *p=&t->edges[k];
                    if(p->y==miny) {
                        top_l=minimum(top_l,p->x);
                        top_r=maximum(top_r,p->x);
                    }
                    if(p->x==minx) left_y=p->y;
                    if(p->x==maxx) right_y=p->y;
                }
                if(left_y<0 || right_y<0 || top_l>top_r)
                    break;
                h->x[0]=(short)((top_l+top_r)/2);
                h->y[0]=(short)miny;
                h->x[1]=(short)maxx;
                h->y[1]=(short)right_y;
                h->x[2]=(short)minx;
                h->y[2]=(short)left_y;
                /* top -> right -> left must describe a convex core. */
                if((long)(h->x[1]-h->x[0])*(h->y[2]-h->y[0])-
                   (long)(h->y[1]-h->y[0])*(h->x[2]-h->x[0])<=0)
                    break;
                h->left=(short)minx; h->right=(short)maxx;
                h->top=(short)miny; h->bottom=(short)maxy;
                h->first=(short)i; h->count=(short)(j-i+1);
                for(k=0;k<3;k++) {
                    int n=(k+1)%3;
                    dx=h->x[n]-h->x[k];
                    dy=h->y[n]-h->y[k];
                    len=integer_sqrt((unsigned long)((long)dx*dx+
                                                      (long)dy*dy));
                    if(!len) len=1;
                    h->nx[k]=(short)((long)dy*256/(long)len);
                    h->ny[k]=(short)(-(long)dx*256/(long)len);
                }
                t->sling_count++;
                break;
            }
        }
    }
}

int table_index(Table *t)
{
    int i,b,lo,hi,k,minx,maxx,miny,maxy,diameter;
    memset(t->band_count,0,sizeof(t->band_count));
    t->bumper_count=0;
    index_slings(t);
    t->outer_right_rail=-1;
    t->outer_left_rail=-1;
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
        /* The source's long right exterior rail is around x=170..193,
         * y=40..166 in the imported SCP2 projection. Find it by shape
         * rather than binding logic to a hard-coded edge index. This
         * supports a cheap one-sided escape guard after ordinary collisions
         * when the two closely spaced projected walls let a ball tunnel
         * past them. We prefer the rightmost qualifying wall. */
        if(!e->score && minimum(e->y,e->ey)<=45 &&
           maximum(e->y,e->ey)>=160 &&
           maximum(e->x,e->ex)>=185 &&
           abs(e->ey-e->y)>=95 &&
           (t->outer_right_rail<0 ||
            maximum(e->x,e->ex)>
            maximum(t->edges[t->outer_right_rail].x,
                    t->edges[t->outer_right_rail].ex)))
            t->outer_right_rail=(short)i;
        /* Source x=62,y=40 to x=39,y=166 is the matching left exterior.
         * Do not extend a blanket rectangular boundary through playable
         * upper ramps or lower outlanes. This guard uses the actual edge
         * slope only within its imported vertical interval. */
        if(!e->score && minimum(e->y,e->ey)<=45 &&
           maximum(e->y,e->ey)>=160 &&
           minimum(e->x,e->ex)<=42 &&
           maximum(e->x,e->ex)>=60 &&
           abs(e->ey-e->y)>=95 &&
           (t->outer_left_rail<0 ||
            minimum(e->x,e->ex)<
            minimum(t->edges[t->outer_left_rail].x,
                    t->edges[t->outer_left_rail].ex)))
            t->outer_left_rail=(short)i;
        /* Each radial bumper is tested once instead of eight conflicting
         * side capsules. Preserve ordinary scored line fixtures. */
        for(k=0;k<t->bumper_count;k++)
            if(i>=t->bumpers[k].first &&
               i<t->bumpers[k].first+8) break;
        if(k<t->bumper_count) continue;
        /* A sling is a SOLID face, not a stack of 14 independent
         * double-sided line segments. Mixing them recreates the
         * interior trap after we project the ball out of its hull. */
        for(k=0;k<MAX_SLINGS;k++) {
            const Sling *h=&t->slings[k];
            if(h->count && i>=h->first && i<h->first+h->count)
                break;
        }
        if(k<MAX_SLINGS) continue;
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


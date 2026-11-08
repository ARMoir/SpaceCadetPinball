/* SOURCE MAP: Video/sprites/dirty rectangles
 * ORIGINAL: render.cpp; gdrv.cpp
 * STATUS: ADAPTED
 * MATCHES: transparent source sprite compositing
 * DIVERGES: banked B800h or A000h, remapped source palette, no Z layers
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
static unsigned ticks(void)
{
    /* Atomic 16-bit low word. Unsigned subtraction handles wraparound. */
    return *(volatile unsigned __far *)MK_FP(0x40,0x6c);
}

static void mode(unsigned char value)
{
    union REGS r;
    memset(&r,0,sizeof(r)); r.h.ah=0; r.h.al=value; int86(0x10,&r,&r);
}

/* Tandy video is four 8KB banks with two pixels per byte. VGA 13h is
 * a linear 64000-byte framebuffer (one 0..15 palette index per pixel). */
static unsigned address(int x,int y)
{ return (unsigned)((y&3)*8192+(y>>2)*160+(x>>1)); }

static int detect_vga(void)
{
    union REGS r;
    memset(&r,0,sizeof(r));
    r.x.ax=0x1a00; /* BIOS VGA/MCGA Display Combination Code service. */
    int86(0x10,&r,&r);
    return r.h.al==0x1a; /* Non-VGA BIOS leaves AL != 1Ah. */
}

/* RGBI palette from the original 16-color Tandy converter, 6-bit DAC.
 * VGA1 reserves these HUD indices and supplies source-derived art colors.
 * With no VGA1 pack, preserve the original RGBI fallback palette. */
static void set_vga_palette(void)
{
    static const unsigned char dac[48]={
         0, 0, 0,  0, 0,42,  0,42, 0,  0,42,42,
        42, 0, 0, 42, 0,42, 42,21, 0, 42,42,42,
        21,21,21, 21,21,63, 21,63,21, 21,63,63,
        63,21,21, 63,21,63, 63,63,21, 63,63,63
    };
    int i;
    outp(0x3c8,0);
    if(vga_colors) {
        for(i=0;i<768;i++) outp(0x3c9,background[64000U+(unsigned)i]);
    } else for(i=0;i<48;i++) outp(0x3c9,dac[i]);
}

static void pixel(int x,int y,unsigned color)
{
    unsigned a;
    if(x<0 || x>=320 || y<0 || y>=200) return;
    if(vga_mode) {
        video[(unsigned)y*320U+(unsigned)x]=(unsigned char)color;
        return;
    }
    a=address(x,y);
    if(x&1) video[a]=(video[a]&0xf0)|(unsigned char)color;
    else video[a]=(video[a]&0x0f)|(unsigned char)(color<<4);
}

static void restore(int x,int y,int w,int h)
{
    int row,start,end;
    unsigned a;
    if(x<0) { w+=x; x=0; } if(y<0) { h+=y; y=0; }
    if(x+w>320) w=320-x; if(y+h>200) h=200-y;
    if(w<=0 || h<=0) return;
    start=x/2; end=(x+w+1)/2;
    if(vga_mode) {
        /* Match Tandy's two-pixel dirty-rectangle granularity exactly:
         * this prevents subtly different flipper/ball restores on VGA. */
        for(row=y;row<y+h;row++) {
            a=(unsigned)row*320U+(unsigned)(start*2);
            _fmemcpy(video+a,background+a,(unsigned)((end-start)*2));
        }
    } else {
        for(row=y;row<y+h;row++) {
            a=address(start*2,row);
            _fmemcpy(video+a,background+a,end-start);
        }
    }
}

/* Draw a sprite by scanline. The Tandy's interleaved banks make a
 * pixel() call (with modulo and address calculation) expensive on a V20. */
static void sprite(const Sprite *s,int x,int y)
{
    int xx,yy,start,end,dst_y;
    unsigned a;
    for(yy=0;yy<s->h;yy++) {
        const unsigned char *p=s->pixels+yy*s->w;
        dst_y=y+yy;
        if(dst_y<0 || dst_y>=200) continue;
        start=x<0?-x:0;
        end=s->w;
        if(x+end>320) end=320-x;
        if(start>=end) continue;
        if(vga_mode) {
            /* Linear VGA writes with a transparent source sprite.
             * No per-pixel multiplication or division on this hot path. */
            a=(unsigned)dst_y*320U+(unsigned)(x+start);
            for(xx=start;xx<end;xx++,a++)
                if(p[xx]!=255) video[a]=p[xx];
            continue;
        }
        a=address(x+start,dst_y);
        for(xx=start;xx<end;xx++) {
            unsigned char c=p[xx];
            if(c!=255) {
                unsigned char old=video[a];
                if((x+xx)&1) video[a]=(old&0xf0)|c;
                else video[a]=(old&0x0f)|(unsigned char)(c<<4);
            }
            if((x+xx)&1) a++;
        }
    }
}

/* A stationary flipper only needs repainting if the old ball was erased
 * over it. Otherwise leave its pixels alone until the key state changes. */
static int ball_overlaps(const Sprite *s,int x,int y,int w,int h)
{
    return x<s->x+s->w && x+w>s->x &&
           y<s->y+s->h && y+h>s->y;
}

static int rect_overlaps(int x,int y,int w,int h,
                         int x2,int y2,int w2,int h2)
{
    return x<x2+w2 && x+w>x2 && y<y2+h2 && y+h>y2;
}


/* SOURCE MAP: Source assets and screen capture
 * ORIGINAL: loader.cpp; gdrv.cpp
 * STATUS: DOS-ONLY
 * MATCHES: source-derived ball/table assets
 * DIVERGES: SCP2 and DOS-specific Tandy/VGA unpacking
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* VGA1 is an optional presentation-only companion to SCP2 (F-007).
 * Reuse the VGA far background allocation, with 768 DAC bytes at its end.
 * The thirteen sprite headers must exactly match the geometry pack. */
static int load_vga_art(void)
{
    FILE *f=fopen("VGA.DAT","rb");
    unsigned char buf[500];
    unsigned i,j;
    int ok=1;
    if(!f) return -1; /* Legacy RGBI fallback when the optional pack is absent. */
    if(fread(buf,1,4,f)!=4 || memcmp(buf,"VGA1",4)) ok=0;
    for(i=0;i<3 && ok;i++) {
        if(fread(buf,1,256,f)!=256) { ok=0; break; }
        for(j=0;j<256;j++) if(buf[j]>63) ok=0;
        _fmemcpy(background+64000U+i*256U,buf,256);
    }
    for(i=0;i<128 && ok;i++) {
        if(fread(buf,1,500,f)!=500) { ok=0; break; }
        _fmemcpy(background+i*500U,buf,500);
    }
    for(i=0;i<13 && ok;i++) {
        Sprite *s=i<5?&table.sprites[i]:&table.plunger[i-5];
        short dims[4];
        if(fread(dims,sizeof(short),4,f)!=4 || dims[0]!=s->x ||
           dims[1]!=s->y || dims[2]!=s->w || dims[3]!=s->h) {
            ok=0; break;
        }
        if(fread(s->pixels,1,s->w*s->h,f)!=(unsigned)(s->w*s->h)) ok=0;
    }
    if(fgetc(f)!=EOF) ok=0;
    fclose(f);
    if(ok) vga_colors=1;
    return ok;
}

static int load_background(int *enhanced)
{
    FILE *f=0;
    unsigned i;
    unsigned char buf[512];
    int ok=1;
    if(vga_mode && !rgbi_mode) {
        int result=load_vga_art();
        if(result>=0) { *enhanced=0; return result; }
    }
    if(*enhanced) f=fopen("TABLE3D.TGA","rb");
    if(!f) {
        if(*enhanced)
            puts("Enhanced table not found: using original TABLE.TGA.");
        *enhanced=0;
        f=fopen("TABLE.TGA","rb");
    }
    if(!f) return 0;
    if(vga_mode) {
        /* Expand the existing 32768-byte Tandy art directly into a
         * 64000-byte linear VGA background, ONCE at startup. Read each
         * 8K bank as 50x160 used bytes plus 192 unused bytes. No second
         * 32K allocation, no extra disk assets, no runtime conversion. */
        unsigned bank,row,x;
        for(bank=0;bank<4 && ok;bank++) {
            for(row=0;row<50;row++) {
                unsigned dst=(unsigned)(row*4U+bank)*320U;
                if(fread(buf,1,160,f)!=160) { ok=0; break; }
                for(x=0;x<160;x++) {
                    unsigned char pair=buf[x];
                    background[dst+2*x]=(unsigned char)(pair>>4);
                    background[dst+2*x+1]=(unsigned char)(pair&15);
                }
            }
            if(ok && fread(buf,1,192,f)!=192) ok=0;
        }
    } else {
        for(i=0;i<64;i++) {
            if(fread(buf,1,512,f)!=512) { ok=0; break; }
            _fmemcpy(background+i*512U,buf,512);
        }
    }
    if(fgetc(f)!=EOF) ok=0;
    fclose(f); return ok;
}

static void dump_screen(void)
{
    FILE *f=fopen("SCREEN.BIN","wb");
    unsigned i;
    unsigned char buf[512];
    if(!f) return;
    for(i=0;i<(vga_mode?125U:64U);i++) {
        _fmemcpy(buf,video+i*512U,512); fwrite(buf,1,512,f);
    }
    fclose(f);
}


/* SOURCE MAP: Source assets and screen capture
 * ORIGINAL: loader.cpp; gdrv.cpp
 * STATUS: DOS-ONLY
 * MATCHES: source-derived ball/table assets
 * DIVERGES: SCP2 and DOS-specific Tandy/VGA unpacking
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
static int load_background(int *enhanced)
{
    FILE *f=0;
    unsigned i;
    unsigned char buf[512];
    int ok=1;
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


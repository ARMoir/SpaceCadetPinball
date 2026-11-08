/* SOURCE MAP: Input and mouse
 * ORIGINAL: options.cpp; winmain.cpp
 * STATUS: ADAPTED
 * MATCHES: Z and /, mouse left/right flippers, middle-button plunger,
 *          F5 sound and F8 controls shortcuts from options.cpp
 * DIVERGES: DOS IRQ1 / INT33h; source has more input bindings
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* Single-producer IRQ1, single-consumer main-loop initials input queue.
 * 16 slots, one empty to distinguish full; never does DOS I/O in IRQ. */
static volatile unsigned char score_capture=0;
static volatile unsigned char score_scans[16];
static volatile unsigned char score_head=0,score_tail=0;
static int mouse_middle_available=0;

static char score_letter(unsigned scan)
{
    static const unsigned char scans[26]={
        30,48,46,32,18,33,34,35,23,36,37,38,50,
        49,24,25,16,19,31,20,22,47,17,45,21,44
    };
    int i;
    for(i=0;i<26;i++)
        if(scan==scans[i]) return (char)('A'+i);
    return 0;
}

static unsigned score_next_scan(void)
{
    unsigned scan=0;
    _disable();
    if(score_tail!=score_head) {
        scan=score_scans[score_tail];
        score_tail=(unsigned char)((score_tail+1)&15);
    }
    _enable();
    return scan;
}

static void __interrupt __far keyboard(void)
{
    unsigned scan=inp(0x60), acknowledge=inp(0x61);
    outp(0x61,acknowledge|0x80); outp(0x61,acknowledge);
    /* XT/V20 make/break scancodes; extended prefixes are ignored. */
    if(scan!=0xe0 && scan!=0xe1) {
        unsigned code=scan&127;
        /* Capture actual key-down transitions for high-score initials,
         * before the usual game-key masks consume the pending flag. */
        if(score_capture && !(scan&128) && !keys[code] &&
           score_letter(code)) {
            unsigned char next=(unsigned char)((score_head+1)&15);
            if(next!=score_tail) {
                score_scans[score_head]=(unsigned char)code;
                score_head=next;
            }
        }
        /* Menu and toggle keys must fire only once per physical press.
         * Flipper/launcher quick-tap latching stays unchanged. */
        if(!(scan&128) &&
           ((code!=1 && code!=28 && code!=31 && code!=49 &&
             code!=50 && code!=59 && code!=60 && code!=61 &&
             code!=63 && code!=66 && code!=72 && code!=80) || !keys[code]))
            pressed_keys[code]=1;
        keys[code]=(scan&128)?0:1;
    }
    outp(0x20,0x20);
}

/* Optional Microsoft/FreeDOS-style mouse driver, INT 33h.
 * A mouse driver must be loaded in DOS. Probe the vector first: on XT-era
 * DOS an uninstalled 33h can be 0000:0000 or an IRET stub. No serial
 * hardware is touched directly; mouse-less systems have zero polling cost.
 * AX=0 resets driver, hides its cursor, and reports button count in BX. */
static int mouse_detect(void)
{
    const unsigned char __far *handler=
        (const unsigned char __far *)_dos_getvect(0x33);
    union REGS r;
    if(!handler || *handler==0xcf) return 0;
    memset(&r,0,sizeof(r));
    r.x.ax=0;
    int86(0x33,&r,&r);
    mouse_middle_available=(r.x.ax==0xffff && r.x.bx>=3);
    return r.x.ax==0xffff && r.x.bx!=0;
}

/* Once per displayed frame (18.2 Hz), not in the 72.8 Hz physics loop.
 * Function 3 captures held buttons. Function 5 also latches presses
 * released between screen frames, important for short pinball taps.
 * Left=bit0, right=bit1, middle=bit2 when driver reports >=3
 * buttons. No video/physics work is performed in the INT33h calls. */
static unsigned mouse_buttons(void)
{
    union REGS r;
    unsigned bits;
    r.x.ax=3;
    int86(0x33,&r,&r);
    bits=r.x.bx&(mouse_middle_available?7U:3U);
    r.x.ax=5; r.x.bx=0; /* Count and clear left presses. */
    int86(0x33,&r,&r);
    if(r.x.bx) bits|=1U;
    r.x.ax=5; r.x.bx=1; /* Count and clear right presses. */
    int86(0x33,&r,&r);
    if(r.x.bx) bits|=2U;
    if(mouse_middle_available) {
        r.x.ax=5; r.x.bx=2; /* Latch fast middle click for plunger. */
        int86(0x33,&r,&r);
        if(r.x.bx) bits|=4U;
    }
    return bits;
}


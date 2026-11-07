/* SOURCE MAP: Input and mouse
 * ORIGINAL: options.cpp; winmain.cpp
 * STATUS: ADAPTED
 * MATCHES: Z and /, mouse left/right flippers
 * DIVERGES: DOS IRQ1 / INT33h; middle click not ported
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
static void __interrupt __far keyboard(void)
{
    unsigned scan=inp(0x60), acknowledge=inp(0x61);
    outp(0x61,acknowledge|0x80); outp(0x61,acknowledge);
    /* XT/V20 make/break scancodes; extended prefixes are ignored. */
    if(scan!=0xe0 && scan!=0xe1) {
        unsigned code=scan&127;
        /* Menu and toggle keys must fire only once per physical press.
         * Flipper/launcher quick-tap latching stays unchanged. */
        if(!(scan&128) &&
           ((code!=1 && code!=28 && code!=31 && code!=49 &&
             code!=50 && code!=59 && code!=60 && code!=61 &&
             code!=72 && code!=80) || !keys[code]))
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
    return r.x.ax==0xffff && r.x.bx!=0;
}

/* Once per displayed frame (18.2 Hz), not in the 72.8 Hz physics loop.
 * Function 3 captures held buttons. Function 5 also latches presses
 * released between screen frames, important for short pinball taps.
 * Left=bit0, right=bit1. Mouse position/cursor/middle button ignored. */
static unsigned mouse_buttons(void)
{
    union REGS r;
    unsigned bits;
    r.x.ax=3;
    int86(0x33,&r,&r);
    bits=r.x.bx&3U;
    r.x.ax=5; r.x.bx=0; /* Count and clear left presses. */
    int86(0x33,&r,&r);
    if(r.x.bx) bits|=1U;
    r.x.ax=5; r.x.bx=1; /* Count and clear right presses. */
    int86(0x33,&r,&r);
    if(r.x.bx) bits|=2U;
    return bits;
}


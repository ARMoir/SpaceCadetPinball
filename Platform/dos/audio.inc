/* SOURCE MAP: Sound events
 * ORIGINAL: Sound.cpp; TSound.cpp
 * STATUS: DOS-ONLY
 * MATCHES: event feedback
 * DIVERGES: PSG/PC-speaker substitute rather than samples
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
static void silence(void)
{
    if(!sound_enabled) return;
    if(vga_mode) {
        /* On AT/VGA hardware port 0xC0 is not a Tandy PSG.
         * Use the standard PC speaker gate instead. */
        outp(0x61,inp(0x61)&0xfc);
    } else {
        outp(0xc0,0x9f); outp(0xc0,0xbf); outp(0xc0,0xdf); outp(0xc0,0xff);
    }
}

static void play_event(int event)
{
    unsigned divisor;
    if(!sound_enabled) return;
    if(!event) { if(sound_ticks && !--sound_ticks) silence(); return; }
    divisor=event==1?180:event==2?320:event==4?230:
            event==5?760:600;
    if(vga_mode) {
        /* PIT channel 2, PC speaker. Trigger notes on the same events,
         * preserving Tandy PSG output exactly in mode 09h. */
        divisor=(unsigned)(divisor*6U);
        outp(0x43,0xb6);
        outp(0x42,divisor&255);
        outp(0x42,divisor>>8);
        outp(0x61,inp(0x61)|3);
    } else {
        outp(0xc0,0x80|(divisor&15));
        outp(0xc0,divisor>>4);
        outp(0xc0,0x94);
    }
    sound_ticks=2;
}


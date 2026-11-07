/* SOURCE MAP: Hardware screen shake
 * ORIGINAL: render.cpp
 * STATUS: DOS-ONLY
 * MATCHES: nudge visual feedback
 * DIVERGES: Tandy CRTC only; VGA disabled
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* Shift the 6845 start-address by one unit for a short screen jolt.
 * This shifts the visible image in hardware without copying 32 KB of
 * graphics. Always restore zero on pause/exit; /noshake disables it on
 * systems with unusual video hardware. */
static void camera_shift(int offset)
{
    /* VGA has different CRTC memory units: never apply Tandy screen shake
     * to VGA. Keep all physics/state shake counters unchanged. */
    if(vga_mode) { video_shift=0; return; }
    if(!shake_enabled) offset=0;
    if(video_shift==offset) return;
    outp(0x3d4,13);
    outp(0x3d5,offset);
    video_shift=offset;
}


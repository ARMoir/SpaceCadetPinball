/* SOURCE MAP: Sound events
 * ORIGINAL: Sound.cpp; TSound.cpp, component-local sound callbacks
 * STATUS: ADAPTED
 * MATCHES: event-driven sound categories; mute and pause behavior
 * DIVERGES: short original-inspired hardware note sequences, not WAV samples
 * EXCEPTIONS: F-007
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* F-007: non-blocking 18.2Hz sequence. A cue programs SN76489 (Tandy
 * PSG) or PIT2 (IBM PC speaker); no sample buffers, timers or disk I/O.
 * Cues: 1 bumper, 2 launch, 3 drain, 4 kickback, 5 tilt,
 *       6 flipper, 7 nudge, 8 sling. High-priority effects preempt clicks.
 * The desktop source uses independent WAV channels, so these synthesized
 * one-/two-voice cues are approximations, not claimed original samples. */
#define SOUND_CUE_COUNT 8
#define SOUND_MAX_NOTES 5
static const unsigned short sound_notes[SOUND_CUE_COUNT][SOUND_MAX_NOTES]={
    {230,165,120,0,0},   /* bumper rises as it fires */
    {440,340,270,210,0}, /* plunger rises */
    {220,330,480,650,780}, /* ball drain falls */
    {320,240,170,130,0}, /* kickback fanfare */
    {600,850,700,960,0}, /* tilt alarm */
    {580,0,0,0,0},       /* flipper mechanical click */
    {460,360,0,0,0},     /* nudge thump */
    {285,170,245,0,0}   /* sling rebound */
};
static const unsigned char sound_lengths[SOUND_CUE_COUNT]={3,4,5,4,4,1,2,3};
static const unsigned char sound_priority[SOUND_CUE_COUNT]={3,2,6,5,7,1,2,4};
static unsigned char sound_current=0,sound_step=0,sound_variant=0;

static void silence(void)
{
    if(!sound_enabled) return;
    if(vga_mode) {
        outp(0x61,inp(0x61)&0xfc);
    } else {
        outp(0xc0,0x9f); outp(0xc0,0xbf);
        outp(0xc0,0xdf); outp(0xc0,0xff);
    }
}

/* One tone update per display frame, with optional PSG two-voice harmony.
 * Tandy clock/32 is ~112 kHz, PIT2 ~1193 kHz: 11x approximates pitch.
 * There is never direct Tandy port access on an IBM VGA machine. */
static void sound_note(unsigned divisor,unsigned phase)
{
    if(vga_mode) {
        unsigned pit=(unsigned)(divisor*11U);
        outp(0x43,0xb6);
        outp(0x42,pit&255U);
        outp(0x42,pit>>8);
        outp(0x61,inp(0x61)|3);
    } else {
        unsigned volume=phase*2U+2U;
        if(volume>12U) volume=12U;
        outp(0xc0,0x80|(divisor&15U));
        outp(0xc0,divisor>>4);
        outp(0xc0,0x90|volume);
        if(sound_current==2 || sound_current==4) {
            unsigned second=(divisor*3U)/2U;
            if(second>1023U) second=1023U;
            outp(0xc0,0xa0|(second&15U));
            outp(0xc0,second>>4);
            outp(0xc0,0xb8);
        } else outp(0xc0,0xbf);
    }
}

static void play_event(int event,unsigned detail)
{
    unsigned divisor;
    if(!sound_enabled) {
        sound_ticks=0; sound_current=0; sound_step=0;
        return;
    }
    if(event>0 && event<=SOUND_CUE_COUNT &&
       (!sound_current || !sound_ticks ||
        sound_priority[event-1]>=sound_priority[sound_current-1])) {
        sound_current=(unsigned char)event;
        sound_step=0;
        sound_ticks=sound_lengths[event-1];
        sound_variant=(unsigned char)(event==1?(detail%3U):0);
    }
    if(sound_current && sound_ticks) {
        divisor=sound_notes[sound_current-1][sound_step];
        if(sound_current==1) divisor+=(unsigned)(sound_variant*32U);
        sound_note(divisor,sound_step);
        sound_step++; sound_ticks--;
    } else if(sound_current) {
        silence();
        sound_current=0; sound_step=0;
    }
}

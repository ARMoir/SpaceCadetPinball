#!/usr/bin/env python3
"""Guards for F-007 hardware audio and source-derived table lamps.

These are intentionally source-contract checks. The CI also compiles actual
16-bit C89 and runs the full TEST.EXE Tandy and VGA parity/emulator suite.
"""
import re
import unittest
from pathlib import Path

BASE = Path(__file__).resolve().parent
AUDIO = (BASE / "audio.inc").read_text()
LAMPS = (BASE / "lamps.inc").read_text()
MAIN = (BASE / "main.c").read_text()
GAME = (BASE / "game.h").read_text()
BUMPER = (BASE / "bumper.inc").read_text()
CONTROL = (BASE / "control.inc").read_text()

class SoundLampTests(unittest.TestCase):
    def test_eight_unique_bounded_hardware_cues(self):
        rows = re.search(r"sound_notes\[SOUND_CUE_COUNT\]\[SOUND_MAX_NOTES\]\s*=\s*\{(.*?)\};",
                         AUDIO, re.S)
        self.assertIsNotNone(rows)
        values = [tuple(int(n) for n in re.findall(r"\d+", line))
                  for line in rows.group(1).splitlines() if "{" in line]
        self.assertEqual(len(values), 8)
        self.assertEqual(len(set(values)), 8)
        self.assertTrue(all(len(v) == 5 and all(0 <= n <= 1023 for n in v)
                            for v in values))
        self.assertIn("sound_priority[event-1]>=sound_priority[sound_current-1]", AUDIO)
        self.assertIn("if(vga_mode)", AUDIO)
        self.assertIn("outp(0x61,inp(0x61)|3)", AUDIO)
        self.assertIn("outp(0xc0,0x80|(divisor&15U))", AUDIO)
        self.assertIn("outp(0xc0,0xbf)", AUDIO)
        self.assertIn("sound_ticks=0; sound_current=0", AUDIO)

    def test_sound_sources_and_bumper_identity(self):
        self.assertIn("g->event_bumper=(unsigned char)(i+1)", BUMPER)
        self.assertIn("g->steps++; g->event=0; g->event_bumper=0;", CONTROL)
        self.assertIn("unsigned char event_bumper;", GAME)
        for cue in ("sound_event=8;", "sound_event=7;", "sound_event=6;"):
            self.assertIn(cue, MAIN)
        self.assertIn("play_event(sound_event,(unsigned)event_bumper)", MAIN)
        self.assertIn("prev_nudges=prev_slings[0]=prev_slings[1]=0;", MAIN)

    def test_source_anchors_and_no_full_redraw(self):
        self.assertIn("#define LAMP_COUNT (MAX_BUMPERS+4)", LAMPS)
        self.assertIn("t->bumpers[i].x/FP", LAMPS)
        self.assertIn("t->kickback[side*2]", LAMPS)
        self.assertIn("t->slings[i-MAX_BUMPERS-2]", LAMPS)
        self.assertNotIn("*x=t->launch_x+7", LAMPS)
        self.assertNotIn("return g->waiting &&", LAMPS)
        self.assertIn("} else return 0;", LAMPS)
        self.assertIn("lamp_ticks[hit_bumper-1]=12", LAMPS)
        self.assertIn("g->kick_armed[side]", LAMPS)
        self.assertNotIn("g->waiting", LAMPS)
        self.assertIn("restore(x-2,y-2,5,5)", LAMPS)
        self.assertNotIn("restore(0,0,320,200)", LAMPS)
        self.assertNotIn("fopen(", LAMPS)
        self.assertNotIn("malloc(", LAMPS)
        self.assertIn("if(lights_damage&1) ball_redraw=1", MAIN)
        self.assertIn("if(lights_damage&2) plunger_redraw=1", MAIN)
        self.assertLess(MAIN.index("lamps_render(&table"), MAIN.index("if(plunger_redraw) sprite("))
        self.assertLess(MAIN.index("lamps_render(&table"), MAIN.index("sprite(&table.sprites[4]"))
        self.assertIn('"/nolights"', MAIN)
        self.assertIn("if(lights_enabled)", MAIN)

    def test_rgb_palette_and_static_budget(self):
        values = re.search(r"lamp_colors\[LAMP_COUNT\]\s*=\s*\{([^}]+)\}", LAMPS)
        self.assertIsNotNone(values)
        entries = [int(x) for x in re.findall(r"\d+", values.group(1))]
        self.assertEqual(len(entries), 12)
        self.assertTrue(all(0 <= x < 16 for x in entries))
        self.assertIn("if(lamp_ticks[i]) lamp_ticks[i]--;", LAMPS)
        self.assertIn("if(!active) return;", LAMPS)
        self.assertIn("lamps_invalidate();", LAMPS)

if __name__ == "__main__":
    unittest.main()

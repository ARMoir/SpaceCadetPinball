#!/usr/bin/env python3
"""Guard the source-aligned DOS modules, not upstream gameplay parity."""
import re
import unittest
from pathlib import Path

DOS = Path(__file__).resolve().parent
GAME = ("state.inc", "ball_collision.inc", "bumper.inc", "slingshot.inc",
        "rail_collision.inc", "kickback.inc", "nudge.inc", "unstuck.inc",
        "control.inc")
PLATFORM = ("video_shake.inc", "input.inc", "video.inc", "hud.inc",
            "audio.inc", "assets.inc")
INLINE = ("plunger.inc", "flipper.inc")


def included(text):
    return re.findall(r'^\s*#include "([a-z_]+\.inc)"\s*$', text, re.M)


class SourceLayoutTests(unittest.TestCase):
    def test_module_layout_and_provenance(self):
        self.assertEqual(included((DOS/"game.c").read_text()), list(GAME))
        self.assertEqual(included((DOS/"main.c").read_text()), list(PLATFORM))
        self.assertEqual(included((DOS/"control.inc").read_text()), list(INLINE))
        all_files = GAME + PLATFORM + INLINE
        self.assertEqual(len(all_files), len(set(all_files)))
        source_map = (DOS/"SOURCE_MAP.md").read_text()
        for name in all_files:
            body = (DOS/name).read_text()
            for field in ("ORIGINAL:", "MATCHES:", "DIVERGES:",
                          "STATUS:", "EXCEPTIONS:"):
                self.assertIn(field, body, name)
            self.assertIn(name, source_map, name)

    def test_build_links_table_setup_once(self):
        script = (DOS/"build.py").read_text()
        self.assertIn("['main.c','game.c','table.c','scores.c']", script)
        self.assertIn("['test_game.c','game.c','table.c','scores.c']", script)
        self.assertIn("integer_sqrt(", (DOS/"game.c").read_text())
        self.assertIn("table_index(", (DOS/"table.c").read_text())

    def test_unchanged_inlined_branches(self):
        ctrl = (DOS/"control.inc").read_text()
        self.assertIn('#include "plunger.inc"', ctrl)
        self.assertIn('#include "flipper.inc"', ctrl)
        self.assertIn("if(g->waiting)", ctrl)
        self.assertIn("for(i=0;i<2;i++)", (DOS/"flipper.inc").read_text())
        self.assertIn("g->charge<PLUNGER_MAX_CHARGE", (DOS/"plunger.inc").read_text())


if __name__ == "__main__":
    unittest.main()

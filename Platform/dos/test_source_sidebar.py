"""Source-style DOS sidebar contract checks (F-007).

Source digit file is externally supplied FONT.DAT; the game includes only
the tiny offline-converted score_font.h. These checks complement the 16-bit
Open Watcom build and full DOSBox-X Tandy/VGA gameplay tests.
"""
import re
import unittest
from pathlib import Path

BASE=Path(__file__).resolve().parent
HUD=(BASE/"hud.inc").read_text()
FONT=(BASE/"score_font.h").read_text()
MAIN=(BASE/"main.c").read_text()


class SourceSidebarTests(unittest.TestCase):
    def test_original_digits_are_compact_and_present(self):
        m=re.search(r"score_digits\[10\]\[10\]\s*=\s*\{(.*?)\};",FONT,re.S)
        self.assertIsNotNone(m)
        rows=re.findall(r"\{([^{}]+)\}",m.group(1))
        self.assertEqual(len(rows),10)
        bitmaps=[list(map(lambda x:int(x,16),
                          re.findall(r"0x[0-9a-fA-F]+",row))) for row in rows]
        self.assertTrue(all(len(x)==10 for x in bitmaps))
        self.assertEqual(len({tuple(x) for x in bitmaps}),10)
        self.assertEqual(sum(len(x) for x in bitmaps),100)
        self.assertIn("SCORE_DIGIT_W 5",FONT)
        self.assertIn("SCORE_DIGIT_H 8",FONT)
        self.assertIn("ba89418be9aed5d7f5bc197c00fab4cacf3aff23a1ddc0de134010414acf2d0b",FONT)
        self.assertIn("static const unsigned char colors[4]={0,1,13,15}",HUD)
        self.assertIn("score_digits[digit][source>>2]",HUD)

    def test_digits_have_upright_readable_orientation(self):
        body=re.search(r"score_digits\[10\]\[10\].*?\{(.*?)\};",FONT,re.S).group(1)
        glyphs=[]
        for row in re.findall(r"\{([^{}]+)\}",body):
            data=[int(v,16) for v in re.findall(r"0x[0-9a-f]+",row)]
            glyphs.append([[(data[(y*5+x)//4]>>(6-2*((y*5+x)%4)))&3
                            for x in range(5)] for y in range(8)])
        # Test character anatomy, not just ten distinct byte arrays:
        # 3 has a right stem; 7 a top bar; 4's lower stem is on the right;
        # 6 closes its lower bowl while 9 closes its upper bowl.
        ink=lambda row: [v>=2 for v in row]
        for y in (2,5):
            self.assertTrue(any(ink(glyphs[3][y])[3:]))
            self.assertFalse(any(ink(glyphs[3][y])[:2]))
        self.assertGreaterEqual(sum(ink(glyphs[7][0])),3)
        self.assertLessEqual(sum(ink(glyphs[7][7])),1)
        self.assertFalse(any(ink(glyphs[4][6])[:2]))
        self.assertTrue(any(ink(glyphs[4][6])[3:]))
        self.assertTrue(any(ink(glyphs[6][5])[:2]))
        self.assertFalse(any(ink(glyphs[9][5])[:2]))
        self.assertTrue(any(ink(glyphs[9][2])[:2]))
        self.assertFalse(any(ink(glyphs[6][2])[3:]))

    def test_all_score_sizes_fit_right_cell(self):
        # Source score region x=264..317. The last pixel always x=315.
        for length in range(1,11):
            step=6 if length<=8 else 5
            left=316-((length-1)*step+5)
            right=left+(length-1)*step+4
            self.assertGreaterEqual(left,265)
            self.assertLessEqual(right,316)
        self.assertIn("sprintf(buf,\"%lu\",game.score);",HUD)
        self.assertNotIn("\"%010lu\"",HUD)
        self.assertIn("int step=(count<=8U)?6:5;",HUD)

    def test_source_panel_positions_are_disjoint(self):
        for rect in (
            "panel_frame(240,59,80,24)",
            "panel_frame(240,86,80,24)",
            "panel_frame(241,87,21,22)",
            "panel_frame(264,87,54,22)",
            "panel_frame(240,113,80,30)",
            "panel_frame(240,146,80,52)",
        ):
            self.assertIn(rect,HUD)
        self.assertIn("restore(240,59,80,141)",HUD)
        self.assertIn("score_number(316,94,buf)",HUD)
        self.assertIn("text_color(274,68,\"BALL\",12)",HUD)
        for label in ("F2 STARTS NEW","CONTINUE PLAY","3D PINBALL",
                      "SPACE CADET","AWAITING","DEPLOYMENT"):
            self.assertIn('\"'+label+'\"',HUD)
        self.assertIn("static void draw_menu_background(void)",HUD)
        self.assertIn("static void draw_title(void)",HUD)
        self.assertIn("source_message(245,152,\"3D PINBALL\")",HUD)
        self.assertIn('#include "score_font.h"',MAIN)

    def test_score_event_does_not_repaint_the_sidebar(self):
        # Valid inclusive inner bounds: x266..315, y93..102 for score.
        # All four patches exclude the metallic frames and source logo.
        for rect in ("restore(266,93,50,10)", "restore(304,67,11,9)",
                     "restore(243,115,74,26)", "restore(243,179,74,16)"):
            self.assertIn(rect, HUD)
        self.assertIn("static void draw_score_only(void)", HUD)
        self.assertIn("static void draw_status_only(void)", HUD)
        self.assertIn("if(old_score!=game.score) draw_score_only();", MAIN)
        self.assertIn("if(old_balls!=game.balls) draw_ball_only();", MAIN)
        self.assertIn("if(old_over!=game.over) draw_command_only(paused);",MAIN)
        self.assertIn("if(hud_status_dirty || old_wait!=game.waiting", MAIN)
        self.assertIn("if(!hud_event_frames || hud_event!=event)", MAIN)
        event_section=re.search(
            r"if\(event\) \{(.*?)\} else if\(hud_event_frames", MAIN, re.S)
        self.assertIsNotNone(event_section)
        self.assertNotIn("ui_dirty=1", event_section.group(1))
        self.assertIn("hud_status_dirty=1", event_section.group(1))
        self.assertEqual(HUD.count("restore(240,59,80,141)"),1)
        # Initial, menu and explicit view/UI transitions retain full redraw.
        self.assertEqual(MAIN.count("draw_panel(view,choice,paused);"),3)

    def test_no_runtime_asset_parse_or_gameplay_changes(self):
        self.assertNotIn('fopen("FONT.DAT"',MAIN)
        self.assertNotIn('fopen("FONT.DAT"',HUD)
        self.assertIn("if(vga_mode)",HUD)
        self.assertIn("panel_frame(240,86,80,111)",HUD)
        for label in ("CONTROLS","HIGH SCORES","NEW HIGH SCORE",
                      "Z OR LEFT CLICK","SPACE OR MID CLK","ENTER SAVE"):
            self.assertIn('\"'+label+'\"',HUD)

if __name__=="__main__":
    unittest.main()

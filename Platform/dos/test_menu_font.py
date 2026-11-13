"""Check that 3x5 DOS help glyphs match the documented nudge keys.

The previous 'A' bitmap was identical to 'H', causing the real Tandy F1
help to show apparent 'H D SIDE BUMP' although the left key is A.
"""
import re
import unittest
from pathlib import Path

DOS = Path(__file__).resolve().parent
MAIN = (DOS / "main.c").read_text()
HUD = (DOS / "hud.inc").read_text()
INPUT = (DOS / "input.inc").read_text()


def glyph(letter):
    match = re.search(
        r"static const unsigned short font\[\]\s*=\s*\{([^}]+)\};",
        HUD, flags=re.S)
    if not match:
        raise ValueError("Missing font array")
    numbers = [int(n) for n in re.findall(r"\d+", match.group(1))]
    if len(numbers) != 36:
        raise ValueError("Expected exactly ten digits and 26 letters")
    bits = numbers[10 + ord(letter) - ord("A")]
    return tuple("".join("#" if bits & (1 << (14 - j*3 - i)) else "."
                         for i in range(3)) for j in range(5))


class HelpTextTests(unittest.TestCase):
    def test_A_and_H_are_distinct_and_readable(self):
        self.assertEqual(glyph("A"), (".#.", "#.#", "###", "#.#", "#.#"))
        self.assertEqual(glyph("H"), ("#.#", "#.#", "###", "#.#", "#.#"))
        self.assertNotEqual(glyph("A"), glyph("H"))

    def test_readable_R_and_distinct_P_M_N_at_three_by_five(self):
        # R must have a closed upper bowl and TWO feet, rather than
        # looking like the old 'P' with one descending stroke.
        self.assertEqual(glyph("R"),
                         ("##.", "#.#", "##.", "#.#", "#.#"))
        self.assertEqual(glyph("P"),
                         ("##.", "#.#", "##.", "#..", "#.."))
        # A readable N needs continuous left/right vertical strokes.
        # A broken lower-left stem made the former diagonal ambiguous.
        self.assertEqual(glyph("N"),
                         ("#.#", "##.", "###", "#.#", "#.#"))
        rows=glyph("N")
        self.assertTrue(all(row[0]=="#" for row in rows))
        # In 3px, the upper diagonal uses the right-stem pixel
        # before rejoining it; the lower right remains continuous.
        self.assertTrue(all(row[2]=="#" for row in rows[2:]))
        self.assertNotEqual(rows[1],glyph("M")[1])
        self.assertEqual(glyph("M"),
                         ("#.#", "###", "###", "#.#", "#.#"))
        self.assertNotEqual(glyph("R"), glyph("P"))
        self.assertNotEqual(glyph("R"), glyph("B"))
        self.assertNotEqual(glyph("N"), glyph("M"))
        alphabet=[glyph(chr(c)) for c in range(ord("A"),ord("Z")+1)]
        self.assertEqual(len(set(alphabet)),26)

    def test_font_stays_tiny_and_renderer_keeps_sidebar_spacing(self):
        match=re.search(
            r"static const unsigned short font\[\]\s*=\s*\{([^}]+)\};",
            HUD, flags=re.S)
        self.assertIsNotNone(match)
        values=[int(n) for n in re.findall(r"\d+",match.group(1))]
        self.assertEqual(len(values),36)
        self.assertTrue(all(0 <= value < (1<<15) for value in values))
        self.assertIn("bits&(1U<<(14-j*3-i))",HUD)
        self.assertIn("x+=4; s++;",HUD)
        self.assertIn("for(j=0;j<5;j++) for(i=0;i<3;i++)",HUD)
        # The source-style score digits remain their separate 5x8 font.
        self.assertIn("score_digits[digit][source>>2]",HUD)
        self.assertIn("for(row=0;row<8U;row++)",HUD)
        self.assertIn("for(col=0;col<5U;col++,source++)",HUD)
        self.assertIn("restore(266,93,50,10)",HUD)

    def test_mouse_help_and_driver_latching_are_available(self):
        self.assertIn('"Z OR LEFT CLICK"', HUD)
        self.assertIn('"/ OR RIGHT CLICK"', HUD)
        # Slash needs an explicit 3x5 glyph in this letter-only font.
        self.assertIn("bits=*s=='/'?4772U:", HUD)
        # Detect an installed driver, and poll held and quick clicks only
        # when present; leave smoke tests independent of host mouse state.
        self.assertIn("if(!smoke && !no_mouse) mouse_available=mouse_detect();", MAIN)
        self.assertIn("mouse_input=mouse_available?mouse_buttons():0;", MAIN)
        self.assertIn("MOUSE_GAME_KEYS(mouse_input)", MAIN)
        self.assertIn("r.x.ax=3;", INPUT)
        self.assertIn("r.x.ax=5; r.x.bx=0;", INPUT)
        self.assertIn("r.x.ax=5; r.x.bx=1;", INPUT)
        self.assertIn("r.x.ax=5; r.x.bx=2;", INPUT)
        self.assertIn("if(mouse_middle_available)", INPUT)
        self.assertIn("key_f5=pressed_keys[63]", MAIN)
        self.assertIn("key_f8=pressed_keys[66]", MAIN)
        self.assertIn("score_next_scan()", MAIN)
        self.assertIn('"HIGH SCORES"', HUD)
        self.assertIn('"NEW HIGH SCORE"', HUD)
        self.assertIn("key_new=0; key_f2=0;", MAIN)
        self.assertIn("if(!smoke && old_over==0 && game.over", MAIN)
        self.assertIn('scores_load("SCORES.DAT")', MAIN)
        self.assertIn('scores_save("SCORES.DAT")', MAIN)
        self.assertIn("score_capture && !(scan&128) && !keys[code]", INPUT)
        self.assertIn('"BUMPER HIT"', HUD)
        self.assertIn('"/nomouse"', MAIN)

    def test_help_shows_real_nudge_keys(self):
        for label in ("A LEFT BUMP", "D RIGHT BUMP", "X UP BUMP"):
            self.assertIn('"' + label + '"', HUD)
        self.assertIn("keys[30]||pressed_keys[30]", MAIN)
        self.assertIn("keys[32]||pressed_keys[32]", MAIN)
        self.assertIn("keys[45]||pressed_keys[45]", MAIN)


if __name__ == "__main__":
    unittest.main()

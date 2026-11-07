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

    def test_mouse_help_and_driver_latching_are_available(self):
        self.assertIn('"Z OR LEFT CLICK"', HUD)
        self.assertIn('"/ OR RIGHT CLICK"', HUD)
        # Slash needs an explicit 3x5 glyph in this letter-only font.
        self.assertIn("bits=*s=='/'?4772U:", HUD)
        # Detect an installed driver, and poll held and quick clicks only
        # when present; leave smoke tests independent of host mouse state.
        self.assertIn("if(!smoke && !no_mouse) mouse_available=mouse_detect();", MAIN)
        self.assertIn("MOUSE_FLIPPER_KEYS(mouse_buttons())", MAIN)
        self.assertIn("r.x.ax=3;", INPUT)
        self.assertIn("r.x.ax=5; r.x.bx=0;", INPUT)
        self.assertIn("r.x.ax=5; r.x.bx=1;", INPUT)
        self.assertIn('"/nomouse"', MAIN)

    def test_help_shows_real_nudge_keys(self):
        for label in ("A LEFT BUMP", "D RIGHT BUMP", "X UP BUMP"):
            self.assertIn('"' + label + '"', HUD)
        self.assertIn("keys[30]||pressed_keys[30]", MAIN)
        self.assertIn("keys[32]||pressed_keys[32]", MAIN)
        self.assertIn("keys[45]||pressed_keys[45]", MAIN)


if __name__ == "__main__":
    unittest.main()

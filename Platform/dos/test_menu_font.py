"""Check that 3x5 DOS help glyphs match the documented nudge keys.

The previous 'A' bitmap was identical to 'H', causing the real Tandy F1
help to show apparent 'H D SIDE BUMP' although the left key is A.
"""
import re
import unittest
from pathlib import Path

MAIN = (Path(__file__).resolve().parent / "main.c").read_text()


def glyph(letter):
    match = re.search(
        r"static const unsigned short font\[\]\s*=\s*\{([^}]+)\};",
        MAIN, flags=re.S)
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

    def test_help_shows_real_nudge_keys(self):
        for label in ("A LEFT BUMP", "D RIGHT BUMP", "X UP BUMP"):
            self.assertIn('"' + label + '"', MAIN)
        self.assertIn("keys[30]||pressed_keys[30]", MAIN)
        self.assertIn("keys[32]||pressed_keys[32]", MAIN)
        self.assertIn("keys[45]||pressed_keys[45]", MAIN)


if __name__ == "__main__":
    unittest.main()

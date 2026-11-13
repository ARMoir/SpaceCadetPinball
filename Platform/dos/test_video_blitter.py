#!/usr/bin/env python3
"""Regression model for Tandy 09h packed two-pixel sprite writes.

The DOS executable is still exercised in DOSBox-X by the normal workflow:
this test exhausts the alignment, transparency and clipping edge cases that
are easy to miss in a final-frame screenshot, without requiring DOS hardware.
"""
import random
import unittest
from pathlib import Path

WIDTH, HEIGHT = 320, 200
BYTES_PER_ROW = 160
TRANSPARENT = 255


def original_row(dest, pixels, x):
    """Reference: previous DOS per-pixel read/modify/write loop."""
    for i, c in enumerate(pixels):
        px = x + i
        if not 0 <= px < WIDTH or c == TRANSPARENT:
            continue
        a = px // 2
        if px & 1:
            dest[a] = (dest[a] & 0xF0) | c
        else:
            dest[a] = (dest[a] & 0x0F) | (c << 4)


def packed_row(dest, pixels, x):
    """Model the byte-aligned Tandy branch of video.inc's sprite()."""
    start = -x if x < 0 else 0
    end = len(pixels)
    if x + end > WIDTH:
        end = WIDTH - x
    if start >= end:
        return
    xx = start
    a = (x + start) // 2
    if (x + xx) & 1:
        c = pixels[xx]
        xx += 1
        if c != TRANSPARENT:
            dest[a] = (dest[a] & 0xF0) | c
        a += 1
    while xx + 1 < end:
        left, right = pixels[xx:xx + 2]
        if left == TRANSPARENT:
            if right != TRANSPARENT:
                dest[a] = (dest[a] & 0xF0) | right
        elif right == TRANSPARENT:
            dest[a] = (dest[a] & 0x0F) | (left << 4)
        else:
            dest[a] = (left << 4) | right
        xx += 2
        a += 1
    if xx < end and pixels[xx] != TRANSPARENT:
        dest[a] = (dest[a] & 0x0F) | (pixels[xx] << 4)


class PackedTandyTests(unittest.TestCase):
    def assert_equal_blit(self, pixels, x, baseline):
        reference, optimized = bytearray(baseline), bytearray(baseline)
        original_row(reference, pixels, x)
        packed_row(optimized, pixels, x)
        self.assertEqual(optimized, reference, (x, pixels))

    def test_scanline_lookup_matches_both_hardware_layouts(self):
        tandy = [(y & 3) * 8192 + (y >> 2) * 160
                 for y in range(HEIGHT)]
        vga = [y * 320 for y in range(HEIGHT)]
        for y in range(HEIGHT):
            for x in (0, 1, 2, 157, 318, 319):
                self.assertEqual(tandy[y] + (x >> 1),
                                 (y & 3) * 8192 + (y >> 2) * 160 + (x >> 1))
                self.assertEqual(vga[y] + x, y * 320 + x)
        self.assertLess(tandy[-1] + 159, 32768)
        self.assertLess(vga[-1] + 319, 65536)

    def test_every_packed_pair_transparency_combination(self):
        baseline = bytes((i * 37 + 0x5A) & 255 for i in range(BYTES_PER_ROW))
        for first in range(17):
            for second in range(17):
                pixels = [TRANSPARENT if first == 16 else first,
                          TRANSPARENT if second == 16 else second]
                for x in (0, 1, 2, 3, 318, 319, -1, -2):
                    self.assert_equal_blit(pixels, x, baseline)

    def test_clipping_and_odd_widths(self):
        randomizer = random.Random(0xC0A)
        for x in list(range(-12, 13)) + list(range(307, 333)):
            for width in range(1, 20):
                pixels = [randomizer.randrange(17) for _ in range(width)]
                pixels = [TRANSPARENT if c == 16 else c for c in pixels]
                baseline = bytes(randomizer.randrange(256)
                                 for _ in range(BYTES_PER_ROW))
                self.assert_equal_blit(pixels, x, baseline)

    def test_randomized_sprite_rows(self):
        randomizer = random.Random(0xC0DE)
        for _ in range(1500):
            x = randomizer.randrange(-35, 325)
            width = randomizer.randrange(1, 65)
            pixels = [randomizer.randrange(17) for _ in range(width)]
            pixels = [TRANSPARENT if c == 16 else c for c in pixels]
            baseline = bytes(randomizer.randrange(256)
                             for _ in range(BYTES_PER_ROW))
            self.assert_equal_blit(pixels, x, baseline)

    def test_dos_rendering_code_is_wired_to_lookup(self):
        folder = Path(__file__).resolve().parent
        video = (folder / "video.inc").read_text()
        main = (folder / "main.c").read_text()
        self.assertIn("static unsigned video_row[200]", video)
        self.assertIn("static void init_video_rows(void)", video)
        self.assertIn("init_video_rows();", main)
        self.assertIn("while(xx+1<end)", video)
        self.assertIn("video[a]=(unsigned char)((left<<4)|right);", video)


if __name__ == "__main__":
    unittest.main()

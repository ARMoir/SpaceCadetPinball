#!/usr/bin/env python3
"""F-007 source-color Tandy artwork fidelity and safety regressions."""
import unittest
from pathlib import Path

from convert import PALETTE, pack_screen
from enhance_table import unpack_screen
from source_art import source_art, palette_choices, distance

DATA = Path(__file__).parent / "data"
TABLE = (DATA / "TABLE.DAT").read_bytes()
VGA = (DATA / "VGA.DAT").read_bytes()
CLASSIC = (DATA / "TABLE.TGA").read_bytes()


def build_fixture(index, rgb6, base_color=7):
    """Recolor a source palette entry in a valid source-matched VGA pack."""
    vga = bytearray(VGA)
    vga[4+index*3:7+index*3] = bytes(rgb6)
    classic = [[0]*320 for _ in range(200)]
    for y in range(35, 64):
        for x in range(40, 74):
            vga[772+y*320+x] = index
            classic[y][x] = base_color
    return bytes(vga), bytes(pack_screen(classic))


class SourceTandyTests(unittest.TestCase):
    def test_real_source_produces_stable_compact_bank_image(self):
        encoded, pixels, stats = source_art(VGA, TABLE, CLASSIC)
        self.assertEqual(len(encoded), 32768)
        self.assertEqual(encoded, bytes(pack_screen(pixels)))
        self.assertEqual(unpack_screen(encoded), pixels)
        self.assertEqual(source_art(VGA,TABLE,CLASSIC)[0], encoded)
        self.assertGreater(stats['patterned'], 0)
        self.assertGreater(stats['protected_edges'], 0)
        self.assertGreater(stats['solid'], 0)
        self.assertGreater(sum(a!=b for a,b in zip(encoded,CLASSIC)), 50)
        original = unpack_screen(CLASSIC)
        for y in range(200):
            for x in range(320):
                if x>=236 or y>=191:
                    self.assertEqual(pixels[y][x],original[y][x])
                self.assertTrue(0 <= pixels[y][x] <= 15)
        self.assertEqual(CLASSIC,(DATA/"TABLE.TGA").read_bytes())
        self.assertEqual(VGA,(DATA/"VGA.DAT").read_bytes())
        self.assertEqual(TABLE,(DATA/"TABLE.DAT").read_bytes())

    def test_half_red_uses_genuine_rgbi_pair_for_flat_region(self):
        # 85 red is exactly midway between Tandy black and dark red.
        vga,classic = build_fixture(200,(21,0,0))
        raw,pixels,stats = source_art(vga,TABLE,classic)
        self.assertEqual(len(raw),32768)
        self.assertGreater(stats['patterned'],100)
        values={pixels[y][x] for y in range(40,58) for x in range(45,65)}
        self.assertEqual(values,{0,4})
        # Actual paired CRT appearance approximates the source midpoint.
        used=[PALETTE[pixels[y][x]][0] for y in range(40,58)
              for x in range(45,65)]
        self.assertEqual(sum(used)//len(used),85)

    def test_one_pixel_highlight_stays_sharp_and_palette_exact(self):
        vga,classic = build_fixture(200,(21,0,0))
        vga = bytearray(vga)
        base = [[0]*320 for _ in range(200)]
        for y in range(35,64):
            for x in range(40,74):
                base[y][x]=7
                # Source-derived single-pixel bright rail, no averaging.
                if x==56:
                    vga[772+y*320+x]=15
                    base[y][x]=15
        raw,pixels,stats=source_art(bytes(vga),TABLE,bytes(pack_screen(base)))
        self.assertEqual(len(raw),32768)
        self.assertTrue(all(pixels[y][56]==15 for y in range(36,63)))
        # A neighbor with the strong source edge is drawn as one
        # solid RGBI color, not a stippled pair.
        self.assertEqual(len({pixels[y][55] for y in range(37,60)}),1)
        self.assertGreater(stats['protected_edges'],0)

    def test_corrupt_mismatch_and_palette_validation(self):
        for vga in (b"bad", VGA[:-1], VGA[:200]):
            with self.subTest(length=len(vga)),self.assertRaises(ValueError):
                source_art(vga,TABLE,CLASSIC)
        with self.assertRaises(ValueError):
            source_art(VGA,TABLE,CLASSIC[:-1])
        with self.assertRaises(ValueError):
            source_art(VGA,b"bad",CLASSIC)

    def test_pair_choice_penalizes_excessive_contrast(self):
        solids, pairs=palette_choices(PALETTE)
        self.assertEqual(solids,list(range(16)))
        self.assertTrue(all(pair is None for pair in pairs))
        self.assertGreater(distance((255,255,255),(0,0,0)),41000)


if __name__=="__main__":
    unittest.main()

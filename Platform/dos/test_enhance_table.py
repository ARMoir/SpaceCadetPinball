import unittest
from pathlib import Path
from convert import pack_screen
from enhance_table import bumper_caps, enhance, generate, unpack_screen

HERE = Path(__file__).resolve().parent


class OfflineDepthTests(unittest.TestCase):
    def test_nibble_and_bank_round_trip(self):
        img = [[(3*x+y) % 16 for x in range(320)] for y in range(200)]
        encoded = bytes(pack_screen(img))
        self.assertEqual(len(encoded), 32768)
        self.assertEqual(unpack_screen(encoded), img)

    def test_reject_incorrect_asset(self):
        with self.assertRaises(ValueError):
            unpack_screen(b"bad")
        with self.assertRaises(ValueError):
            bumper_caps(b"bad")

    def test_neutral_metal_bevel(self):
        base = [[0] * 320 for _ in range(200)]
        for y in range(45, 55):
            for x in range(60, 72):
                base[y][x] = 7 if x > 60 and y > 45 else 8
        base[46][61] = 13  # Colored lamp must never change.
        shaded, counts = enhance(base, [(65,50,5,5)])
        self.assertEqual(shaded[46][61], 13)
        self.assertGreater(sum(counts.values()), 0)
        self.assertEqual(len(shaded), 200)
        self.assertEqual(len(shaded[0]), 320)

    def test_original_source_art_and_collision_pack_remain_untouched(self):
        raw = (HERE / "data" / "TABLE.TGA").read_bytes()
        pack = (HERE / "data" / "TABLE.DAT").read_bytes()
        art = unpack_screen(raw)
        enhanced, shaded, caps, changes = generate(raw, pack)
        self.assertEqual(len(caps), 7)  # Seven known Space Cadet caps.
        self.assertEqual(len(raw), len(enhanced))
        self.assertEqual((HERE / "data" / "TABLE.TGA").read_bytes(), raw)
        self.assertEqual((HERE / "data" / "TABLE.DAT").read_bytes(), pack)
        self.assertEqual(pack_screen(shaded), enhanced)
        changed = 0
        for y in range(200):
            for x in range(320):
                old, new = art[y][x], shaded[y][x]
                self.assertTrue(0 <= new <= 15)
                if old != new:
                    changed += 1
                    self.assertIn(old, (7, 8, 15))
                    self.assertLess(x, 236)
                    self.assertLess(y, 191)
                if x >= 240 or y >= 192:
                    self.assertEqual(old, new)
        self.assertGreater(changed, 30)
        self.assertLess(changed, 7000)
        # Second-pass art adds noticeably more surface relief, entirely
        # during offline generation. Its counters make unintended weakening
        # of the rail/bumper bevels visible in CI.
        self.assertGreater(changes["rail_rim"]+changes["rail_underside"],
                           0)
        self.assertGreater(changes["bumper_dome"]+
                           changes["bumper_underside"], 0)
        self.assertGreater(changes["contact_occlusion"]+
                           changes["hardware_relief"], 0)
        # All additional height/sidewall/occlusion effects are built into
        # the table image, never evaluated in the game's 16-bit loop.
        self.assertGreater(changes["height_rim"]+
                           changes["mach_band"], 0)
        self.assertGreater(changes["height_sidewall"], 0)
        self.assertGreater(changes["height_contact"]+
                           changes["height_dither"], 0)
        self.assertGreater(changes["high_cap_shadow"], 0)
        self.assertGreater(sum(changes.values()), 100)
        self.assertNotEqual(raw, enhanced)
        self.assertEqual(generate(raw, pack)[0], enhanced)  # Reproducible.


if __name__ == "__main__":
    unittest.main()

import unittest
from pathlib import Path
from convert import pack_screen
from enhance_table import (bumper_caps, enhance, generate, unpack_screen,
                           polish_artifacts, sharpen_metal_from_vga)

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

    def test_cleanup_removes_pits_halos_isolated_sparkles_not_bevels(self):
        base = [[0] * 320 for _ in range(200)]
        for y in range(40, 54):
            for x in range(50, 80):
                base[y][x] = 7 if x < 70 else 8
        # Source steel remains solid; do not invent a black pit on it.
        art = [row[:] for row in base]
        art[44][72] = 0
        # The color and adjacent original gray should remain unmodified.
        base[45][59] = 13
        art[45][59] = 13
        art[45][60] = 15
        # Single isolated shading in otherwise unmodified metal is noise.
        art[49][54] = 15
        # Continuous connected bright bevel is intended detail.
        art[42][54] = art[42][55] = art[42][56] = 15
        counts = {}
        polish_artifacts(base, art, counts)
        self.assertEqual(art[44][72], 8)
        self.assertEqual(art[45][60], base[45][60])
        self.assertEqual(art[49][54], base[49][54])
        self.assertEqual(art[42][54:57], [15, 15, 15])
        self.assertEqual(art[45][59], 13)
        self.assertEqual(counts["artifact_pits"], 1)
        self.assertEqual(counts["artifact_color_halos"], 1)
        self.assertEqual(counts["artifact_sparkles"], 1)

    def test_cleanup_never_blurs_colored_or_classic_art(self):
        base = [[0] * 320 for _ in range(200)]
        # A diagonal source speck is not continuous raised metal.
        base[60][60] = base[59][59] = 8
        out = [row[:] for row in base]
        out[60][60] = 7
        counts = {}
        polish_artifacts(base, out, counts)
        self.assertEqual(out[60][60], 8)
        self.assertEqual(base[60][60], 8)
        self.assertEqual(counts["artifact_islands"], 1)

    def test_source_color_edge_guide_sharpens_only_protected_metal(self):
        # Synthetic 6-bit DAC edge: center metal is locally brighter.
        image = [[0]*320 for _ in range(200)]
        for y in range(40, 51):
            for x in range(45, 61):
                image[y][x] = 7
        out = [row[:] for row in image]
        # Colored lamps and the immediately adjacent gray border stay safe.
        image[45][56] = out[45][56] = 13
        rgb = bytearray(768)
        for i, level in ((1,18),(2,32),(3,56),(4,12),(5,50)):
            rgb[i*3:i*3+3] = bytes([level]*3)
        indices = bytearray([2]*64000)
        center = 48*320+51
        indices[center] = 3
        indices[center-1] = 1
        groove = 48*320+57
        indices[groove] = 4
        indices[groove-1] = 1
        indices[groove+1] = 5
        indices[groove-320] = 5
        indices[groove+320] = 5
        pack = b"VGA1" + bytes(rgb) + bytes(indices)
        counters = {}
        sharpen_metal_from_vga(image,out,pack,counters)
        self.assertEqual(out[48][51], 15)
        self.assertEqual(out[48][57], 8)
        self.assertEqual(out[45][56], 13)
        self.assertEqual(out[45][55], image[45][55])
        self.assertEqual(out[10][10], 0)
        self.assertGreater(counters["sharp_light"], 0)
        self.assertGreater(counters["sharp_dark"], 0)
        with self.assertRaises(ValueError):
            sharpen_metal_from_vga(image,out,pack[:-640],{})

    def test_enhanced_asset_guided_by_original_vga_colors(self):
        raw=(HERE/"data"/"TABLE.TGA").read_bytes()
        table=(HERE/"data"/"TABLE.DAT").read_bytes()
        vga=(HERE/"data"/"VGA.DAT").read_bytes()
        original=unpack_screen(raw)
        previous=generate(raw,table)[0]
        improved,frame,caps,stats=generate(raw,table,vga)
        self.assertEqual(len(improved),32768)
        self.assertEqual(bytes(pack_screen(frame)),improved)
        self.assertEqual(generate(raw,table,vga)[0],improved)
        self.assertNotEqual(previous,improved)
        self.assertEqual(len(caps),7)
        sharpened=sum(stats[k] for k in
                      ("sharp_light","sharp_dark","sharp_contours"))
        self.assertGreater(sharpened,0)
        self.assertLess(sharpened,5000)
        for y in range(200):
            for x in range(320):
                initial=original[y][x]
                result=frame[y][x]
                if initial not in (7,8,15) or x>=236 or y>=191:
                    self.assertEqual(initial,result)
                elif initial != result:
                    self.assertIn(result,(7,8,15))
        self.assertEqual((HERE/"data"/"TABLE.TGA").read_bytes(),raw)
        self.assertEqual((HERE/"data"/"TABLE.DAT").read_bytes(),table)
        self.assertEqual((HERE/"data"/"VGA.DAT").read_bytes(),vga)

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
                    # No black pinholes may be added to originally
                    # gray/white metal in the enhanced table.
                    self.assertIn(new, (7, 8, 15))
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
        self.assertGreater(changes["artifact_pits"], 0)
        self.assertGreater(changes["artifact_color_halos"], 0)
        self.assertGreater(changes["artifact_sparkles"], 0)
        self.assertLess(changed, 3000)
        self.assertNotEqual(raw, enhanced)
        self.assertEqual(generate(raw, pack)[0], enhanced)  # Reproducible.


if __name__ == "__main__":
    unittest.main()

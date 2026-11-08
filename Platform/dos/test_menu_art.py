"""Regression checks for the image-backed DOS menu in both video modes."""
import hashlib
from pathlib import Path
import tempfile
import unittest

import build_menu_art as art

HERE = Path(__file__).resolve().parent
HUD = (HERE / "hud.inc").read_text()
MAIN = (HERE / "main.c").read_text()
BUILD = (HERE / "build.py").read_text()


class TitleArtTests(unittest.TestCase):
    def test_packed_asset_dimensions_and_checksum(self):
        raw = art.decode_source()
        self.assertEqual(len(raw), 24000 + 2014)
        self.assertEqual(hashlib.sha256(raw).hexdigest(), art.EXPECTED_SHA256)
        self.assertEqual(art.BACKGROUND_BYTES, 120 * 200)
        self.assertEqual(art.LOGO_BYTES, 38 * 53)
        indices = [index for byte in raw for index in (byte >> 4, byte & 15)]
        self.assertTrue(all(0 <= index < 16 for index in indices))
        self.assertGreater(len(set(indices)), 8)
        self.assertNotEqual(raw[:120], raw[120*100:120*101])

    def test_asset_is_built_and_preview_has_valid_png(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = art.build(tmp)
            self.assertEqual(path.read_bytes(), b"SCM1" + art.decode_source())
            self.assertEqual(path.stat().st_size, 26018)
            png = (Path(tmp) / "MENU_PREVIEW.png").read_bytes()
            self.assertTrue(png.startswith(b"\x89PNG\r\n\x1a\n"))
            self.assertIn(b"IHDR", png[:32])

    def test_tandy_packed_pixels_equal_vga_expansion(self):
        raw = art.decode_source()
        # Both backends get identical palette indices. Tandy copies packed
        # bytes into interleaved B800h scanlines; VGA expands into A000h.
        for y in (0, 30, 100, 160, 199):
            packed = raw[y*120:(y+1)*120]
            vga = art.unpack_rgbi(packed, 240)
            tandy = [c for b in packed for c in (b >> 4, b & 15)]
            self.assertEqual(vga, tandy)
            self.assertEqual(len(vga), 240)

    def test_menu_is_a_static_scene_not_a_per_frame_upload(self):
        self.assertIn('load_menu_background();', MAIN)
        self.assertIn('draw_menu_background();', MAIN)
        self.assertIn('if(view) {', MAIN)
        self.assertIn('if(prior_view) {', MAIN)
        self.assertIn('restore(0,0,320,200);', MAIN)
        self.assertIn('if(!prior_view && menu_background)', MAIN)
        self.assertIn('if(!menu_background) return;', HUD)
        self.assertIn('video+address(0,y)', HUD)
        self.assertIn('video[a+2*x]', HUD)
        self.assertIn('MENU_BACKGROUND_BYTES+', HUD)
        self.assertIn('if(menu_background) _ffree(menu_background);', MAIN)
        self.assertIn("build_menu_art.py", BUILD)
        self.assertNotIn('fopen("MENU.DAT"', MAIN)


if __name__ == "__main__":
    unittest.main()

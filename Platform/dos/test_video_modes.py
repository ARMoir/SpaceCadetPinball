import unittest

from verify_video_modes import compare_frames, unpack_tandy, unpack_vga


class DualVideoTests(unittest.TestCase):
    def test_tandy_bank_and_nibble_order(self):
        tandy=bytearray(32768)
        vga=bytearray(64000)
        for y in range(200):
            for x in range(0,320,2):
                a=(x+y)%16
                b=(x+y+1)%16
                off=(y&3)*8192+(y>>2)*160+x//2
                tandy[off]=(a<<4)|b
                vga[y*320+x]=a
                vga[y*320+x+1]=b
        self.assertEqual(unpack_tandy(tandy),bytes(vga))
        self.assertEqual(compare_frames(tandy,vga),64000)

    def test_bad_dimensions_and_palette_fail(self):
        with self.assertRaises(ValueError):
            unpack_tandy(bytes(32767))
        with self.assertRaises(ValueError):
            unpack_vga(bytes(63999))
        vga=bytearray(64000)
        vga[17]=16
        with self.assertRaises(ValueError):
            unpack_vga(vga)

    def test_any_mismatch_fails(self):
        tandy=bytes(32768)
        vga=bytearray(64000)
        vga[1234]=7
        with self.assertRaises(AssertionError):
            compare_frames(tandy,vga)


if __name__=="__main__":
    unittest.main()

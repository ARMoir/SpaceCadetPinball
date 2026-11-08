import unittest
from pathlib import Path
from verify_vga_art import validate_pack


class VGAArtTests(unittest.TestCase):
    def test_source_pack_and_rejections(self):
        root=Path(__file__).parent/'data'
        vga=(root/'VGA.DAT').read_bytes();table=(root/'TABLE.DAT').read_bytes()
        validate_pack(vga,table)
        for damaged in (vga[:100],vga[:-1],vga+b'x',b'BAD!'+vga[4:]):
            with self.assertRaises(ValueError): validate_pack(damaged,table)
        for offset in (4,64772,64780):
            damaged=bytearray(vga)
            damaged[offset]=255 if offset!=64780 else (0 if damaged[offset]==255 else 255)
            with self.assertRaises(ValueError): validate_pack(damaged,table)


if __name__=='__main__': unittest.main()

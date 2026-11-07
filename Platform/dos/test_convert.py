import struct
import tempfile
import unittest
from pathlib import Path
from convert import Dat, pack_screen, segment
from make_demo import generate


def resource(groups):
    body=bytearray()
    for fields in groups:
        body.append(len(fields))
        for kind,value in fields:
            body.append(kind)
            if kind not in (0,2): body.extend(struct.pack('<I',len(value)))
            body.extend(value)
    header=struct.pack('<21s50s100sIHIH',b'PARTOUT(4.0)RESOURCE',b'test',b'fixture',183+len(body),len(groups),len(body),0)
    return header+body


class ImportTests(unittest.TestCase):
    def test_banks_and_nibbles(self):
        pixels=[[(x+y)%16 for x in range(320)] for y in range(200)]
        packed=pack_screen(pixels)
        self.assertEqual(len(packed),32768)
        for y in range(200):
            for x in range(320):
                b=packed[(y%4)*8192+(y//4)*160+x//2]
                self.assertEqual((b&15 if x%2 else b>>4),pixels[y][x])
        for bank in range(4):
            self.assertEqual(packed[bank*8192+8000:(bank+1)*8192],bytes(192))

    def test_bitmap_padding_orientation(self):
        bitmap=struct.pack('<BhhhhIB',0,3,2,137,2,8,2)+bytes([4,5,6,99,1,2,3,99])
        dat=Dat(resource([[(3,b'table\0'),(1,bitmap)]]))
        self.assertEqual(dat.named('table'),0)
        self.assertEqual(dat.bitmap(0),(3,2,137,2,[[1,2,3],[4,5,6]]))

    def test_truncated_and_unsupported(self):
        good=resource([[(3,b'table\0')]])
        for i in range(len(good)):
            with self.assertRaises(ValueError): Dat(good[:i])
        with self.assertRaises(ValueError): Dat(good+b'x')
        with self.assertRaises(ValueError): Dat(bytes(len(good)))
        with self.assertRaises(ValueError): Dat(resource([[(13,b'')]]))

    def test_attributes_and_normal(self):
        d=Dat(resource([[(11,struct.pack('<3f',700,183,238))]]))
        self.assertEqual(d.attr(0,700),(183,238))
        self.assertIsNone(d.attr(0,800))
        self.assertEqual(segment((10,10),(10,20)),(10,10,10,20,0,256,640,0,0))
        self.assertIsNone(segment((1,1),(1,1)))

    def test_demo_pack(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp); generate(p)
            self.assertEqual((p/'TABLE.TGA').stat().st_size,32768)
            data=(p/'TABLE.DAT').read_bytes()
            self.assertEqual(data[:4],b'SCP1')
            count=struct.unpack_from('<h',data,4)[0]
            pos=36+18*count
            for _ in range(5):
                x,y,w,h=struct.unpack_from('<4h',data,pos); pos+=8
                self.assertLessEqual(w*h,1024)
                self.assertTrue(all(v<=15 or v==255 for v in data[pos:pos+w*h]))
                pos+=w*h
            self.assertEqual(pos,len(data))


if __name__=='__main__': unittest.main()

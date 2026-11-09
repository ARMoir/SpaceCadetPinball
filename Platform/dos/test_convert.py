import struct
import tempfile
import unittest
from pathlib import Path
from convert import Dat, pack_screen, segment
from component_import import directional_line, ramp_path, pack_components, validate_extension
import json
import math
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

    def test_source_components_and_pack(self):
        root=Path(__file__).parent/'data'
        report=json.loads((root/'IMPORT.json').read_text())
        c=report['components']
        self.assertEqual(report['source_sha256'], '42defd5d2a339a3953fe643716aff4e35b2cea7a3e54505c82d1e78d5325bc04')
        self.assertEqual([(r['group'],r['plane_count'],r['assist']) for r in c['ramps']],[(101,18,1),(105,2,0)])
        self.assertEqual([v['group'] for v in c['oneways']],list(range(345,354)))
        self.assertEqual([v['mask'] for v in c['oneways']],[1]*7+[4,1])
        self.assertEqual([g['group'] for g in c['gates']],[501,502])
        body=pack_components(c)
        self.assertTrue((root/'TABLE.DAT').read_bytes().endswith(body))
        validate_extension(body)
        for n in range(1,len(body)):
            with self.assertRaises(ValueError): validate_extension(body[:n])
        with self.assertRaises(ValueError): validate_extension(body+b'x')
        # Gates are source lines; prior four-edge runtime proxies are elsewhere.
        self.assertLess(c['gates'][0]['projected'][0][0],60)
        self.assertGreater(c['gates'][1]['projected'][0][1],145)
        for ramp in c['ramps']:
            class Source:
                def attr(self, group, key, default=None):
                    if key==1300: return [ramp['plane_count']]+[v for p in ramp['planes'] for v in p]
                    if key in (1301,1302): return ramp['walls'][key-1301]
                    return default
                def field(self,*args): return b''
                def name(self,*args): return ramp['name']
            result=ramp_path(Source(),ramp['group'],lambda p:p)
            self.assertEqual(result['entry_attribute'],ramp['entry_attribute'])
            self.assertEqual(result['assist'],ramp['assist'])
            self.assertEqual(len(result['segments']),len(ramp['segments']))

    def test_direction_survives_reflected_projection(self):
        for project in (lambda p:p, lambda p:(20-p[0],30+p[1])):
            front=directional_line((2,2),(6,2),1,project)
            back=directional_line((6,2),(2,2),-0.8,project)
            self.assertEqual(front[5],-256)
            self.assertEqual(back[5],256)
            self.assertEqual(front[1],round(project((2,1))[1]*64))
            self.assertEqual(back[1],round(project((2,1.2))[1]*64))
            self.assertEqual(front[6],4*64)
        with self.assertRaises(ValueError): directional_line((1,1),(1,1),1,lambda p:p)

    def test_demo_pack(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp); generate(p)
            self.assertEqual((p/'TABLE.TGA').stat().st_size,32768)
            data=(p/'TABLE.DAT').read_bytes()
            self.assertEqual(data[:4],b'SCP2')
            count=struct.unpack_from('<h',data,4)[0]
            pos=44+18*count
            for _ in range(13):
                x,y,w,h=struct.unpack_from('<4h',data,pos); pos+=8
                self.assertLessEqual(w*h,1024)
                self.assertTrue(all(v<=15 or v==255 for v in data[pos:pos+w*h]))
                pos+=w*h
            self.assertEqual(pos,len(data))


if __name__=='__main__': unittest.main()

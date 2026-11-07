#!/usr/bin/env python3
"""Offline PARTOUT importer. No proprietary resources are included in this tree.

Format references: partman.h/cpp, GroupData.cpp, gdrv.cpp and proj.cpp.
Only the Windows PINBALL.DAT resolution-0 format is supported initially.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

PALETTE = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),
           (170,0,0),(170,0,170),(170,85,0),(170,170,170),
           (85,85,85),(85,85,255),(85,255,85),(85,255,255),
           (255,85,85),(255,85,255),(255,255,85),(255,255,255)]


class Dat:
    def __init__(self, data):
        self.data, self.pos, self.groups = data, 0, []
        header = self.take(183)
        if header[:21].rstrip(b'\0') != b'PARTOUT(4.0)RESOURCE':
            raise ValueError('Not a PARTOUT 4.0 resource')
        count = struct.unpack_from('<H', header, 175)[0]
        self.take(struct.unpack_from('<H', header, 181)[0])
        for _ in range(count):
            fields = []
            for _ in range(self.take(1)[0]):
                kind = self.take(1)[0]
                if kind > 12:
                    raise ValueError('Unsupported field type')
                size = 2 if kind in (0, 2) else struct.unpack('<I', self.take(4))[0]
                fields.append((kind, self.take(size)))
            self.groups.append(fields)
        if self.pos != len(data):
            raise ValueError('Unexpected trailing resource bytes')

    def take(self, size):
        if size < 0 or self.pos + size > len(self.data):
            raise ValueError('Truncated resource')
        value = self.data[self.pos:self.pos+size]
        self.pos += size
        return value

    def field(self, group, kind):
        return next((v for k, v in self.groups[group] if k == kind), None)

    def named(self, name):
        return next(i for i in range(len(self.groups))
                    if self.field(i, 3) == name.encode() + b'\0')

    def name(self, group):
        return (self.field(group, 3) or b'').rstrip(b'\0').decode('ascii', errors='replace')

    def attr(self, group, key, default=None, kind=11):
        fmt = 'f' if kind == 11 else 'h'
        size = 4 if kind == 11 else 2
        for k, v in self.groups[group]:
            if k == kind:
                if len(v) % size:
                    raise ValueError('Misaligned attribute')
                a = struct.unpack('<' + fmt * (len(v)//size), v)
                if a and math.floor(a[0]) == key:
                    return a[1:]
        return default

    def bitmap(self, group):
        for kind, raw in self.groups[group]:
            if kind != 1 or raw[0] != 0:
                continue
            res, w, h, x, y, size, flags = struct.unpack_from('<BhhhhIB', raw)
            if w <= 0 or h <= 0 or w*h > 1024*768 or size != len(raw)-14:
                raise ValueError('Invalid bitmap dimensions/size')
            raw = raw[14:]
            if flags & 4:
                raise ValueError('Full Tilt spliced sprites are not supported')
            stride = (w+3) & ~3
            if size != stride*h:
                raise ValueError('Invalid bitmap stride')
            # Original indexed resources are bottom-up (gdrv::ApplyPalette).
            pixels = [list(raw[row*stride:row*stride+w]) for row in reversed(range(h))]
            return w, h, x, y, pixels
        return None


def png(path, pixels):
    h, w = len(pixels), len(pixels[0])
    def chunk(kind, data):
        return struct.pack('>I', len(data))+kind+data+struct.pack('>I', zlib.crc32(kind+data))
    raw = b''.join(b'\0'+bytes(c for p in row for c in PALETTE[p]) for row in pixels)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+
                           chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))


def pack_screen(pixels):
    """BIOS mode 09h: four 8KB banks, high nibble is left pixel."""
    out = bytearray(32768)
    for y, row in enumerate(pixels):
        for x in range(0, 320, 2):
            out[(y & 3)*8192+(y >> 2)*160+(x >> 1)] = row[x]*16+row[x+1]
    return out


def inspect(dat):
    for i in range(len(dat.groups)):
        name = dat.name(i)
        bmp = dat.bitmap(i)
        attrs = [struct.unpack('<'+'f'*(len(v)//4),v) for k,v in dat.groups[i] if k==11]
        if name in ('table','camera_info','table_objects','plunger','flipper1','flipper2') or 'bump' in name:
            print(i, name, bmp[:4] if bmp else None, attrs)


def segment(a, b, score=0, boost=0):
    x, y = a
    ex, ey = b
    length = math.hypot(ex-x, ey-y)
    if length < .5:
        return None
    return tuple(round(v) for v in (x,y,ex,ey,(ex-x)*256/length,
                                    (ey-y)*256/length,length*64,score,boost))


def export(dat, output):
    output.mkdir(parents=True, exist_ok=True)
    table = dat.named('table')
    w,h,ox,oy,base = dat.bitmap(table)
    sy = 188/h
    sx = sy*1.2  # 320x200 pixels on a 4:3 display are 5:6, not square.
    left, top = round((232-w*sx)/2), 4
    palette = dat.field(dat.named('background'),5)
    if palette is None or len(palette) != 1024:
        raise ValueError('Missing 256-color palette')
    rgb = [tuple(palette[i+2:i+3]+palette[i+1:i+2]+palette[i:i+1]) for i in range(0,1024,4)]
    rgb[:10] = [(0,0,0),(128,0,0),(0,128,0),(128,128,0),(0,0,128),
                (128,0,128),(0,128,128),(192,192,192),(192,220,192),(166,202,240)]
    rgb[246:255] = [(0,0,0)]*9
    rgb[255] = (255,255,255)
    def nearest(color):
        return min(range(16), key=lambda i: sum((color[c]-PALETTE[i][c])**2 for c in range(3)))
    lut = [nearest(c) for c in rgb]
    objects = dat.attr(dat.named('table_objects'),1025,kind=10)
    objects = list(zip(objects[::2],objects[1::2]))
    # Composite initial-state art, excluding moving flippers/plunger. Static wall
    # sprites are impact flashes and must not be baked into the background.
    for kind, group in objects:
        if kind not in (1002,1005,1006,1011,1012,1014,1017,1018,1019,1029):
            continue
        bmp = dat.bitmap(group)
        if not bmp:
            continue
        bw,bh,bx,by,pix = bmp
        for y in range(bh):
            for x in range(bw):
                xx,yy = bx-ox+x,by-oy+y
                if 0 <= xx < w and 0 <= yy < h and pix[y][x] != 0:
                    base[yy][xx] = pix[y][x]
    pixels = [[0]*320 for _ in range(200)]
    # Box-filter before palette reduction; dithering is intentionally omitted
    # to keep the tiny ball and playfield features readable in 16 colors.
    for y in range(round(h*sy)):
        for x in range(round(w*sx)):
            samples = [rgb[base[yy][xx]] for yy in range(int(y/sy),min(h,max(int(y/sy)+1,int((y+1)/sy))))
                       for xx in range(int(x/sx),min(w,max(int(x/sx)+1,int((x+1)/sx))))]
            pixels[top+y][left+x] = nearest(tuple(sum(p[c] for p in samples)//len(samples) for c in range(3)))
    camera = struct.unpack('<15f',dat.field(dat.named('camera_info'),11))
    center = dat.attr(table,700)
    def project(p):
        x,y = p[:2]
        z = 0  # All collision geometry is flattened to the table plane.
        v = [camera[i]*x+camera[i+1]*y+camera[i+2]*z+camera[i+3] for i in (0,4,8)]
        return (left+(v[0]*camera[12]/v[2]+center[0])*sx,
                top+(v[1]*camera[12]/v[2]+center[1])*sy)
    edges, imported, omitted = [], [], []
    for kind, group in objects:
        wall = dat.attr(group,600)
        # Ground-level static walls and bumpers only. Upper-level ramp rails
        # cannot be flattened without creating invisible barriers.
        attrs = dat.field(group,10) or b''
        shorts = struct.unpack('<'+'h'*(len(attrs)//2),attrs)
        upper = any(shorts[i]==602 and shorts[i+1]!=0 for i in range(0,len(shorts)-1,2))
        if kind not in (1000,1010,1005) or not wall or (upper and kind!=1005):
            if wall:
                omitted.append({'group':group,'name':dat.name(group),'type':kind})
            continue
        shape = math.floor(wall[0])
        score = (500 if group < 405 else 1500) if kind==1005 else 0
        boost = 35 if kind==1005 else 0
        if shape == 1 and kind == 1005:
            # With no elevation/occlusion, the source's small stem collider
            # can sit below/outside the visible cap. Flatten every bumper
            # layer to its actual sprite footprint so visible caps are solid.
            bw,bh,bx,by,_ = dat.bitmap(group)
            cx=left+(bx-ox+bw/2)*sx
            cy=top+(by-oy+bh/2)*sy
            # Approximate the visible cap with an eight-sided outline.
            rx,ry=bw*sx*.42,bh*sy*.42
            pts=[(cx+rx*math.cos(i*math.tau/8),cy+ry*math.sin(i*math.tau/8)) for i in range(9)]
        elif shape == 1:
            # Eight-sided bumper outline preserves source center/radius.
            pts = [project((wall[1]+wall[3]*math.cos(i*math.tau/8),
                            wall[2]+wall[3]*math.sin(i*math.tau/8))) for i in range(9)]
        elif shape == 2:
            pts = [project(wall[1:3]),project(wall[3:5])]
        else:
            n = shape-1
            pts = [project(wall[1+i*2:3+i*2]) for i in range(n+1)]
        for a,b in zip(pts,pts[1:]):
            edge = segment(a,b,score,boost)
            if edge and all(-32 <= v <= 350 for v in edge[:4]):
                edges.append(edge)
        imported.append({'group':group,'name':dat.name(group),'type':kind})
    flippers, sprites = [], []
    for kind in (1003,1004):
        group = next(g for k,g in objects if k==kind)
        points = [project(dat.attr(group,a)) for a in (800,801,802)]
        flippers.extend(round(v) for point in points for v in point)
        states = int(dat.attr(group,100,kind=10)[0])
        for index in (0,states-1):
            bw,bh,bx,by,pix = dat.bitmap(group+index)
            sw,sh = max(1,round(bw*sx)),max(1,round(bh*sy))
            data = bytes(255 if pix[min(bh-1,int(y/sy))][min(bw-1,int(x/sx))]==0 else
                         lut[pix[min(bh-1,int(y/sy))][min(bw-1,int(x/sx))]] for y in range(sh) for x in range(sw))
            sprites.append((round(left+(bx-ox)*sx),round(top+(by-oy)*sy),sw,sh,data))
    group = dat.named('ball')
    bw,bh,bx,by,pix = dat.bitmap(group)
    sw,sh = max(3,round(bw*sx)),max(3,round(bh*sy))
    data = bytes(255 if pix[min(bh-1,y*bh//sh)][min(bw-1,x*bw//sw)]==0 else
                 lut[pix[min(bh-1,y*bh//sh)][min(bw-1,x*bw//sw)]] for y in range(sh) for x in range(sw))
    sprites.append((0,0,sw,sh,data))
    launch = project(dat.attr(next(g for k,g in objects if k==1001),601))
    drain = project(dat.attr(next(g for k,g in objects if k==1007),600)[1:3])[1]
    plunger_group=next(g for k,g in objects if k==1001)
    plunger=[]
    for index in range(8):
        bw,bh,bx,by,pix=dat.bitmap(plunger_group+index)
        sw,sh=max(1,round(bw*sx)),max(1,round(bh*sy))
        data=bytes(255 if pix[min(bh-1,y*bh//sh)][min(bw-1,x*bw//sw)]==0 else
                   lut[pix[min(bh-1,y*bh//sh)][min(bw-1,x*bw//sw)]] for y in range(sh) for x in range(sw))
        plunger.append((round(left+(bx-ox)*sx),round(top+(by-oy)*sy),sw,sh,data))
    kicks=[]
    for kind,group in objects:
        if kind==1014:
            wall=dat.attr(group,600)
            a,b=project(wall[1:3]),project(wall[3:5])
            kicks.append((round((a[0]+b[0])/2),round((a[1]+b[1])/2)))
    kicks.sort()
    if len(kicks)!=2:
        raise ValueError('Expected two Space Cadet kickbacks')
    write_pack(output,pixels,edges,flippers,launch,drain,sprites,plunger,kicks)
    report = {'source_sha256':hashlib.sha256(dat.data).hexdigest(), 'source':'Windows PINBALL.DAT',
              'segments':len(edges),'imported_collision_objects':imported,'omitted_collision_objects':omitted,
              'simplifications':['Projected 2D fixed-point physics; not original world-space integrator',
              'Static ground walls; all seven bumper layers flattened to visible sprite caps; two-state flipper animation',
              'No ramp elevation, missions, multiball, target progression, or original audio'],
              'flippers':flippers,'launch':launch,'drain_y':drain,'kickbacks':kicks,
              'plunger_frames':[list(s[:4]) for s in plunger]}
    (output/'IMPORT.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Imported {len(edges)} segments; {len(omitted)} collision objects omitted. See IMPORT.json.')


def write_pack(output,pixels,edges,flippers,launch,drain,sprites,plunger=None,kicks=None):
    if len(edges)>640:
        raise ValueError('Too many collision segments (limit 640)')
    if plunger is None:  # Diagnostic table, with no commercial sprites.
        plunger=[(round(launch[0])-1,round(launch[1])+4+i//2,3,3,bytes([7]*9)) for i in range(8)]
    if kicks is None:
        kicks=[(flippers[0]-25,round(drain)-24),(flippers[6]+25,round(drain)-24)]
    if len(sprites)!=5 or len(plunger)!=8 or len(kicks)!=2:
        raise ValueError('SCP2 requires five moving sprites, eight plunger frames and two kickbacks')
    body = b'SCP2'+struct.pack('<4h12h4h',len(edges),round(launch[0]),round(launch[1]),round(drain),*flippers,
                             *(v for point in kicks for v in point))
    body += b''.join(struct.pack('<9h',*edge) for edge in edges)
    for x,y,w,h,data in sprites+plunger:
        if w*h > 1024:
            raise ValueError('Sprite exceeds 1024-byte limit')
        body += struct.pack('<4h',x,y,w,h)+data
    (output/'TABLE.DAT').write_bytes(body)
    (output/'TABLE.TGA').write_bytes(pack_screen(pixels))  # Tandy bank image, NOT a Targa file.
    png(output/'PREVIEW.png',pixels)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dat', type=Path)
    parser.add_argument('--inspect', action='store_true')
    parser.add_argument('--output', type=Path, default=Path('build/dos'))
    args = parser.parse_args()
    dat = Dat(args.dat.read_bytes())
    if args.inspect:
        inspect(dat)
    else:
        export(dat,args.output)

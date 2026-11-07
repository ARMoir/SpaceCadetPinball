#!/usr/bin/env python3
"""Generate an original, freely redistributable diagnostic table, NOT Space Cadet."""
from pathlib import Path
import argparse
import math
from convert import segment, write_pack


def generate(output):
    output.mkdir(parents=True,exist_ok=True)
    pixels = [[0]*320 for _ in range(200)]
    edges=[]
    def line(a,b,score=0,boost=0):
        edges.append(segment(a,b,score,boost))
        n=max(abs(b[0]-a[0]),abs(b[1]-a[1]),1)
        for i in range(n+1):
            x=round(a[0]+(b[0]-a[0])*i/n)
            y=round(a[1]+(b[1]-a[1])*i/n)
            pixels[y][x]=11 if score else 9
    for a,b in [((30,185),(30,20)),((30,20),(50,8)),((50,8),(190,8)),
                ((190,8),(205,20)),((205,20),(205,185)),
                ((190,35),(190,180)),((30,140),(80,160)),((150,160),(185,140))]:
        line(a,b)
    for cx,cy in [(80,55),(130,45),(110,85)]:
        pts=[(round(cx+8*math.cos(i*math.tau/8)),round(cy+8*math.sin(i*math.tau/8))) for i in range(9)]
        for a,b in zip(pts,pts[1:]): line(a,b,500,35)
    flippers=[80,160,105,172,105,150,150,160,125,172,125,150]
    sprites=[]
    for i in range(2):
        x,y,dx,dy,ux,uy=flippers[i*6:i*6+6]
        for ex,ey in [(dx,dy),(ux,uy)]:
            left,top=min(x,ex)-1,min(y,ey)-1
            w,h=abs(x-ex)+3,abs(y-ey)+3
            data=bytearray([255])*(w*h)
            for t in range(101):
                px=round(x+(ex-x)*t/100)-left
                py=round(y+(ey-y)*t/100)-top
                for yy in range(py-1,py+2):
                    for xx in range(px-1,px+2): data[yy*w+xx]=14
            sprites.append((left,top,w,h,bytes(data)))
    sprites.append((0,0,3,3,bytes([255,15,255,15,15,7,255,7,255])))
    write_pack(output,pixels,edges,flippers,(198,170),190,sprites)
    (output/'DEMO.TXT').write_text('Diagnostic table: original test artwork, not Space Cadet.\n')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('build/dos'))
    generate(p.parse_args().output)

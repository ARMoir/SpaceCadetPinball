#!/usr/bin/env python3
"""Offline faux-3D lighting for the *existing* 320x200 Tandy playfield.

Input: TABLE.TGA, a 32768-byte interleaved 4-bit SCREEN (not a Targa image),
and the matching SCP2 TABLE.DAT. Output: TABLE3D.TGA of the *same* format.

Does not alter the original TABLE.TGA, TABLE.DAT, sprite pixels, positions,
physics, hitboxes, palette or screen dimensions. All light/shadow work runs
on a PC in the asset build, not on the V20 during gameplay.
"""
import argparse
from pathlib import Path
import struct
from convert import pack_screen, png

WIDTH, HEIGHT = 320, 200
PLAYFIELD_RIGHT = 236
PLAYFIELD_BOTTOM = 191
NEUTRAL = (7, 8, 15)


def unpack_screen(data):
    """Reverse mode 09h banked nibbles without reading the unused tail bytes."""
    if len(data) != 32768:
        raise ValueError("Tandy image must contain exactly 32768 bytes")
    return [[(data[(y & 3) * 8192 + (y >> 2) * 160 + (x >> 1)] >>
              (0 if x & 1 else 4)) & 15 for x in range(WIDTH)]
            for y in range(HEIGHT)]


def bumper_caps(pack):
    """Derive seven circular cap bounds from original-source scored octagons.

    This is visual metadata only. The output can NEVER modify these edges.
    """
    if pack[:4] != b"SCP2":
        raise ValueError("Expected original SCP2 table geometry")
    count = struct.unpack_from("<h", pack, 4)[0]
    if count < 0 or count > 640 or 44 + count * 18 > len(pack):
        raise ValueError("Invalid SCP2 segment array")
    edges = [struct.unpack_from("<9h", pack, 44 + i * 18)
             for i in range(count)]
    result = []
    i = 0
    while i + 7 < count:
        run = edges[i:i+8]
        if (run[0][7] > 0 and
                all(e[7:9] == run[0][7:9] and
                    e[2:4] == run[(j+1) % 8][:2]
                    for j, e in enumerate(run))):
            xs = [x for e in run for x in (e[0], e[2])]
            ys = [y for e in run for y in (e[1], e[3])]
            cx = (min(xs) + max(xs)) / 2
            cy = (min(ys) + max(ys)) / 2
            rx = max(2, (max(xs)-min(xs))/2)
            ry = max(2, (max(ys)-min(ys))/2)
            result.append((cx,cy,rx,ry))
            i += 8
        else:
            i += 1
    return result


def _neutral(src, x, y):
    return src[y][x] in NEUTRAL


def enhance(original, caps):
    """Deterministic 16-color lighting, restricted to existing neutral pixels.

    The source's colored bulbs, labels and mechanisms are left pixel-identical.
    Highlights/shadows follow a fixed top-left light source and lower-right
    projected shadows. No perspective warp or geometric transformation.
    """
    if len(original) != HEIGHT or any(len(row) != WIDTH for row in original):
        raise ValueError("Expected 320x200 indexed playfield")
    src = original
    out = [row[:] for row in src]
    counters = {"edge_highlight":0,"edge_shadow":0,
                "bumper_highlight":0,"bumper_shadow":0,"cast_shadow":0}

    # Grayscale raised rail/ramp outlines: emphasize their existing upper-left
    # rim and darken their lower-right rim, but only if the neighboring pixels
    # indicate an actual visible neutral-gray surface. Never draw collision
    # edges: some imported edges are invisible or on another table layer.
    for y in range(6, PLAYFIELD_BOTTOM):
        for x in range(6, PLAYFIELD_RIGHT):
            c = src[y][x]
            if c == 7:
                # Existing pale steel side facing the light.
                if (src[y-1][x] in (0, 8) and
                    src[y][x-1] in (0, 8) and
                    (_neutral(src,x+1,y) or _neutral(src,x,y+1)) and
                    ((x+y) & 1) == 0):
                    out[y][x] = 15
                    counters["edge_highlight"] += 1
                elif (src[y+1][x] in (0, 8) and
                      src[y][x+1] in (0, 8) and
                      (_neutral(src,x-1,y) or _neutral(src,x,y-1)) and
                      ((x+y) & 3) != 0):
                    out[y][x] = 8
                    counters["edge_shadow"] += 1
            elif c == 8:
                # Spare low-level highlight on the boundary of a substantial
                # darker rail, not randomly across the dotted floor.
                if (src[y-1][x] == 0 and src[y][x-1] == 0 and
                    _neutral(src,x+1,y) and _neutral(src,x,y+1) and
                    ((x+y) & 3) == 0):
                    out[y][x] = 7
                    counters["edge_highlight"] += 1

    # Round cap lighting: preserve original colorful rings but emphasize the
    # gray/metallic perimeter. The source cap center and extent are imported
    # from the SAME TABLE.DAT used by the collision engine.
    for cx,cy,rx,ry in caps:
        xmin=max(5,int(cx-rx-3))
        xmax=min(PLAYFIELD_RIGHT,int(cx+rx+4))
        ymin=max(5,int(cy-ry-3))
        ymax=min(PLAYFIELD_BOTTOM,int(cy+ry+4))
        for y in range(ymin,ymax):
            for x in range(xmin,xmax):
                nx=(x-cx)/rx
                ny=(y-cy)/ry
                d2=nx*nx+ny*ny
                c=src[y][x]
                # Thin curved bevel around visible cap face.
                if .48 <= d2 <= 1.40 and c in NEUTRAL:
                    if nx+ny < -.30 and c in (7,8):
                        new = 15 if c==7 else 7
                        if ((x+y)&1)==0 and out[y][x]!=new:
                            out[y][x]=new
                            counters["bumper_highlight"]+=1
                    elif nx+ny > .35 and c in (7,15):
                        if out[y][x]!=8:
                            out[y][x]=8
                            counters["bumper_shadow"]+=1
                # A short, staggered shadow beneath the cap, down/right;
                # restrict to neutral background and outside the cap itself.
                sx=(x-cx-2)/(rx+1.2)
                sy=(y-cy-2)/(ry+1.2)
                if (.78 <= sx*sx+sy*sy <= 1.35 and
                    nx*nx+ny*ny >= .90 and c in (7,8) and
                    ((x+y)&1)==0):
                    new = 8 if c==7 else 0
                    if out[y][x]!=new:
                        out[y][x]=new
                        counters["cast_shadow"]+=1

    # One-pixel rail side-shadow based on ACTUAL painted pale-steel surface
    # instead of guessed collision edges. A muted checker makes the shadow
    # less like a black outline in 16 colors.
    for y in range(7, PLAYFIELD_BOTTOM-3):
        for x in range(7, PLAYFIELD_RIGHT-3):
            if src[y][x] not in (7,15):
                continue
            if (src[y+1][x] in (0,8) and src[y][x+1] in (0,8)
                and src[y+2][x+2] == 8 and ((x+y)&3)==0
                and out[y+2][x+2]==8):
                out[y+2][x+2]=0
                counters["cast_shadow"]+=1
    return out, counters


def generate(original, pack):
    pixels=unpack_screen(original)
    caps=bumper_caps(pack)
    if not caps:
        raise ValueError("No source bumper caps found in SCP2 table")
    shaded,counts=enhance(pixels,caps)
    result=bytes(pack_screen(shaded))
    if len(result)!=len(original):
        raise AssertionError("Unexpected Tandy image length")
    return result,shaded,caps,counts


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--table",type=Path,required=True,help="Unmodified TABLE.TGA")
    p.add_argument("--pack",type=Path,required=True,help="Matching TABLE.DAT")
    p.add_argument("--output",type=Path,required=True,help="Enhanced TABLE3D.TGA")
    p.add_argument("--preview",type=Path,help="Optional PNG preview")
    a=p.parse_args()
    old=a.table.read_bytes()
    data,shaded,caps,counts=generate(old,a.pack.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_bytes(data)
    if a.preview:
        a.preview.parent.mkdir(parents=True,exist_ok=True)
        png(a.preview,shaded)
    print("Tandy 16-color static lighting: %d bumper caps, %d bytes; %s" %
          (len(caps),len(data),counts))


if __name__=="__main__":
    main()

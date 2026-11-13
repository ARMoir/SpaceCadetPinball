#!/usr/bin/env python3
"""Offline RGBI source-table reconstruction for the Tandy 1000 (F-007).

WolfensteinCGA's cgaify tool inspired the palette-pair search, but no code
is copied. Read Cadet's original-source VGA1 color image and generate a
prepacked 320x200, 16-color Tandy 09h screen. Two-color patterns approximate
source midtones on broad surfaces; single-pixel contours and small bright
details remain solid. The 7 MHz V20 does no palette matching or dithering.

TABLESRC.TGA is a raw 32768-byte Tandy bank image, NOT a Targa file.
The original TABLE.TGA and enhanced TABLE3D.TGA are never modified.
"""
import argparse
from pathlib import Path

from convert import PALETTE, pack_screen, png
from enhance_table import unpack_screen
from verify_vga_art import validate_pack

WIDTH, HEIGHT = 320, 200
RIGHT, BOTTOM = 236, 191
RGBI_COUNT = 16
# Reject very different pair members: CRT checkerboard is not a free
# substitute for actual colors, especially on narrow pinball rails.
MAX_PAIR_DISTANCE = 41000
MAX_PAIR_LUMA_DELTA = 88
# A pair must improve average source color enough to justify its 2x2 pattern.
MIN_PAIR_GAIN = 440
PAIR_DISTANCE_COST = 1  # Penalize high-frequency contrast by distance / 20.
EDGE_DISTANCE = 2700

def distance(a, b):
    return sum((a[k] - b[k])**2 for k in range(3))

def luma(rgb):
    return (54*rgb[0] + 183*rgb[1] + 19*rgb[2]) >> 8

def decode_vga(vga, table):
    """Validate exact VGA1/SCP2 compatibility before using source colors."""
    validate_pack(vga, table)
    palette = [tuple(channel*255//63 for channel in vga[4+i*3:7+i*3])
               for i in range(252)]
    image = vga[772:64772]
    if len(image) != 64000 or any(i >= 252 for i in image):
        raise ValueError("VGA1 has invalid opaque source indices")
    return palette, image

def palette_choices(palette):
    """WolfensteinCGA-style weighted search for 16 solids or two RGBI colors.

    Midpoint colors are computed OFFLINE and never uploaded as new colors.
    A pair alternates its two genuine Tandy indices on a fixed 2x2 grid.
    """
    pairs = []
    for first in range(RGBI_COUNT):
        for second in range(first+1, RGBI_COUNT):
            a, b = PALETTE[first], PALETTE[second]
            spread = distance(a, b)
            if spread > MAX_PAIR_DISTANCE or abs(luma(a)-luma(b)) > MAX_PAIR_LUMA_DELTA:
                continue
            midpoint = tuple((a[i]+b[i])//2 for i in range(3))
            pairs.append((first, second, midpoint, spread))

    solids, choices = [], []
    for rgb in palette:
        solid = min(range(RGBI_COUNT),
                    key=lambda c: (distance(rgb,PALETTE[c]),c))
        base_error = distance(rgb, PALETTE[solid])
        best = None
        best_error = base_error
        for first, second, middle, spread in pairs:
            # A weighted preference for close colors limits checkerboard
            # noise, even when a contrasting pair has a perfect average.
            error = distance(rgb,middle) + spread//20
            if error + MIN_PAIR_GAIN < best_error:
                best_error = error
                best = (first, second)
        solids.append(solid)
        choices.append(best)
    return solids, choices

def source_art(vga, table, original):
    """Return raw packed board, pixel grid and build-time diagnostics."""
    palette, image = decode_vga(vga, table)
    if len(original) != 32768:
        raise ValueError("Original Tandy table must have exactly 32768 bytes")
    classic = unpack_screen(original)
    solids, pairs = palette_choices(palette)
    # Source-color RGB pixels are already the original-table projection at
    # 320x200. Do NOT scale or smooth the image a second time.
    output = [row[:] for row in classic]
    counts = dict(solid=0, patterned=0, protected_edges=0,
                  recovered_dark=0, original_black=0)
    for y in range(BOTTOM):
        offset = y*WIDTH
        for x in range(RIGHT):
            src_index = image[offset+x]
            rgb = palette[src_index]
            base = classic[y][x]
            if luma(rgb) <= 10:
                # Preserve true black outlines and the playfield silhouette.
                output[y][x] = 0
                counts["original_black"] += 1
                continue
            # Keep source-black one-pixel outlines if this is a near-black
            # area rather than an accidentally black-quantized colored rail.
            if base == 0 and luma(rgb) < 19:
                output[y][x] = 0
                counts["original_black"] += 1
                continue
            solid = solids[src_index]
            pair = pairs[src_index]
            if pair:
                # Don't pepper source-text, colored lights and fine rails
                # with checkerboard pixels. All four neighbors must have
                # similar source color (not just similar luminance).
                if (x==0 or y==0 or x+1>=RIGHT or y+1>=BOTTOM or
                    any(distance(rgb,palette[image[offset+dy*WIDTH+x+dx]])
                        > EDGE_DISTANCE
                        for dx,dy in ((0,-1),(0,1),(-1,0),(1,0)))):
                    counts["protected_edges"] += 1
                    pair = None
            if pair:
                output[y][x] = pair[(x+y)&1]
                counts["patterned"] += 1
            else:
                output[y][x] = solid
                counts["solid"] += 1
            if base == 0 and output[y][x] != 0:
                counts["recovered_dark"] += 1
    # Panel (240..319), right-hand margin and bottom rows are bit-exact
    # classic pixels; source and moving game sprites are not affected.
    raw = bytes(pack_screen(output))
    if len(raw) != 32768:
        raise AssertionError("Tandy source art must be exactly 32768 bytes")
    return raw, output, counts

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--vga",type=Path,required=True,help="Source-color VGA.DAT")
    p.add_argument("--pack",type=Path,required=True,help="Matching TABLE.DAT")
    p.add_argument("--classic",type=Path,required=True,help="Untouched TABLE.TGA")
    p.add_argument("--output",type=Path,required=True,help="Output TABLESRC.TGA")
    p.add_argument("--preview",type=Path,help="Optional RGBI source-art PNG")
    args=p.parse_args()
    raw, pixels, counts = source_art(args.vga.read_bytes(),
                                     args.pack.read_bytes(),
                                     args.classic.read_bytes())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(raw)
    if args.preview:
        args.preview.parent.mkdir(parents=True,exist_ok=True)
        png(args.preview,pixels)
    print("F-007 source-color Tandy RGBI: %d bytes; %s" % (len(raw),counts))

if __name__ == "__main__":
    main()

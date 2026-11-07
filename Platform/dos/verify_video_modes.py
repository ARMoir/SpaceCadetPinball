#!/usr/bin/env python3
"""Pixel-perfect regression between Tandy 09h and VGA 13h final frames.

Both backends must display the same 16-color source art, HUD, ball, and
flipper sprites. This script does NOT assert hardware performance parity.
"""
from pathlib import Path
import argparse

WIDTH, HEIGHT = 320, 200


def unpack_tandy(data):
    if len(data) != 32768:
        raise ValueError("Expected a 32768-byte four-bank Tandy capture")
    return bytes(
        ((data[(y & 3) * 8192 + (y >> 2) * 160 + (x >> 1)] >>
          (0 if x & 1 else 4)) & 15)
        for y in range(HEIGHT) for x in range(WIDTH)
    )


def unpack_vga(data):
    if len(data) != 64000:
        raise ValueError("Expected a 64000-byte linear VGA mode-13h capture")
    if any(c > 15 for c in data):
        raise ValueError("VGA output unexpectedly uses colors outside RGBI 0..15")
    return data


def compare_frames(tandy, vga):
    a, b = unpack_tandy(tandy), unpack_vga(vga)
    if a != b:
        differs = [i for i, (x, y) in enumerate(zip(a,b)) if x != y]
        coords = [(i % WIDTH, i // WIDTH, a[i], b[i]) for i in differs[:12]]
        raise AssertionError("Tandy/VGA frames differ at %d pixels, examples %s" %
                             (len(differs),coords))
    return len(a)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("directory",nargs="?",type=Path,default=Path("build/dos"))
    args=p.parse_args()
    count=compare_frames((args.directory/"SCREEN-TANDY.BIN").read_bytes(),
                         (args.directory/"SCREEN-VGA.BIN").read_bytes())
    print("PASS Tandy09/VGA13 frame parity: %d pixels identical, 16-color indexed" % count)


if __name__=="__main__":
    main()

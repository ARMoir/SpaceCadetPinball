"""Validate VGA1 against SCP2 dimensions/transparency and smoke gameplay."""
import argparse
from pathlib import Path
import struct
from convert import PALETTE
from component_import import validate_extension
from run_tests import validate_smoke


def validate_pack(vga, table):
    if vga[:4]!=b"VGA1" or table[:4]!=b"SCP2":
        raise ValueError("Bad VGA1/SCP2 header")
    if len(vga)<64772 or any(c>63 for c in vga[4:772]):
        raise ValueError("Truncated VGA image or invalid DAC palette")
    if vga[4:52]!=bytes(c*63//255 for rgb in PALETTE for c in rgb):
        raise ValueError("HUD palette changed")
    if len(set(vga[772:64772]))<=16 or 255 in vga[772:64772]:
        raise ValueError("Expected source-color opaque background")
    vp,tp=64772,44+struct.unpack_from('<h',table,4)[0]*18
    for i in range(13):
        if vp+8>len(vga) or tp+8>len(table): raise ValueError("Missing sprite")
        a=struct.unpack_from('<4h',vga,vp)
        b=struct.unpack_from('<4h',table,tp)
        if a!=b or not 0<a[2]*a[3]<=1024: raise ValueError("Sprite alignment changed")
        size=a[2]*a[3]; vp+=8; tp+=8
        x,y=vga[vp:vp+size],table[tp:tp+size]
        if len(x)!=size or len(y)!=size: raise ValueError("Truncated sprite")
        if [c==255 for c in x]!=[c==255 for c in y]:
            raise ValueError("Sprite transparency changed")
        vp+=size; tp+=size
    if vp!=len(vga): raise ValueError("Trailing data")
    validate_extension(table[tp:])


def compare_gameplay(directory):
    def counters(name):
        text=(directory/name).read_text()
        line=next(s for s in text.splitlines() if s.startswith('smoke_gameplay='))
        return validate_smoke('PASS '+line.split('=',1)[1],True)
    a=counters('RUN_VGA_SUMMARY.TXT')
    b=counters('RUN_VGA_COLOR_SUMMARY.TXT')
    # Timing/appearance may differ; every simulation/input counter must match.
    for key in a.keys()|b.keys():
        if key not in ('slow_frames','vga_colors') and a.get(key)!=b.get(key):
            raise ValueError('VGA graphics changed gameplay counter: '+key)
    print('PASS: source-color and RGBI VGA gameplay counters match')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--runs',type=Path);args=p.parse_args()
    root=Path(__file__).parent/'data'
    validate_pack((root/'VGA.DAT').read_bytes(),(root/'TABLE.DAT').read_bytes())
    print('PASS: source-color VGA pack, HUD palette, thirteen sprite bounds/masks')
    if args.runs: compare_gameplay(args.runs)

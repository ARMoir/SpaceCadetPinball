#!/usr/bin/env python3
"""Check MZ real-mode target and static memory budget (not a timing claim)."""
from pathlib import Path
import re
import struct


def verify(directory):
    for name in ('CADET','TEST'):
        data=(directory/(name+'.EXE')).read_bytes()
        assert data[:2]==b'MZ', 'Not a DOS MZ executable'
        words=struct.unpack_from('<14H',data)
        _,last,pages,relocs,header,minalloc,maxalloc,ss,sp,_,ip,cs,relocoff,_=words
        file_size=pages*512 if not last else (pages-1)*512+last
        assert file_size==len(data), 'Unexpected overlay or appended extender'
        assert header*16<=len(data) and relocoff+relocs*4<=header*16
        assert ss*16+sp < 128*1024 and cs==0 and ip<len(data)
        image=len(data)-header*16
        base=image+minalloc*16+256
        tandy_required=base+32768
        vga_required=base+64000
        # Mouse/HUD/high-score additions exceed the original 128KB guard.
        # Keep a bounded 160KB minimum-load budget; this is not peak heap
        # usage or proof of real-hardware performance on the 640KB target.
        assert tandy_required<160*1024, 'Tandy 09h load + 32KB background exceeds 160KB'
        assert vga_required<192*1024, 'VGA 13h load + 64KB background exceeds 192KB'
        map_text=(directory/(name+'.MAP')).read_text()
        dgroup=int(re.search(r'DGROUP\s+[0-9a-fA-F]+:0000\s+([0-9a-fA-F]+)',map_text).group(1),16)
        assert dgroup<=65536
        assert 'lib386' not in map_text and 'clib3' not in map_text
        print(f'{name}: MZ {len(data)} bytes; DGROUP {dgroup}; '
              f'Tandy09 minimum {tandy_required} bytes; '
              f'VGA13 minimum {vga_required} bytes (not peak heap)')


if __name__=='__main__': verify(Path(__file__).resolve().parents[2]/'build/dos')

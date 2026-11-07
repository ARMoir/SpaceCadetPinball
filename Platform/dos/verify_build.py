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
        required=image+minalloc*16+256+32768
        assert required<128*1024, 'Minimum load + PSP + background exceeds 128KB'
        map_text=(directory/(name+'.MAP')).read_text()
        dgroup=int(re.search(r'DGROUP\s+[0-9a-fA-F]+:0000\s+([0-9a-fA-F]+)',map_text).group(1),16)
        assert dgroup<=65536
        assert 'lib386' not in map_text and 'clib3' not in map_text
        print(f'{name}: MZ {len(data)} bytes; DGROUP {dgroup}; minimum image/PSP/background {required} bytes')


if __name__=='__main__': verify(Path(__file__).resolve().parents[2]/'build/dos')

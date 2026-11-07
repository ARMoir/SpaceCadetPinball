#!/usr/bin/env python3
"""Run real-mode binaries in DOSBox-X; fail if DOS tests don't leave markers."""
import argparse
import os
from pathlib import Path
import subprocess
from convert import png


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--emulator',required=True,type=Path)
    p.add_argument('--directory',type=Path,default=Path('build/dos'))
    p.add_argument('--cycles',type=int,default=1200)
    p.add_argument('--original',action='store_true',help='Check full launch reaches a bumper in the original table')
    a=p.parse_args()
    directory=a.directory.resolve()
    for name in ('TEST.OK','SMOKE.TXT','SCREEN.BIN','TEST.LOG'):
        (directory/name).unlink(missing_ok=True)
    conf=Path(__file__).with_name('dosbox.conf').resolve()
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    test_command='test.exe'+(' /original' if a.original else '')+' > TEST.LOG'
    for label,cycles,command,marker in [('CORE',100000,test_command,'TEST.OK'),
                                     ('VIDEO',a.cycles,'cadet.exe /smoke','SMOKE.TXT')]:
        commands=[str(a.emulator.resolve()),'-conf',str(conf),'-fastlaunch','-exit',
                  '-set',f'cpu cycles=fixed {cycles}',
                  '-c',f'mount c "{directory}"','-c','c:',
                  '-c',command,'-c','exit']
        result=subprocess.run(commands,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
        (directory/(label+'.LOG')).write_bytes(result.stdout)
        if result.returncode or not (directory/marker).exists():
            raise RuntimeError(f'{label} DOS test failed; see TEST.LOG and {label}.LOG')
    print((directory/'TEST.OK').read_text().strip())
    print((directory/'SMOKE.TXT').read_text().strip())
    raw=(directory/'SCREEN.BIN').read_bytes()
    if len(raw)!=32768:
        raise RuntimeError('Incorrect video memory size')
    pixels=[]
    for y in range(200):
        row=[]
        for x in range(320):
            v=raw[(y&3)*8192+(y>>2)*160+x//2]
            row.append(v&15 if x&1 else v>>4)
        pixels.append(row)
    png(directory/'SMOKE.png',pixels)


if __name__=='__main__':
    main()

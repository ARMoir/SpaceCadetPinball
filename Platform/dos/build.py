#!/usr/bin/env python3
"""Build with an unpacked Open Watcom 2 installation (WATCOM environment)."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'build'/'dos'


def main():
    watcom = Path(os.environ['WATCOM']).resolve()
    host = 'binnt64' if os.name == 'nt' else 'binl64'
    env = dict(os.environ, WATCOM=str(watcom), INCLUDE=str(watcom/'h'),
               PATH=str(watcom/host)+os.pathsep+os.environ['PATH'])
    OUT.mkdir(parents=True, exist_ok=True)
    compiler = watcom/host/('wcl.exe' if os.name=='nt' else 'wcl')
    common = [str(compiler),'-q','-bt=dos','-0','-ms','-os','-s','-w4','-we',
              '-k4096', '-i='+str(ROOT/'Platform/dos')]
    # Table setup is independently compiled. .inc logic stays in its original
    # hot translation units to avoid new 8086 per-contact calls.
    for exe, sources in [('CADET',['main.c','game.c','table.c']),
                         ('TEST',['test_game.c','game.c','table.c'])]:
        subprocess.run(common+['-fe='+exe+'.EXE','-fm='+exe+'.MAP']+
                       [str(ROOT/'Platform/dos'/s) for s in sources],cwd=OUT,env=env,check=True)
    shutil.copy(ROOT/'Platform/dos/README.md',OUT/'README.TXT')
    print('Native 8086 DOS executables:',OUT)


if __name__ == '__main__':
    main()

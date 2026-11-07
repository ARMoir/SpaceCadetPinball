#!/usr/bin/env python3
"""Run real-mode binaries in DOSBox-X; fail if DOS tests don't leave markers."""
import argparse
import os
from pathlib import Path
import subprocess
import re
from convert import png


def validate_smoke(report, original=False):
    """A marker proves exit, not playability. Require a completed full shot."""
    if not report.startswith('PASS '):
        raise RuntimeError('Smoke test did not report successful completion')
    fields = dict(re.findall(r'\b([a-z_]+)=(\d+)\b', report))
    required = ('frames', 'steps', 'launches', 'full_launches', 'hits', 'score')
    if any(key not in fields for key in required):
        raise RuntimeError('Smoke report is missing gameplay counters')
    counters = {key: int(value) for key, value in fields.items()}
    if counters['frames'] != 360 or counters['steps'] != 1440:
        raise RuntimeError('Smoke test did not complete all frames and physics steps')
    if not 1 <= counters['full_launches'] <= counters['launches']:
        raise RuntimeError('Smoke test never completed a fully charged launch')
    if original and (counters['hits'] < 1 or counters['score'] < 500):
        raise RuntimeError('Space Cadet smoke test did not reach a scoring bumper')
    return counters


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
            for name in ('TEST.LOG', label+'.LOG'):
                path=directory/name
                if path.exists():
                    print(f'--- {name} ---')
                    print(path.read_text(errors='replace')[-4000:])
            raise RuntimeError(f'{label} DOS test failed; see TEST.LOG and {label}.LOG')
    print((directory/'TEST.OK').read_text().strip())
    report=(directory/'SMOKE.TXT').read_text().strip()
    print(report)
    validate_smoke(report, original=a.original)
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

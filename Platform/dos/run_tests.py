#!/usr/bin/env python3
"""Run real-mode binaries in DOSBox-X; fail if DOS tests don't leave markers."""
import argparse
import os
from pathlib import Path
import subprocess
import re
from hashlib import sha256
from datetime import datetime, timezone
from time import monotonic
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
    p.add_argument('--video',choices=('tandy','vga'),default='tandy',
                   help='Test actual Tandy 09h or VGA 13h memory layout')
    p.add_argument('--auto-video',action='store_true',
                   help='Use BIOS video auto-detection instead of an explicit switch')
    a=p.parse_args()
    directory=a.directory.resolve()
    summary=[
        "CADET_RUN_SUMMARY schema=1",
        "start_utc="+datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "commit="+os.environ.get("GITHUB_SHA","local/unset"),
        "emulator="+a.emulator.name,
        "hardware=DOSBox-X "+("VGA 13h" if a.video=="vga" else "Tandy 09h")+
            " emulation, NOT a physical hardware benchmark",
        "video="+a.video,
        "video_selection="+("auto" if a.auto_video else "explicit"),
        "video_cycles_per_ms="+str(a.cycles),
        "original_table="+str(int(a.original)),
    ]
    for asset in ("CADET.EXE","TEST.EXE","TABLE.DAT","TABLE.TGA","TABLE3D.TGA"):
        target=directory/asset
        if target.is_file():
            summary.append("%s bytes=%d sha256=%s" % (
                asset,target.stat().st_size,sha256(target.read_bytes()).hexdigest()))
        else:
            summary.append(asset+" MISSING")
    try:
        for name in ('TEST.OK','SMOKE.TXT','SCREEN.BIN','TEST.LOG',
                     'CORE.LOG','VIDEO.LOG','SMOKE.png','RUN_SUMMARY.TXT'):
            (directory/name).unlink(missing_ok=True)
        conf=Path(__file__).with_name('dosbox.conf').resolve()
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
        test_command='test.exe'+(' /original' if a.original else '')+' > TEST.LOG'
        video_switch='' if a.auto_video else ' /'+a.video
        for label,cycles,command,marker in [('CORE',100000,test_command,'TEST.OK'),
                                         ('VIDEO',a.cycles,'cadet.exe /smoke'+video_switch,'SMOKE.TXT')]:
            commands=[str(a.emulator.resolve()),'-conf',str(conf),'-fastlaunch','-exit',
                      '-set','dosbox machine='+('svga_s3' if a.video=='vga' else 'tandy'),
                      '-set',f'cpu cycles=fixed {cycles}',
                      '-c',f'mount c "{directory}"','-c','c:',
                      '-c',command,'-c','exit']
            started=monotonic()
            result=subprocess.run(commands,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
            (directory/(label+'.LOG')).write_bytes(result.stdout)
            ok=(not result.returncode and (directory/marker).exists())
            summary.append("%s_phase=%s exit_code=%d marker=%d host_seconds=%.2f cycles_per_ms=%d" % (
                label,"PASS" if ok else "FAIL",result.returncode,
                int((directory/marker).exists()),monotonic()-started,cycles))
            if not ok:
                for name in ('TEST.LOG', label+'.LOG'):
                    path=directory/name
                    if path.exists():
                        print(f'--- {name} ---')
                        print(path.read_text(errors='replace')[-4000:])
                raise RuntimeError(f'{label} DOS test failed; see TEST.LOG and {label}.LOG')
        core_report=(directory/'TEST.OK').read_text().strip()
        if not core_report.startswith("PASS"):
            raise RuntimeError("Core marker missing PASS completion")
        print(core_report)
        report=(directory/'SMOKE.TXT').read_text().strip()
        print(report)
        counters=validate_smoke(report, original=a.original)
        expected_vga=int(a.video=='vga')
        if counters.get('video_vga')!=expected_vga:
            raise RuntimeError('Game chose wrong video mode; expected VGA=%d' % expected_vga)
        summary.append("smoke_gameplay="+" ".join(k+"="+str(v) for k,v in counters.items()))
        raw=(directory/'SCREEN.BIN').read_bytes()
        if len(raw)!=(64000 if a.video=='vga' else 32768):
            raise RuntimeError('Incorrect video memory size for '+a.video)
        pixels=[]
        for y in range(200):
            row=[]
            for x in range(320):
                if a.video=='vga':
                    color=raw[y*320+x]
                else:
                    value=raw[(y&3)*8192+(y>>2)*160+x//2]
                    color=value&15 if x&1 else value>>4
                if color>=16:
                    raise RuntimeError('VGA image has unexpected non-RGBI palette pixels')
                row.append(color)
            pixels.append(row)
        png(directory/('SMOKE-VGA.png' if a.video=='vga' else 'SMOKE.png'),pixels)
        # Retain both display dumps for pixel-for-pixel parity validation.
        (directory/('SCREEN-VGA.BIN' if a.video=='vga' else 'SCREEN-TANDY.BIN')).write_bytes(raw)
        summary.append("result=PASS")
    except Exception as exc:
        summary.append('result=FAIL error='+type(exc).__name__+': '+str(exc))
        raise
    finally:
        summary.append('finish_utc='+datetime.now(timezone.utc).isoformat(timespec='seconds'))
        (directory/'RUN_SUMMARY.TXT').write_text('\n'.join(summary)+'\n',encoding='utf-8')


if __name__=='__main__':
    main()

#!/usr/bin/env python3
"""Check that F-007 source-art graphics do not alter deterministic gameplay."""
import argparse
from pathlib import Path
from run_tests import validate_smoke
from verify_video_modes import unpack_tandy

def gameplay(summary):
    line=next(line for line in summary.read_text().splitlines()
              if line.startswith("smoke_gameplay="))
    return validate_smoke("PASS "+line.split("=",1)[1],original=True)

def compare(directory):
    for video in ("TANDY","VGA"):
        baseline=gameplay(directory/("RUN_"+video+"_SUMMARY.TXT"))
        source=gameplay(directory/("RUN_"+video+"_SOURCE_SUMMARY.TXT"))
        # Graphics affect the number of completed slow frames; everything
        # else, including physics, scoring, gates, input, should be identical.
        for key in baseline.keys()|source.keys():
            if key!="slow_frames" and baseline.get(key)!=source.get(key):
                raise AssertionError("%s source art changed simulation %s" % (video,key))
    before=unpack_tandy((directory/"SCREEN-TANDY.BIN").read_bytes())
    after=unpack_tandy((directory/"SCREEN-TANDY-SOURCE.BIN").read_bytes())
    differences=sum(a!=b for a,b in zip(before,after))
    if differences<100:
        raise AssertionError("Source-art playfield must differ from 3D baseline")
    print("PASS: sourceart simulation unchanged in both video modes; "
          "%d display pixels differ from default 3D" % differences)

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("directory",nargs="?",type=Path,default=Path("build/dos"))
    compare(p.parse_args().directory)

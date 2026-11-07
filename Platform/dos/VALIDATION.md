# Prototype validation

Tested November 7, 2026, using the owner-supplied Windows PINBALL.DAT.
Source resource SHA-256 and the complete import inventory are in data/IMPORT.json.

## Completed

* Open Watcom 2, pinned `2026-11-01-Build`, Windows x64 host.
  Both binaries build warning-free with `-bt=dos -0 -ms -os -s -w4 -we`.
* CADET.EXE: 27,318 bytes, plain DOS MZ with no appended extender;
  DGROUP 51,392 bytes. MZ minimum load allocation + PSP + 32KB background:
  108,054 bytes. This is a checked lower bound, not a peak heap measurement.
* Five host converter tests: all screen pixels round-trip through the four
  Tandy banks; padding, nibble order, bitmap orientation, malformed/truncated
  inputs, attributes, segment generation, and generated diagnostic pack.
* DOSBox-X 2026.10.01, Tandy machine, normal 8086 core, 1MB emulated total
  memory, XMS/EMS/UMB disabled. DOS programs only use conventional memory.
* Native TEST.EXE passes the synthetic physics checks and 20,000-step soak.
  Imported-table regressions cover seven scoring bumpers, both kickbacks at
  three entry offsets, eight source plunger frames, 12 partial-charge returns,
  and full-power scoring shots. Tilted paddles retain passive collisions;
  a synthetic wall test verifies kickback returns cannot bypass walls.
* Five smoke-report parser tests verify that incomplete runs, missing full
  launches and zero-score original-table runs fail the runner.
* CADET /smoke at fixed 1200 emulator cycles/ms: 360 frames, 1440 substeps,
  2500 points, five bumper hits, one fully charged launch, three balls remaining.
  This run reported seven late frames; timing varies with host load.
* Inspected both the native video-memory dump and the actual DOSBox-X window:
  original table artwork, ball, flippers and HUD render in Tandy mode.

The emulator cycle setting is a repeatable test load, **not** a conversion to
7.16 MHz NEC V20 performance. Timing and sound on a real Tandy are unverified.

## Reproduce

```
python Platform/dos/test_convert.py
python Platform/dos/test_run_tests.py
python Platform/dos/build.py
python Platform/dos/verify_build.py
python Platform/dos/run_tests.py --emulator /path/to/dosbox-x --original
```

Copy data/TABLE.DAT and data/TABLE.TGA to build/dos before running DOS tests.
CI performs these steps and publishes EXEs, maps, table pack and logs.

## Required hardware follow-up

Test on the actual Tandy 1000 EX/V20 with 640KB: simultaneous flipper keys,
quick Space taps, keyboard repeat/release, sustained play, nudge recovery,
PSG sound, exit/relaunch and DOS keyboard restoration. Measure late-frame
count with `CADET /record`; examine ball motion and input latency, not only FPS.
Do not label this prototype hardware-verified until these checks are complete.

Flattened walls can still trap a ball, including around the upper-left chute;
these scoring/containment tests do not certify every trajectory or mission.

Remaining gameplay gaps and original-vs-simplified boundaries are in README.md.

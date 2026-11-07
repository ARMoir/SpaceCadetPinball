# Prototype validation

Tested November 7, 2026, using the owner-supplied Windows PINBALL.DAT.
Source resource SHA-256 and the complete import inventory are in data/IMPORT.json.

## Completed

* Open Watcom 2, pinned `2026-11-01-Build`, Windows x64 host.
  Both binaries build warning-free with `-bt=dos -0 -ms -os -s -w4 -we`.
* The F-006 flipper baseline on 2026-11-07 was a 27,478-byte
  plain DOS MZ, DGROUP 51,392 bytes, with a 108,214-byte minimum
  image/PSP/background allocation (not peak heap). **These numbers are
  a historical baseline**, not the current size after adding in-memory
  flight tracing; read the latest Actions `verify_build.py` output for
  current EXE/DGROUP/memory checks. The 16-bit loader remains real mode.
* Five host converter tests: all screen pixels round-trip through the four
  Tandy banks; padding, nibble order, bitmap orientation, malformed/truncated
  inputs, attributes, segment generation, and generated diagnostic pack.
* DOSBox-X 2026.10.01, Tandy machine, normal 8086 core, 1MB emulated total
  memory, XMS/EMS/UMB disabled. DOS programs only use conventional memory.
* Native TEST.EXE passes the synthetic physics checks and 20,000-step soak.
  Imported-table regressions cover seven scoring bumpers, both kickbacks at
  three entry offsets, eight source plunger frames, 12 partial-charge returns
  followed by a second charge/relaunch of the same ball, and full-power
  scoring shots. The 2026-11-07 flipper follow-up also tests stronger
  tip-vs-pivot contacts on **both** flippers, retained incoming horizontal
  momentum, a faster wall reflection, and the moving midpoint contact.
  Tilted paddles retain passive collisions; a synthetic wall test verifies
  kickback returns cannot bypass walls. The later F-002/F-004
  regression suite adds radial upper-left bumper penetration/rebound,
  top/side screen boundary retention, and true lower-drain differentiation.
* Five smoke-report parser tests verify that incomplete runs, missing full
  launches and zero-score original-table runs fail the runner.
* CADET /smoke at fixed 1200 emulator cycles/ms: 360 frames, 1440 substeps,
  2500 points, five bumper hits, one fully charged launch, three balls remaining.
  This run reported seven late frames; timing varies with host load.
* Inspected both the native video-memory dump and the actual DOSBox-X window:
  original table artwork, ball, flippers and HUD render in Tandy mode.
* **F-007 enhanced 3D artwork (2026-11-07):** offline `enhance_table.py`
  generates a second 32768-byte raw Tandy background, `TABLE3D.TGA`,
  from the untouched `TABLE.TGA` and `TABLE.DAT`. Converter tests verify
  unchanged collision packs, all seven source bumper centers, exact Tandy
  nibble/bank packing, fixed 16-color palette, unchanged colored lights,
  no geometry warp, deterministic output and preservation of the original
  background. DOSBox-X smoke tests exercise the enhanced default.
  `CADET /classic` selects the original artwork and removes the tiny
  dynamic ball shadow/glint; `CADET /3d` forces the enhanced artwork.
  The 3D effect's perceived depth, CRT readability and tiny ball-overlay
  rendering cost still need **physical V20 hardware confirmation**.
  Static shading is fully precomputed and adds no work to the physics loop.

The emulator cycle setting is a repeatable test load, **not** a conversion to
7.16 MHz NEC V20 performance. Timing and sound on a real Tandy are unverified.
The F-006 flipper improvement and repeat-plunger regressions passed
[GitHub Actions on 2026-11-07](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42384787810),
but **the new feel, bounce variation, and contact reliability are not yet
verified on the physical Tandy**. The project owner reported weak flippers and
predictable trajectories on the *previous* real-hardware build, motivating this
source-inspired adjustment. Original flipper impulse parity is not claimed.

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

The owner observed **two upper-left playability failures** on a real Tandy:
a ball trapped next to the upper-left bumper, and a separate apparent
disappearance after entering the highlighted upper-left region.
The latest F-004 correction reassembles the eight-sided imported bumper
outlines into radial contacts and resolves them after nearby static rails.
The F-002 containment also prevents top/side exits being counted as drains;
only the bottom drain consumes a ball. Both are source-inspired or temporary
playability adaptations with documented fidelity gaps, **not yet verified
on the physical computer**.

For any repeat occurrence, run `CADET /record` and press Esc as soon as the
ball behaves unexpectedly. Send `INPUT.TXT` and `FLIGHT.TXT`. The latter
contains the last 256 rendered frames (about 14 seconds) of Q6 ball flight,
state and boundary counters, buffered without gameplay disk writes. This
can distinguish an off-screen exit/ball drain from an on-screen collision
trap without relying on a screenshot alone.

**New right-edge real-Tandy observation (2026-11-07):** `FLIGHT(2).TXT`
shows a ball pass the imported exterior right wall and travel to about
(235, 113) at frame 1439, then encounter the emergency screen safety
boundary at frame 1440, followed by a second boundary contact at frame
1455 near (234, 152). The flight log reports `boundary_hits=2`,
`nudges=0`, and `tilts=0`. The original SCP2 right outer rail occupies
approximately (170,40) to (193,166); at y=100 it is around x=181.
Those observations show the screen safety rail was working as a fallback
but the ordinary projected rail was not containing the ball.

**F-002 remedy under test:** `table_index` selects the source-derived
long right exterior edge and a one-sided normal collision now recovers
balls that cross it before reaching the far side of the screen.
Regression starts beyond the rail at the reported y=102 with an outward
velocity and requires an inward reflected shot, without counting a
screen-edge contact. The `right_rail_saves` counter is separate from
`boundary_hits`; `FLIGHT.TXT` schema 3 includes a `RIGHT_RAIL` column.
The 16-bit /original and DOSBox-X checks pass; **correctness, feel, and
performance on the actual 7.16 MHz Tandy still need confirmation.**
This is temporary geometry containment, not a faithful import of the
original 3D ramps and gate groups.

Flattened collision groups, omitted ramps, and one-way gates can still trap
a ball in the upper-left chute; these tests do not certify every trajectory.

Remaining gameplay gaps and original-vs-simplified boundaries are in
[README.md](README.md). The mandatory [fidelity audit and exception register](FIDELITY.md)
is the primary source for **P0/P1 remediation priorities**, references to the
original implementation, and required evidence whenever the DOS version
deviates from the original. This validation records passing test cases, not
approval of the currently documented gameplay approximations.

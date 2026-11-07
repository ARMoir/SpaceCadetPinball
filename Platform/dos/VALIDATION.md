# Prototype validation and final handoff

Reviewed November 7, 2026, using the owner-supplied Windows PINBALL.DAT.
The [feature-status inventory](FEATURE_STATUS.md) is the definitive account
of original-game systems **not yet implemented**; successful tests below
do not imply complete gameplay or final V20 hardware certification.
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
* **VGA mode 13h integration, 2026-11-07:** a single `CADET.EXE`
  supports `/tandy`, `/vga`, and default BIOS auto-detection.
  The mode-13 VGA renderer expands the existing 32768-byte four-bank
  artwork into a 64000-byte linear framebuffer/background once at launch.
  Both video paths retain identical original 0..15 RGBI artwork,
  sprite coordinates, physics, controls, and scoring; Tandy retains
  its original PSG and CRT hardware handling. VGA programs the DAC
  and sends event tones to the PC speaker instead of Tandy PSG.
  The DOSBox-X CI runs full 360-frame/1440-step gameplay in **both**
  Tandy and VGA modes, with explicit switches and automatic detection.
  `verify_video_modes.py` compares decoded Tandy and VGA screen
  dumps pixel-for-pixel; the initial dual-mode CI run
  [42401448271](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42401448271)
  reported **64000 identical pixels**, matching score 3500,
  seven scoring bumper hits, two full-charge launches and two balls
  remaining. This verifies the chosen final emulated frame, **not**
  every intermediate frame. Runtime speed, VGA hardware compatibility
  and speaker audio need actual machine confirmation.
  `verify_build.py` now tracks Tandy 32KB and VGA 64000-byte
  far-buffer minimum memory requirements separately.
* One verified [2026-11-07 GitHub Actions run](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42393901598):
  360 rendered frames and 1440 physics steps, score 3500, seven bumper hits,
  two full-charge launches, two balls remaining and ten slow frames
  at fixed 1200 DOSBox-X emulator cycles/ms. This is an example
  **emulated** run, not a measurement of physical V20 frames/second;
  results and late-frame counters vary with host and workload.
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
python Platform/dos/test_analyze_logs.py
python Platform/dos/test_menu_font.py
python Platform/dos/test_video_modes.py
python Platform/dos/test_enhance_table.py
python Platform/dos/build.py
python Platform/dos/verify_build.py
python Platform/dos/run_tests.py --emulator /path/to/dosbox-x --original --video tandy --auto-video
python Platform/dos/run_tests.py --emulator /path/to/dosbox-x --original --video vga
python Platform/dos/verify_video_modes.py build/dos
python Platform/dos/run_tests.py --emulator /path/to/dosbox-x --original --video vga --auto-video
python Platform/dos/verify_video_modes.py build/dos
```

Copy `Platform/dos/data/TABLE.DAT` and `TABLE.TGA` to `build/dos`
and generate `TABLE3D.TGA` with `enhance_table.py` before running
the DOSBox-X tests, or use the pinned GitHub Actions workflow, which
performs these steps and publishes the EXEs, matching data, previews
and logs in one artifact.

## Required hardware follow-up

**VGA compatibility physical check (2026-11-07):**
Run `CADET /vga /record` on a conventional VGA PC with 256KB+
of conventional RAM available beyond DOS/driver overhead (640KB
recommended). Verify 320x200 centered art, left/right mouse and
keyboard flippers, full launch, score, PC-speaker tones, 20 minutes
of uninterrupted play, and clean restoration of the original video
mode on Esc. Compare `CADET /tandy` on the actual Tandy/PCjr.
Do **not** assume that emulator pixel parity proves physical VGA
performance, sound quality, or mode switching. The VGA path intentionally
uses 16 of its 256 palette indices; there is no full 256-color
resource converter. The Tandy-specific CRTC shift is disabled on VGA.



**Optional mouse flippers (2026-11-07):** `CADET.EXE` now detects a
standard DOS `INT 33h` mouse driver at startup (after Tandy mode setup).
Mouse left/right buttons produce the existing left/right flipper key bits,
including dual-button holds. DOS `INT 33h` function 3 supplies held
button state; function 5 captures fast presses released between the 18.2 Hz
input samples. The game does not draw a mouse pointer, move the paddle
with mouse position or modify physics. `/nomouse` disables the feature,
and missing-driver / emulator smoke paths do not poll the driver. Core
DOS tests verify mapping of 0/left/right/both/middle-only button states
and matching flipper collisions; a source-contract regression checks
driver detection and input latching. **Actual serial mouse driver support,
short-click responsiveness and frame-time cost on the Tandy remain UNVERIFIED.**
Test both buttons together, rapid clicks, mixed Z + right mouse, and
`CADET /nomouse` on hardware.

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
`boundary_hits`. The latest follow-up now uses `FLIGHT.TXT` schema 4,
which records `RIGHT_RAIL`, `LEFT_RAIL`, and `BORDER` separately.
The 16-bit /original and DOSBox-X checks pass; **correctness, feel, and
performance on the actual 7.16 MHz Tandy still need confirmation.**
This is temporary geometry containment, not a faithful import of the
original 3D ramps and gate groups.

**Additional F-002 / F1 controls review (2026-11-07):** the F1 menu's
tiny 3×5 font previously encoded `A` identically to `H`, misleading
users about the left bump key. The glyph has been corrected and the F1
instructions now show A / D / X on separate lines. A Python regression
test checks the glyph and key mapping.

The original imported *left* outer rail, about (62,40) to (39,166),
is now used for a matching one-sided outer-boundary guard, while the
right-hand rail guard is **disabled when the ball is legitimately in
the shooter tube**. New DOS regression cases include a fast left-side
escape, unchanged inboard ball motion and a valid shooter trajectory.
These guards apply only within their source rail intervals and do not
turn the lower outlanes or ramp passages into a global invisible wall.
The positive regressions and emulated build pass, but physical Tandy
playability and timing are **not yet re-verified**. Source 3D/layer
collision remains incomplete.

Flattened collision groups, omitted ramps, and one-way gates can still trap
a ball in the upper-left chute; these tests do not certify every trajectory.

Remaining implemented, simplified and **absent** gameplay systems are
documented in [FEATURE_STATUS.md](FEATURE_STATUS.md), with source
boundaries in [README.md](README.md). The
[fidelity audit and exception register](FIDELITY.md) preserves original-source
references, P0/P1 remediation priorities and the technical reasons for
documented deviations. This validation records passing test cases,
**not approval of the approximations, full-feature parity, or final
hardware verification**.

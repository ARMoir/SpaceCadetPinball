# Prototype validation and final handoff

Reviewed November 8, 2026, against the checked-in converted source pack.
The original Windows PINBALL.DAT is NOT checked into this repository;
attribute-exact future reimports must be validated with that source file.
The [feature-status inventory](FEATURE_STATUS.md) is the definitive account
of original-game systems **not yet implemented**; successful tests below
do not imply complete gameplay or final V20 hardware certification.
Source resource SHA-256 and the complete import inventory are in data/IMPORT.json.

## Completed

**Current baseline:** GitHub Actions run
[42548769975](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42548769975)
passed the Tandy/VGA real-mode build and deterministic emulator checks for
commit 187f15a8. Since that run, third-hit alternating-sling escape,
fixed-coordinate near-crest ramp assistance, lower-apron outward-ball
deflection, source-style text and offline Tandy art cleanup are in the
tested code. The last three gameplay features are not original source
collision/layer implementations and real V20 effectiveness remains
UNVERIFIED. All future updates to the importer/physics should require
paired positive/negative regression coverage and full CI.

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
* Smoke-report parser regressions verify that incomplete runs, missing
  full launches, zero-score original-table runs and re-enabled dynamic
  ball shadowing fail the runner.
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
  `CADET /classic` selects the original artwork; `CADET /3d` forces
  the enhanced artwork. Both now render the unmodified, source-derived
  metallic ball sprite, with the former shadow/glint overlay removed
  following physical Tandy feedback that it looked less round. Smoke
  validation requires `moving_ball_shadow=0` in either video mode, and
  `test_run_tests.py` covers absent/re-enabled overlay reports.
  The corrected ball's CRT appearance and hardware frame timing remain
  **UNVERIFIED** pending a new V20 test. Static artwork shading remains
  fully precomputed and adds no work to the physics loop.

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
python Platform/dos/test_source_layout.py
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

**Forgiving center gap (F-006, 2026-11-07):** the imported
raised flipper tips are about 18 pixels apart in projected DOS
coordinates. The original has a real center drain; the precise
original-vs-port drain rate is not measured. At the owner's request,
the DOS port now slightly favors easier play over strict fidelity
when continuous flipper rotation cannot be represented accurately.
A five-pixel inward **collision-only** extension is precomputed per
raised flipper, activated solely for a descending ball while that
flipper is held. Synthetic 16-bit regression tests verify left and
right near-center saves, unaffected idle drains, disabled saves
during tilt, a still-open mid-gap with both flippers held, and no
false catch from underneath. Unchanged game physics and art are used
by both Tandy and VGA. The automated 360-frame runs still pass.
Physical test remains **PENDING**: compare a few minutes of attempts
to save descending near-center balls, including late key presses,
released flippers, and holding both. The additional narrow collision
candidate should be cheap but its real 7.16 MHz cost is UNVERIFIED.
Adjust `FLIPPER_TIP_ASSIST_PX` in `game.h` if testing shows an
unfairly invisible catch or not enough improvement.

**Universal stuck-ball recovery (F-008, 2026-11-07):**
The original desktop `control::UnstuckBall` tries an upward throw
outside flipper/plunger bounds, then relaunches if recovery fails.
DOS now detects lack of real positional progress (both x/y travel
within 3 pixels for **110 physics substeps**, about 1.5 seconds).
A first rescue nudges upward/inward; further unresolved intervals of
73 substeps permit up to two short **geometry-verified** local moves
before a free same-ball plunger return. The watchdog backs off after
six consecutive steps at least 10px from the initial trapped
position. It exempts shooter travel, waiting balls, kickbacks,
flipper capture areas and lower drains; pause/menu logic does not
advance game physics. The 16-bit regression suite tests all three
recovery stages, preserved score and balls, continuing normal play,
fast velocity with near-zero actual progress, and the exclusions.
`SMOKE.TXT` and `INPUT.TXT` now include three counters:
`unstick_nudges`, `unstick_moves`, `unstick_relaunches`.

**Hardware acceptance pending:** on the real 7.16 MHz V20, first
confirm that previously trapped shots escape from the right sling
and other collapsed-layer pockets without unnatural teleports.
Hold a ball on a flipper, let a normal center drain occur, and
perform a weak-shot shooter return to check that none triggers a
false rescue. Use `CADET /record` to inspect the counters and
`slow_frames`. If `unstick_relaunches` becomes common, the
underlying collision geometry still needs fixing. Emulated tests
do not establish actual V20 performance or exact original timing.

## Required hardware follow-up

**New F-002/F-008 physical acceptance:** With `CADET /record`,
record a set of upper-right and upper-left ramp attempts that nearly
reach the crest, flipper shots into both outer sling backs, downhill
return passes through those lanes, shooter and legitimate kickback
returns, and repeated alternating slingshot contacts. Compare
`ramp_crest_assists`, `sling_backflow_redirects`,
`sling_loop_breaks`, `boundary_hits`, `slow_frames` and whether
the ball actually completes the intended climb without false rescues.
These cumulative statistics need corresponding short FLIGHT.TXT
snapshots or live observation to establish outcome. None is a
substitute for real-hardware testing. Final source-geometry fidelity
requires a reimport of omitted TOneway and TRamp source attributes.

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

**Right and left slingshot anti-trap fix (2026-11-07, F-002):**
The source table has two closed 14-segment sling outlines, about
x=76–91/y=118–143 and x=141–156/y=118–143. Instead of
treating their outlines as conflicting two-sided capsules, DOS now
recognizes each closed source shape at table-index time and uses one
solid convex 3-face collision hull with outward Q8 normals. Only
those two perimeter chains are removed from generic edge indexing.
Each physics step keeps the ball center outside the filled hull
(including its 3px collision radius); nearby approaches receive
normal outward reflections rather than arbitrary random kicks.
The first broad-phase test is a short bounding-box check. The art,
SCP2 format, lower drains, flippers, lamps and VGA/Tandy rendering
remain unchanged. `INPUT.TXT` reports `sling_left_rescues` and
`sling_right_rescues` if `CADET /record` is used.

The 16-bit imported-table regression checks both original sling
hulls, source-perimeter exclusion from collision bands, direct
center escape, and 7 approach directions/speeds per side over
repeated steps. This tests the user's problematic right side
explicitly. Emulated pass does **not** yet prove that the physical
Tandy trajectory and visual appearance are completely resolved.
Re-test several right-sling entries at full speed and watch for
unnatural bounces or V20 slow frames.

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

## Source-aligned module refactor (2026-11-07)

[SOURCE_MAP.md](SOURCE_MAP.md) maps the
[current architecture](README.md#source-aligned-architecture) to original
C++ routines, documenting limited matches, divergent code and open F-###
exceptions. The refactor moved SCP2 table parsing/indexing into
[`table.c`](table.c), independently compiled only at load time.
`game.c` and `main.c` include ordered `.inc` implementation fragments.
Both `flipper.inc` and `plunger.inc` remain **inside** `game_step`,
without introducing additional physics calls. Expanded C statements
were compared to the pre-refactor code to confirm the intended
behavior-neutral extraction.

**Verified:** [GitHub Actions run 42422864888](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888)
passed the `test_source_layout.py` regression tests, converter and
host tests, Open Watcom 16-bit build, MZ/DGROUP/memory checks, DOSBox-X
Tandy auto/forced and VGA forced/auto gameplay, and final 64,000-pixel
video parity. The workflow published the `CADET` artifact.
The first refactor run failed because `test_menu_font.py` still searched
for help text and glyphs in the former monolithic `main.c`. Moving those
test checks to `hud.inc` and `input.inc` corrected the issue.

**Not verified:** physical 7.16 MHz V20 responsiveness, physical VGA
performance, or exact source-level game-rule fidelity. These remain
distinct from the passing emulator/build tests, and all F-001 through
F-008 exceptions remain open.

## Middle mouse, timed HUD and high scores (2026-11-07)

- Original `options.cpp` bindings for middle mouse launch, F5 sound
  and F8 controls are restored alongside existing keyboard controls.
  Only INT 33h drivers reporting three buttons are queried for middle
  press counts. Rapid scan codes for three-letter score entry go into
  a bounded IRQ-to-main queue; no DOS file access occurs in IRQ1.
- The `TTextBox.cpp`-inspired HUD uses one 26-frame event message,
  preserving gameplay and sprite art but not the original message queue
  and priority system. Menu HIGH SCORES is available without altering
  the table graphics.
- The separate `scores.c` module keeps a local five-entry table
  in `SCORES.DAT`; loads at startup, and saves only on confirmation
  following a qualifying game-over. Invalid/missing files reset to
  blank scores; `/smoke` never accesses the file. No file access
  occurs during active ball physics.
- Native `TEST.EXE` assertions cover rank/tie order, middle mouse
  button masks, file roundtrip, invalid names and corrupt file rejection.
  Host input/HUD tests check original shortcuts and initial-entry routing.
  Standard DOSBox-X Tandy/VGA gameplay and frame parity are required.
- **Physical acceptance pending:** on the real EX/V20, test held
  middle plunger, quick tap, driver with only two reported buttons,
  F5/F8, timed messages and the entire score-entry/save/reload flow.
  Measure score prompt redraw and test a read-only/disk-full directory
  for the visible SAVE FAILED indicator. Behavior and V20 cost are
  UNVERIFIED pending those hardware tests. F-007 and new F-009 remain
  open for original UI/score parity.

### Rechecking the three November 7 UI improvements

A fresh automated build should cover the original mouse middle-button
plunger mapping, F5/F8 shortcut routing, HUD event messages and the new
five-entry `SCORES.DAT` implementation. The `TEST.EXE` suite checks
score sorting, roundtrip persistence and malformed-file recovery. The
18.2 Hz `/smoke` mode deliberately skips score-file I/O, so live
initials entry, mouse-driver compatibility and physical V20 performance
must still be tested manually after the DOS build succeeds.

## Runtime packaging review, 2026-11-08

The current mouse/HUD/high-score build is 39,216 bytes with DGROUP 64,496
bytes. Its checked minimum image + PSP + background is 131,536 bytes for
Tandy and 162,768 bytes for VGA. The prior 128KB Tandy guard failed by 464
bytes; the guard is now 160KB (VGA stays 192KB), with the independent 64KB
DGROUP limit retained. These are minimum allocations, not peak heap or
physical 640KB/V20 validation. Near-data headroom is limited; future static
data additions need care. No gameplay behavior changed in this review.

The runtime package contains only CADET.EXE, TABLE.DAT, TABLE.TGA and
TABLE3D.TGA. Packaging rejects missing/invalid required inputs and excludes
local high scores and developer/test files. Successful branch workflows
publish a versioned prerelease only after all checks; validation evidence
is a separate artifact.

## VGA source-color upgrade, 2026-11-08

The new VGA1 asset is checked against all thirteen SCP2 sprite dimensions and
transparency masks, including corrupt/truncated inputs. The runtime uses a
64768-byte far VGA graphics allocation (64000 pixels plus 768 palette bytes).
Tandy allocation and source collision data are unchanged. RGBI VGA remains
available for pixel parity with Tandy; source-color VGA has its own capture
and must match all gameplay/input counters from the RGBI run. Physical
monitor color quality and V20 performance remain unverified.

## Ball-position guard validation (2026-11-08)

A new synthetic test failed before the fix: the ball crossed a static wall
at an allowed speed and was corrected to the wrong side. Axis-aligned and
diagonal cases now preserve the approach side, while endpoint passage and
retreat remain free. The 80,000-step varied-control suite checks both source
outer rails, seven bumper disks and bounded sling collision regions after
each active playfield step, in addition to the existing 20,000-step soak.
The sling assertion includes its finite AABB; extending its acute half-planes
infinitely would incorrectly flag valid space above the sling. See F-002 for
scope and performance uncertainty; this is not a hardware certification.

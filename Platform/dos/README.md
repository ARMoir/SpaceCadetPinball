# Space Cadet: native DOS / Tandy prototype

This is a separate C89, 16-bit real-mode target for an NEC V20 / 8088-class
Tandy 1000. It does not use SDL, Windows, a DOS extender, an FPU, XMS or EMS.
The desktop version is unchanged. This is a playable experimental prototype,
**not** a feature-complete port or a final hardware-certified release.

**Final project handoff:** [FEATURE_STATUS.md](FEATURE_STATUS.md) lists
exactly what works, what is simplified, what is **not implemented**, and
what still needs physical V20 testing. [VALIDATION.md](VALIDATION.md) records
automated checks and hardware observations. Feature work is paused unless
a real playability problem justifies another change.

## Project priorities: playability first, authenticity a very close second

**First priority:** make Space Cadet smooth, reliable, playable, and fun on the
real Tandy 1000 EX with a 7.16 MHz V20. Ball motion, flipper strength, input
response, shooter return, and stable frame timing must work on the machine.

**Very close second:** preserve original Space Cadet logic, rules, physics,
artwork, and behavior wherever practical. Start from the source and optimize
implementation (fixed-point math, precomputed geometry, bounded collision
checks, and efficient Tandy rendering) before changing mechanics. If restoring
source behavior conflicts with actual hardware playability, use the smallest
playable adaptation and document the difference and its evidence.

Before each DOS code or data change, consult the standing
[**fidelity audit, source references and exception register**](FIDELITY.md)
and the repository [`AGENTS.md`](../../AGENTS.md). If a necessary hardware
constraint changes gameplay behavior, document the deviation, original
source, reason, measurement (or explicitly **UNVERIFIED**), tests, and return
path in the same change. Reference its **F-###** entry in the commit summary.
An unported feature or unmeasured workaround is not a justified optimization.

**Work order:** prioritize hardware-reported weaknesses such as underpowered
flippers, repeated ball paths, missed contacts, stuck balls, and launcher
recovery. Next, restore the highest-impact original rules (P0 kickback/gate and
collision-layer behavior, then P1 plunger, bumpers, nudging and flippers).
A successful emulator build does not establish hardware performance or parity.

## Build and play

1. Unpack Open Watcom 2 and set `WATCOM` to its absolute installation directory.
2. From the repository root, run `python Platform/dos/build.py` (Python 3.9+).
3. Copy `TABLE.DAT`, `TABLE.TGA` and `IMPORT.json` from
   `Platform/dos/data` into `build/dos`, or regenerate these from a Windows
   `PINBALL.DAT` using
   `python Platform/dos/convert.py /path/to/PINBALL.DAT --output build/dos`.
   Full Tilt CADET.DAT is not supported. `make_demo.py` produces an asset-free
   diagnostic table, not Space Cadet.
4. Generate the optional, CPU-free enhanced static background on your PC:
   `python Platform/dos/enhance_table.py --table build/dos/TABLE.TGA --pack build/dos/TABLE.DAT --output build/dos/TABLE3D.TGA --preview build/dos/PREVIEW3D.png`.
   GitHub Actions already runs this step and packages both looks.
5. Copy **`CADET.EXE`, `TABLE.DAT`, `TABLE.TGA`, and `TABLE3D.TGA`** into
   the same DOS directory. `CADET` uses the enhanced background by default;
   `CADET /classic` loads the original unmodified background, with no ball
   shadow or glint. `CADET /3d` requests the enhanced style explicitly.
   If `TABLE3D.TGA` is missing, the game automatically uses `TABLE.TGA`.
   Both background files are raw 32768-byte Tandy mode-09h bank images
   (not Targa). Keep the executable and SCP2 `TABLE.DAT` from one build.

Controls: **Z** left flipper, **/** right flipper, hold **Space** to pull the
plunger back and release to launch (about 1.3 seconds to full power, with a
small ~1% launch variation), **X** upward bump, **A** bump from the left,
**D** bump from the right, **S** toggle sound on/off (shown in the
right sidebar), **N** new game,
**Esc** quit. Both flippers can be held together. There are three balls.
`/nosound` starts with sound disabled, and **S** can re-enable it during
play. Switching sound off silences any currently playing tone immediately.
`/noshake` disables optional hardware viewport shaking (use this if the
display shifts incorrectly on actual hardware). The small one-character
shake is done by a short MC6845 CRTC start-address offset and is restored
on pause, menu and exit, with no full-screen redraw.
Run on Tandy-compatible video;
there is no VGA fallback or reliable automatic Tandy detection in this version.
Visual style may be selected only at startup, not while the ball is moving.

The native program uses BIOS mode 09h, packed 4bpp B800 video memory with four
8KB banks, and a private IRQ1 make/break keyboard handler. It restores the prior
keyboard vector and video mode on normal exit. Do not terminate the emulator
to test cleanup; use Esc. No PIT reprogramming: the BIOS timer drives 18.2 FPS
with four fixed physics substeps (about 72.8 Hz). Late frames are counted and
dropped rather than running unbounded catch-up work.

## Optional static faux-3D enhancement (F-007)

The 16-color original background remains **byte-for-byte unchanged** in
`TABLE.TGA`. `TABLE3D.TGA` is generated **offline** by
[`enhance_table.py`](enhance_table.py): existing gray ramps/rails gain
top-left highlights and down-right shading, and the seven source-derived
bumper caps receive subtle bevels/cast shadows. **The second, stronger offline
shading pass** adds clearer top-left metallic rims, darker lower-right rail
undersides, short contact shadows on existing gray metal near flipper guides
and shooter-lane hardware, and brighter domes/deeper undersides on the seven
original bumper caps. It only recolors pixels that were already gray or
white in the source image. **A third, purely offline trompe-l'œil pass**
derives a three-tier *visual-only* height mask from already painted metal
and the original seven bumper centers. One-pixel bright/dark Mach-band edges,
1–3 pixel *internal* sidewall shading, small ordered-dither contact shadows,
and longer offset shadows for higher bumper housings simulate extrusion.
The masks are not exported to the game or used for ball collisions.
Colored lamps and target art remain unchanged, and no perspective warp
or new collision geometry is introduced. `TABLE.DAT`, original ball/flipper/plunger sprite data,
gameplay state and scoring are identical between looks.

The enhanced ball gets a small shadow and one highlight pixel, drawn only
when the ball was already moving and being repainted. Those pixels fit in
the existing 6x6 dirty rectangle; no screen-wide redrawing or added
simulation step. Static artwork introduces **no extra work during gameplay**.
The 4-pixel ball decoration has a tiny runtime cost and still needs
measurement for brightness and perceived motion on the real CRT.

Compare the same shot in `CADET /classic` versus `CADET /3d`. Use
`CADET /record /classic` or `CADET /record /3d` to capture timing and
confirm the enhanced appearance does not produce an unacceptable slowdown.
The two builds use the **same** collision and gameplay logic.

## What comes from the original

* Actual playfield bitmap, default-state object artwork, ball sprite, and
  resting/raised flipper sprites and eight plunger frames from PINBALL.DAT; scaled and quantized offline
  to the fixed RGBI palette. Display aspect assumes a 4:3 Tandy monitor.
* Original camera projection, ground-level static wall paths, flipper endpoints,
  plunger feed/animation positions, kickback strike centers and drain height.
  All seven bumper caps have scoring collision outlines fitted to their artwork.
* The original `TPlunger.cpp` charges in timed 25ms steps, advances the
  pullback sprite as charge grows, and gives the released ball a small
  random addition to launch boost (up to 10% in the original). DOS keeps
  this hold/release behavior in integers and narrows the variation to about
  1% to maintain the imported shooter-lane trajectory.
* Three-ball game length (`TPinballTable.cpp`) and initial bumper scores of
  500 / 1500 (`control.cpp`). These constants do not implement the complete
  original rule state machine.
* The normal-component reflection principle used by `maths::basic_collision`.
  The implementation is new fixed-point C, not a recompilation of that function.

`IMPORT.json` records the source hash, imported and omitted collision objects,
and simplifications for each conversion. Original resources are not needed
to compile the executable. This fork includes converted Space Cadet resources
in `data/` from the owner-supplied Windows resource file, and packages them in
the CI download. See `data/README.md` for provenance and licensing boundaries.

## Deliberate prototype simplifications / limitations

* Physics runs in projected screen coordinates (Q6), not the original 3D world.
  Gravity, speed limits, restitution and flipper impulses are tuned constants.
  Spin, material-specific behavior, stochastic drag and continuous swept
  collision detection are absent. The shooter speed limits reach about
  2.06 pixels/substep vertically; powered playfield motion can reach about
  2.75 pixels/substep, before collision corrections.
* All seven source bumper caps have original artwork and imported eight-edge
  footprints. In F-004, the runtime now precomputes one **radial hit surface**
  per cap, with a consistent outward normal. This follows the original
  `TBumper` circular contact principle and prevents the old eight two-sided
  segments from pushing the ball into their shared interior. The three
  elevated launch bumpers are still flattened into one plane, so the source
  collision-layer behavior is incomplete. Most remaining static segments are
  two-sided capsules; collision groups are not reproduced. The upper-left skill-shot
  throat now has a small downhill-only passage, approximating the original
  TOneway gates so a ball does not get trapped by flattened ramp walls.
  The original projected outer-right rail is now also identified from
  `TABLE.DAT`: if a fast ball slips outside that rail (as observed on the
  Tandy at frames 1432-1455), a one-sided normal reflection catches it
  at the *table* edge, not the screen edge. This is a temporary F-002
  playability guard, not an imported replacement for the original gate
  or collision-layer rules. The extreme top and side screen limits still
  reflect instead of consuming balls as an emergency fallback.
  The full ramp/one-way mechanics and final hardware verification remain
  incomplete. Other original gates and one-way edges still need explicit
  data import.
* Missed shots that return down the diagonal shooter tube return to the
  plunger without using a ball. The game tracks whether the ball has entered
  the main playfield, then catches a weak shot near the tube's lower end
  (before the shared drain line). Normal playfield drains still count.
* Ramps, elevation/occlusion, holes, sinks, gates, kickouts and rollover/target
  rules are not implemented. Their initial artwork can still be visible.
  Ground-only filtering prevents upper-level rails becoming invisible walls.
* The plunger uses eight original frames at their imported positions, with
  integer charge selecting the frame. Animation uses small dirty rectangles.
* Original rest/raised flipper endpoints are retained. Collision checks also
  cover a midpoint during a seven-substep fixed-point sweep, approximating
  the original moving flipper without trigonometry. **Powered contact now
  adds a normal-direction impulse scaled by pivot distance to the reflected
  incoming velocity**, rather than replacing it with a largely fixed shot.
  Playfield shots can reach 176 Q6 vertically; the shooter retains its
  tuned 132 Q6 cap. This prioritizes the hardware-reported need for more
  lively, controllable flippers. It is source-inspired, not exact angular
  physics; see [F-006](FIDELITY.md#f-006-flipper-and-collision-response).
  The artwork still has only two states. Timing curves and accurate trapping
  are not yet reproduced. Static lights do not animate.
* Single ball/player, basic bumper scoring, drain/relaunch and restart only.
  Missions, jackpots, rank, bonus, skill-shot and multiball remain future work.
  A V20-friendly approximation of original nudge.cpp now applies additive
  velocity, its inverse after about 0.4 seconds, three bump directions
  (A, D, X), and a decaying tilt meter with warning and flipper lockout
  until the ball drains. This is not a 3D rigid-body simulation.
* Both outlane kickbacks use a short wind-up, then throw the ball upward
  and inward. Each side gets one save per ball. The original named Hazard
  Target triplets and gate enable/disable rules are not yet imported, so
  their per-side recharge logic remains an intentional approximation.
  Strike centers come from the source collision segments; trigger widths,
  timing and impulses are simplified. Walls stay solid throughout the return.
  The HUD shows `KICK L R`, or a dash for a consumed side.
* Reduced precision/two-sided walls can still trap the ball; nudge with X,
  A, or D to free it, but repeated nudges can tilt.
* Short synthesized Tandy PSG tones replace original samples and music.
* Whole-table view prioritizes memory and frame cost over fine visual detail.

## Tests and performance

`TEST.EXE` runs the same core using actual 16-bit arithmetic: progressive
plunger pullback, weak and full launch, bounded variation, wall reflection,
bumper scoring, shooter-lane return versus regular drain, left/right flippers,
contact-dependent flipper leverage and incoming-momentum response, more energetic
playfield speed, three-direction nudges and return impulses, tilt, both timed
outlane kickbacks and single-use gating, drain/restart and a 20,000-step soak. It writes `TEST.OK` on success.
`CADET /smoke` runs 360 timed frames, writes `SCREEN.BIN` (raw video memory)
and `SMOKE.TXT`, and restores DOS. It exercises the backend but does not prove
physical key responsiveness or real V20 timing.
`TEST /original` additionally checks all seven bumpers, both kickbacks at
three entry offsets, original plunger frame positions, 12 partial-charge
shooter-return trajectories, successful recharging/relaunching of a returned
ball, and full-power scoring shots. Tilted flippers
retain passive resting collisions. The deterministic smoke test must complete
360 frames / 1440 steps and a fully charged launch; with `--original`, the
runner also requires a bumper hit and at least 500 points. Python tests reject
incomplete, uncharged and zero-score smoke reports.
`CADET /record` uses real keyboard input and writes `INPUT.TXT` on Esc,
including observed control bits, launches, nudges, kickbacks, tilts, top/side
safety contacts (`boundary_hits`), source outer-right-rail catches
(`right_rail_saves`), and slow-frame count for hardware checks.
It also writes **`FLIGHT.TXT`**, a rolling trace of the **last 256 rendered
frames (about 14 seconds)**: Q6 ball position and velocity, remaining balls,
READY/shooter status, charge, event code and both separate cumulative
`BORDER` and `RIGHT_RAIL` recovery counters (flight schema 3).
Flight data is buffered in memory,
**not written to disk during gameplay**, so normal play is unaffected. If the
ball disappears near the upper-left chute, press **Esc immediately** and
inspect/share `FLIGHT.TXT` and `INPUT.TXT`. These show whether it exited the
screen, drained, or remained inside an unexpected collision area.

The small memory model keeps code and near data individually below 64KB;
the immutable background is one 32KB far allocation. Only changed ball and
flipper rectangles are restored. Collision candidates use 25 narrower 8-pixel vertical bands and
precomputed per-edge bounds before fixed-point dot products. Moving sprites
are drawn scanline-by-scanline without per-pixel video address division;
stationary flippers are not repainted unnecessarily. No per-frame allocation,
floating point, full-screen blit, or full-table collision scan is used.
Build maps and executable headers are checked by `verify_build.py`.

See [`VALIDATION.md`](VALIDATION.md) for measured emulator results and
remaining hardware checks, and [`FEATURE_STATUS.md`](FEATURE_STATUS.md)
for the complete inventory of missing original Space Cadet features.

## Format and source references

`convert.py` follows `SpaceCadetPinball/partman.h`, `partman.cpp`, `gdrv.cpp`,
`GroupData.cpp`, `loader.cpp`, `proj.cpp`, `TFlipperEdge.cpp`, and
`TEdgeSegment.cpp`. Generated packs are little-endian SCP2: 4-byte magic,
four signed shorts (edge count, launch X/Y, drain Y), 12 flipper coordinate
shorts, four kickback center shorts (left X/Y, right X/Y), nine shorts per
edge, then five ball/flipper sprites followed by eight plunger sprites. Each sprite has X/Y/W/H
shorts and W*H bytes (0..15 or 255 for transparent). Caps are enforced by
both converter and runtime. Coordinates are pixels; tangents Q8 and lengths Q6.

Tandy layout reference: Tandy 1000 Technical Reference Manual, video section
(four 8KB banks, mode 09h). Compiler: https://github.com/open-watcom/open-watcom-v2

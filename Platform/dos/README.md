# Space Cadet: native DOS / Tandy prototype

This is a separate C89, 16-bit real-mode target for an NEC V20 / 8088-class
Tandy 1000. It does not use SDL, Windows, a DOS extender, an FPU, XMS or EMS.
The desktop version is unchanged. This is an initial playable prototype,
not a feature-complete port or a verified 7.16 MHz hardware release.

## Build and play

1. Unpack Open Watcom 2 and set `WATCOM` to its absolute installation directory.
2. From the repository root, run `python Platform/dos/build.py` (Python 3.9+).
3. Copy the three files from `Platform/dos/data` named `TABLE.DAT`, `TABLE.TGA`
   and `IMPORT.json` into `build/dos`, or regenerate them with
   `python Platform/dos/convert.py /path/to/PINBALL.DAT --output build/dos`
   using the Windows Space Cadet resource file. Full Tilt CADET.DAT is not yet
   supported. Alternatively run `python Platform/dos/make_demo.py` to generate
   an asset-free diagnostic table; it is **not** Space Cadet.
4. Copy `CADET.EXE`, `TABLE.DAT` and `TABLE.TGA` from `build/dos` to one DOS
   directory and run `CADET`. TABLE.TGA is a raw Tandy bank image, not Targa.

Controls: **Z** left flipper, **/** right flipper, hold **Space** to pull the
plunger back and release to launch (about 1.3 seconds to full power, with a
small ~1% launch variation), **X** nudge (one-second cooldown),
**S** toggle sound on/off (shown in the right sidebar), **N** new game,
**Esc** quit. Both flippers can be held together. There are three balls.
`/nosound` starts with sound disabled, and **S** can re-enable it during
play. Switching sound off silences any currently playing tone immediately.
Run on Tandy-compatible video;
there is no VGA fallback or reliable automatic Tandy detection in this version.

The native program uses BIOS mode 09h, packed 4bpp B800 video memory with four
8KB banks, and a private IRQ1 make/break keyboard handler. It restores the prior
keyboard vector and video mode on normal exit. Do not terminate the emulator
to test cleanup; use Esc. No PIT reprogramming: the BIOS timer drives 18.2 FPS
with four fixed physics substeps (about 72.8 Hz). Late frames are counted and
dropped rather than running unbounded catch-up work.

## What comes from the original

* Actual playfield bitmap, default-state object artwork, ball sprite, and
  resting/raised flipper sprites from PINBALL.DAT; scaled and quantized offline
  to the fixed RGBI palette. Display aspect assumes a 4:3 Tandy monitor.
* Original camera projection, ground-level static wall paths, bumper centers
  and radii, flipper endpoints, plunger feed position and drain height.
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
  collision detection are absent. A substep caps movement to two pixels/axis.
* Bumpers use eight-sided outlines. Static segments are two-sided capsules;
  original one-way edges and collision masks are not reproduced.
* Missed shots that fall back down the narrow shooter lane near the plunger
  return to launch without costing a ball. For now this uses an 8-pixel
  corridor around the imported launch X at drain height, not a full 3D ramp.
* Ramps, elevation/occlusion, holes, sinks, gates, kickouts and rollover/target
  rules are not implemented. Their initial artwork can still be visible.
  Ground-only filtering prevents upper-level rails becoming invisible walls.
* Until the original plunger sprite states are imported into the SCP1 pack,
  a small silver plunger head and shaft are drawn below the imported shooter
  position, moving downward up to 12 pixels as Space is held. These are
  small dirty-rectangle updates, not a full-screen redraw. On release the
  plunger snaps back and the launch speed reflects the chosen charge.
* Original rest/raised flipper endpoints are retained. Collision checks also
  cover a midpoint during a three-substep fixed-point sweep, approximating
  the original moving flipper without trigonometry. The artwork still has
  only two states. Timing curves and accurate trapping are not yet reproduced.
  Static lights do not animate.
* Single ball/player, basic bumper scoring, drain/relaunch and restart only.
  Missions, jackpots, rank, bonus, skill-shot and multiball remain future work.
  Nudge is a fixed impulse with cooldown; the original tilt rules are absent.
  Reduced precision/two-sided walls can trap the ball; use X to recover.
* Short synthesized Tandy PSG tones replace original samples and music.
* Whole-table view prioritizes memory and frame cost over fine visual detail.

## Tests and performance

`TEST.EXE` runs the same core using actual 16-bit arithmetic: progressive
plunger pullback, weak and full launch, bounded variation, wall reflection,
bumper scoring, shooter-lane return versus regular drain,
left/right flippers, drain/restart and a
20,000-step soak with the current table. It writes `TEST.OK` on success.
`CADET /smoke` runs 360 timed frames, writes `SCREEN.BIN` (raw video memory)
and `SMOKE.TXT`, and restores DOS. It exercises the backend but does not prove
physical key responsiveness or real V20 timing.
`TEST /original` additionally checks that a full-power shot leaves the shooter
lane and reaches a scoring bumper in the included Space Cadet pack.
`CADET /record` uses real keyboard input and writes `INPUT.TXT` on Esc, including
observed control bits and launch count, for manual keyboard/cleanup checks.

The small memory model keeps code and near data individually below 64KB;
the immutable background is one 32KB far allocation. Only changed ball and
flipper rectangles are restored. Collision candidates use 25 narrower 8-pixel vertical bands and
precomputed per-edge bounds before fixed-point dot products. Moving sprites
are drawn scanline-by-scanline without per-pixel video address division;
stationary flippers are not repainted unnecessarily. No per-frame allocation,
floating point, full-screen blit, or full-table collision scan is used.
Build maps and executable headers are checked by `verify_build.py`.

See `VALIDATION.md` for measured results and remaining hardware checks.

## Format and source references

`convert.py` follows `SpaceCadetPinball/partman.h`, `partman.cpp`, `gdrv.cpp`,
`GroupData.cpp`, `loader.cpp`, `proj.cpp`, `TFlipperEdge.cpp`, and
`TEdgeSegment.cpp`. Generated packs are little-endian SCP1: 4-byte magic,
four signed shorts (edge count, launch X/Y, drain Y), 12 flipper coordinate
shorts, nine shorts per edge, then five sprites. Each sprite has X/Y/W/H
shorts and W*H bytes (0..15 or 255 for transparent). Caps are enforced by
both converter and runtime. Coordinates are pixels; tangents Q8 and lengths Q6.

Tandy layout reference: Tandy 1000 Technical Reference Manual, video section
(four 8KB banks, mode 09h). Compiler: https://github.com/open-watcom/open-watcom-v2

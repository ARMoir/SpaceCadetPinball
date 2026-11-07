# Original-source mapping for the DOS / Tandy port

**Target:** 16-bit C89 real mode, Tandy 1000 EX with 7.16 MHz NEC V20.
**Pre-refactor baseline:** original `game.c` blob `be9f3f0927331f1ef6ac59aba47348ffc366ff6d`;
original `main.c` blob `564dbacbe79c5e4001bee44f650a3509b5b664e5` (2026-11-07).
**Authority:** [FIDELITY.md](FIDELITY.md) is the exception register;
[FEATURE_STATUS.md](FEATURE_STATUS.md) tracks unimplemented gameplay.

## Classification and limits

- **MATCHES:** a narrow, source-checked behavior or asset relationship, never entire class parity.
- **ADAPTED:** source intent exists, but algorithms, state, timing or geometry differ.
- **DOS-ONLY:** platform-specific replacement for original desktop facilities.
- **MISSING:** source behavior not implemented, never called an optimization.
- **UNVERIFIED:** source equivalence or real Tandy/V20 outcome not yet demonstrated.

All gameplay modules below are **ADAPTED**, not fully matching upstream;
specific aspects in the MATCHES column are more limited than whole functions.

## File and original function map

| DOS implementation | Original code to consult | Matches (limited) | Diverges (F-###) |
| --- | --- | --- | --- |
| [`table.c`](table.c) `table_load`, `table_index`, `index_slings` | [`loader.cpp`](../../SpaceCadetPinball/loader.cpp); [`proj.cpp`](../../SpaceCadetPinball/proj.cpp); [`TEdgeManager.cpp`](../../SpaceCadetPinball/TEdgeManager.cpp); [`TBumper.cpp`](../../SpaceCadetPinball/TBumper.cpp) | Source-derived static geometry, sprite data and offline geometry index | SCP2 reduced 2D layout, no full gate/layer/component semantics (F-002, F-004) |
| [`game.c`](game.c) `integer_sqrt` | [`maths.cpp`](../../SpaceCadetPinball/maths.cpp) | Integer-length calculation for fixed-point contact geometry | No original float/3D math pipeline (F-006) |
| [`state.inc`](state.inc) | [`TPinballTable.cpp`](../../SpaceCadetPinball/TPinballTable.cpp); [`TFlipper.cpp`](../../SpaceCadetPinball/TFlipper.cpp); [`TPlunger.cpp`](../../SpaceCadetPinball/TPlunger.cpp) | three-ball feed and source flipper endpoints | fixed-point three-position flipper and reset conventions (F-003, F-006, F-008) |
| [`ball_collision.inc`](ball_collision.inc) | [`TBall.cpp`](../../SpaceCadetPinball/TBall.cpp); [`TEdgeSegment.cpp`](../../SpaceCadetPinball/TEdgeSegment.cpp); [`maths.cpp`](../../SpaceCadetPinball/maths.cpp) | contact normal, response impulse and correction | Q6/Q8 2D capsules and sampled impulses (F-002, F-006) |
| [`bumper.inc`](bumper.inc) | [`TBumper.cpp`](../../SpaceCadetPinball/TBumper.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp) | radial contact and score events | single layer, fixed score, shared delay (F-004) |
| [`slingshot.inc`](slingshot.inc) | [`TEdgeSegment.cpp`](../../SpaceCadetPinball/TEdgeSegment.cpp); [`TEdgeManager.cpp`](../../SpaceCadetPinball/TEdgeManager.cpp) | source-derived outlines and outward response | solid triangle prevents center trapping (F-002) |
| [`rail_collision.inc`](rail_collision.inc) | [`TEdgeManager.cpp`](../../SpaceCadetPinball/TEdgeManager.cpp); [`TOneway.cpp`](../../SpaceCadetPinball/TOneway.cpp); [`TGate.cpp`](../../SpaceCadetPinball/TGate.cpp) | source rail positions and one-way intent | DOS-only guards and pixel-based upper chute (F-002) |
| [`kickback.inc`](kickback.inc) | [`TKickback.cpp`](../../SpaceCadetPinball/TKickback.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp) | source strike center and delayed firing | automatic one-use arming; missing gate targets (F-001) |
| [`nudge.inc`](nudge.inc) | [`nudge.cpp`](../../SpaceCadetPinball/nudge.cpp); [`pb.cpp`](../../SpaceCadetPinball/pb.cpp) | additive impulse with delayed inverse | tuned Q6 impulses, heat and thresholds (F-005) |
| [`unstuck.inc`](unstuck.inc) | [`control.cpp`](../../SpaceCadetPinball/control.cpp)::UnstuckBall; [`TBall.h`](../../SpaceCadetPinball/TBall.h) | upward release, preserve ball count, exclude capture | Q6 progress watchdog, safe local moves (F-008) |
| [`plunger.inc`](plunger.inc) (inline in `game_step`) | [`TPlunger.cpp`](../../SpaceCadetPinball/TPlunger.cpp) | Hold/charge/release sequence and imported plunger frames | Faster charge and reduced velocity jitter (F-003) |
| [`flipper.inc`](flipper.inc) (inline in `game_step`) | [`TFlipper.cpp`](../../SpaceCadetPinball/TFlipper.cpp); [`TFlipperEdge.cpp`](../../SpaceCadetPinball/TFlipperEdge.cpp) | Source end positions and left/right activation | Sampled sweep, held impact boost and 5px center-tip assist (F-006) |
| [`control.inc`](control.inc) `game_step` | [`TPinballTable.cpp`](../../SpaceCadetPinball/TPinballTable.cpp); [`pb.cpp`](../../SpaceCadetPinball/pb.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp) | Ball count, collision ordering, scoring events, drain and relaunch | Fixed 72.8 Hz Q6 step, simplified missions, side safety containment (F-001 through F-008) |

### DOS platform modules

| DOS file | Original code to consult | Matches / divergence |
| --- | --- | --- |
| [`video_shake.inc`](video_shake.inc) | [`render.cpp`](../../SpaceCadetPinball/render.cpp) | DOS-ONLY; nudge visual feedback; Tandy CRTC only; VGA disabled (F-007) |
| [`input.inc`](input.inc) | [`options.cpp`](../../SpaceCadetPinball/options.cpp); [`winmain.cpp`](../../SpaceCadetPinball/winmain.cpp) | ADAPTED; Z and /, mouse left/right flippers; DOS IRQ1 / INT33h; middle click not ported (F-007) |
| [`video.inc`](video.inc) | [`render.cpp`](../../SpaceCadetPinball/render.cpp); [`gdrv.cpp`](../../SpaceCadetPinball/gdrv.cpp) | ADAPTED; transparent source sprite compositing; banked B800h or A000h, 16-color, no Z layers (F-007) |
| [`hud.inc`](hud.inc) | [`TTextBox.cpp`](../../SpaceCadetPinball/TTextBox.cpp); [`winmain.cpp`](../../SpaceCadetPinball/winmain.cpp) | ADAPTED; score, balls, game status; compact UI/font, incomplete menus (F-007) |
| [`audio.inc`](audio.inc) | [`Sound.cpp`](../../SpaceCadetPinball/Sound.cpp); [`TSound.cpp`](../../SpaceCadetPinball/TSound.cpp) | DOS-ONLY; event feedback; PSG/PC-speaker substitute rather than samples (F-007) |
| [`assets.inc`](assets.inc) | [`loader.cpp`](../../SpaceCadetPinball/loader.cpp); [`gdrv.cpp`](../../SpaceCadetPinball/gdrv.cpp) | DOS-ONLY; source-derived ball/table assets; SCP2 and DOS-specific Tandy/VGA unpacking (F-007) |
| [`main.c`](main.c) DOS game loop | [`pb.cpp`](../../SpaceCadetPinball/pb.cpp); [`winmain.cpp`](../../SpaceCadetPinball/winmain.cpp) | DOS 18.2 Hz display and 4 substeps, keyboard/game status, diagnostics (F-007) |

## How it is compiled and why

`table.c` is an independently compiled object for file parsing, geometry
indexing and setup. `game.c` retains the time-critical
collision/control code in **one translation unit**, with no new calls
per physics step. Each `.inc` is a source fragment included exactly once;
the `flipper.inc` and `plunger.inc` statements are included *inside*
the original `game_step` at their previous positions. `main.c` also
textually includes its DOS drawing, input, HUD, audio and asset routines.
The C preprocessor sees the same function bodies and statement order as
before the extraction. All these fragments remain `static` where they
were before. This is deliberate to protect the V20's stack, CPU and
memory budget, not an invitation to compile `.inc` files individually.

`game.h` still defines `Game`, `Table`, `Edge`, Q6 constants and
the external API. `build.py` now builds `CADET.EXE` from
`main.c + game.c + table.c`, and `TEST.EXE` from
`test_game.c + game.c + table.c`. There are no new external data files,
no changed `TABLE.DAT` format, and no changes to original table art.

Source order matters: `game_step` still handles input/nudge/charging,
kickback capture, indexed walls, bumpers, slingshot containment, outer
rails, flippers, shooter/drain transitions, then the anti-stuck watchdog.
The renderer still draws restorations, flippers, plunger and ball in
their prior order. The `test_source_layout.py` CI test guards this
assembly; existing Open Watcom and DOSBox-X checks test behavior.

## Important missing original functions

**MISSING:** multiball, mission and rank progression, award/jackpot/bonus,
per-bumper control/scoring states, target/gate progression, full ramp
heights/collision groups, original rotating flipper collision, original
sound/music and high-score/multiplayer UI. These are tracked in
[FEATURE_STATUS.md](FEATURE_STATUS.md) and in the open F-001 through
F-008 entries. Their absence is not a performance optimization.

## Procedure for extending a ported component

1. Inspect original C++ source and resource/group behavior, including input,
   state, timing, side effects and the reverse/negative test cases.
2. Update the module's SOURCE MAP comment and this table with narrowly
   demonstrated **MATCHES** and explicitly documented **DIVERGES**.
   Any gameplay change needs an updated F-### entry in
   [FIDELITY.md](FIDELITY.md). Label hardware claims **UNVERIFIED**
   until real measurements exist.
3. Keep the ordered include layout and fixed-point public structs intact
   unless a measured reason justifies change. Run `test_source_layout.py`,
   the native DOS regression suite, both video modes, executable memory
   checks and physical V20 tests where available.

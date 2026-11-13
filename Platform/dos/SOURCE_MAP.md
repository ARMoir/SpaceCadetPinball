# Original-source mapping for the DOS / Tandy port

**Target:** 16-bit C89 real mode, Tandy 1000 EX with 7.16 MHz NEC V20.
**Pre-refactor baseline:** original `game.c` blob `be9f3f0927331f1ef6ac59aba47348ffc366ff6d`;
original `main.c` blob `564dbacbe79c5e4001bee44f650a3509b5b664e5` (2026-11-07).
**Authority:** [FIDELITY.md](FIDELITY.md) is the exception register;
[FEATURE_STATUS.md](FEATURE_STATUS.md) tracks unimplemented gameplay.
This organization was validated by the
[November 7, 2026 refactor CI run](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888).

## Classification and limits

- **MATCHES:** a narrow, source-checked behavior or asset relationship, never entire class parity.
- **ADAPTED:** source intent exists, but algorithms, state, timing or geometry differ.
- **DOS-ONLY:** platform-specific replacement for original desktop facilities.
- **MISSING:** source behavior not implemented, never called an optimization.
- **UNVERIFIED:** source equivalence or real Tandy/V20 outcome not yet demonstrated.

All gameplay modules below are **ADAPTED**, not fully matching upstream;
specific aspects in the MATCHES column are more limited than whole functions.

## Find the right DOS file

| Change or investigation | Start here | Original behavior / exception |
| --- | --- | --- |
| Ball bounce and wall collision | [`ball_collision.inc`](ball_collision.inc), [`game.c`](game.c) | `TBall.cpp`, `maths.cpp` (F-006) |
| Flipper travel, contact or strength | [`flipper.inc`](flipper.inc), [`state.inc`](state.inc) | `TFlipper.cpp`, `TFlipperEdge.cpp` (F-006) |
| Plunger charging or weak return | [`plunger.inc`](plunger.inc), [`state.inc`](state.inc) | `TPlunger.cpp` (F-003) |
| Bumper hitboxes and scoring | [`bumper.inc`](bumper.inc), [`table.c`](table.c) | `TBumper.cpp`, `control.cpp` (F-004) |
| Slingshot or outer-rail trapping | [`slingshot.inc`](slingshot.inc), [`rail_collision.inc`](rail_collision.inc), [`table.c`](table.c) | `TEdgeManager.cpp`, `TEdgeSegment.cpp`, `TOneway.cpp`; directional lower-apron entry guard, passive backside and excluded interior detail (F-002) |
| Kickback catch, gate and rendering | [`kickback.inc`](kickback.inc), [`control.inc`](control.inc), [`table.c`](table.c), [`lamps.inc`](lamps.inc) | `TKickback.cpp`, `TGate.cpp`, `control.cpp::RightKickerControl`; source-inspired proxy gate reclose (F-001) |
| Nudge and tilt | [`nudge.inc`](nudge.inc), [`control.inc`](control.inc) | `nudge.cpp`, `pb.cpp` (F-005) |
| Stuck-ball rescue | [`unstuck.inc`](unstuck.inc), [`slingshot.inc`](slingshot.inc), [`control.inc`](control.inc) | `control.cpp::UnstuckBall`; position watchdog plus DOS-only alternate sling contact sequence (F-002/F-008) |
| Offline Tandy 3D metal/bumper shading || Offline original-source color-pair Tandy variant | [`source_art.py`](source_art.py), [`test_source_art.py`](test_source_art.py) | Windows `PINBALL.DAT` source colors via VGA1 from [`convert.py`](convert.py); WolfensteinCGA-inspired palette-pair matching, fixed two-color patterns only on flat surfaces; now the default Tandy static table (owner-approved), no change to VGA source-color defaults; build-time conversion, not direct full-color runtime render (F-007) |
 [`enhance_table.py`](enhance_table.py), [`test_enhance_table.py`](test_enhance_table.py) | Source `PINBALL.DAT` table bitmap via [`convert.py`](convert.py); multiple original-neutral metallic lighting passes, artifact/pinhole cleanup, then optional source-color VGA1-guided steel-edge sharpening; no runtime paint or geometry changes (F-007) |
| Ball appearance, Tandy/VGA drawing | [`video.inc`](video.inc), [`assets.inc`](assets.inc), [`round_ball.inc`](round_ball.inc), [`main.c`](main.c) | `render.cpp`, `gdrv.cpp`; symmetrical DOS display sprite, original source art via `/sourceball` (F-007) |
| Original title/splash image, paused DOS menu | [`build_menu_art.py`](build_menu_art.py), [`data/menu_art.b64`](data/menu_art.b64), [`hud.inc`](hud.inc), [`main.c`](main.c) | `SpaceCadetPinball/splash_bitmap.bmp`, `winmain.cpp` and supplied 600x600 Space Cadet art; RGBI conversion / 240x200 static splash adaptation (F-007) |
| Menu/help, mouse, sound | [`hud.inc`](hud.inc), [`score_font.h`](score_font.h), [`input.inc`](input.inc), [`audio.inc`](audio.inc) | `winmain.cpp`, `options.cpp`, `TTextBox.cpp`, `score.cpp`, `Sound.cpp` (F-007) |
| Gameplay lamps | [`lamps.inc`](lamps.inc), [`bumper.inc`](bumper.inc), [`main.c`](main.c) | `TLight.cpp`, `TLightGroup.cpp`, `control.cpp`; event-driven 16-color sampled lamps, not the original mission-driven lamp state (F-007) |
| High-score table, initials and file persistence | [`scores.c`](scores.c), [`scores.h`](scores.h), [`hud.inc`](hud.inc) | `high_score.cpp` (F-009) |

## File and original function map

| DOS implementation | Original code to consult | Matches (limited) | Diverges (F-###) |
| --- | --- | --- | --- |
| [`table.c`](table.c) `table_load`, `table_index`, `index_slings` | [`loader.cpp`](../../SpaceCadetPinball/loader.cpp); [`proj.cpp`](../../SpaceCadetPinball/proj.cpp); [`TEdgeManager.cpp`](../../SpaceCadetPinball/TEdgeManager.cpp); [`TBumper.cpp`](../../SpaceCadetPinball/TBumper.cpp) | Source-derived static geometry, sprite data and offline geometry index | SCP2 reduced 2D layout, no full gate/layer/component semantics (F-002, F-004) |
| [`scores.c`](scores.c) `scores_load`, `scores_save`, `scores_rank`, `scores_insert` | [`high_score.cpp`](../../SpaceCadetPinball/high_score.cpp) | Five ranked scores, stable insertion, load/save and name entry | DOS local `SCORES.DAT`, three-character initials instead of long names and config-backed source (F-009) |
| [`game.c`](game.c) `integer_sqrt` | [`maths.cpp`](../../SpaceCadetPinball/maths.cpp) | Integer-length calculation for fixed-point contact geometry | No original float/3D math pipeline (F-006) |
| [`state.inc`](state.inc) | [`TPinballTable.cpp`](../../SpaceCadetPinball/TPinballTable.cpp); [`TFlipper.cpp`](../../SpaceCadetPinball/TFlipper.cpp); [`TPlunger.cpp`](../../SpaceCadetPinball/TPlunger.cpp) | three-ball feed and source flipper endpoints | fixed-point three-position flipper and reset conventions (F-003, F-006, F-008) |
| [`ball_collision.inc`](ball_collision.inc) | [`TBall.cpp`](../../SpaceCadetPinball/TBall.cpp); [`TEdgeSegment.cpp`](../../SpaceCadetPinball/TEdgeSegment.cpp); [`maths.cpp`](../../SpaceCadetPinball/maths.cpp) | contact normal, response impulse and correction | Q6/Q8 capsules, swept flipper guard, sampled impulse, bounded impact-only tangential variation in place of missing material/spin (F-002, F-006) |
| [`bumper.inc`](bumper.inc) | [`TBumper.cpp`](../../SpaceCadetPinball/TBumper.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp) | radial contact and score events | single layer, fixed score, shared delay, radial contact with small impact-only tangential variation and bounded overlap recheck (F-004/F-006) |
| [`slingshot.inc`](slingshot.inc) | [`TEdgeSegment.cpp`](../../SpaceCadetPinball/TEdgeSegment.cpp); [`TEdgeManager.cpp`](../../SpaceCadetPinball/TEdgeManager.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp)::UnstuckBall | source-derived outlines and outward response | solid triangle keeps the interior out; outer/back surfaces slide instead of injecting a sling bounce, duplicate inner art excluded (F-002); third alternating front-face hit triggers near-vertical anti-loop exit (F-008) |
| [`rail_collision.inc`](rail_collision.inc) | [`TEdgeManager.cpp`](../../SpaceCadetPinball/TEdgeManager.cpp); [`TOneway.cpp`](../../SpaceCadetPinball/TOneway.cpp); [`TGate.cpp`](../../SpaceCadetPinball/TGate.cpp); [`TRamp.cpp`](../../SpaceCadetPinball/TRamp.cpp) | source rail positions and one-way intent | DOS-only exterior rail/chute guards, group-105 elevated collision-mask filter at upper-right crest, source-path 90% assist for rising group101 and directional line crossings, lower-sling apron backflow deflector (F-002) |
| [`kickback.inc`](kickback.inc), [`control.inc`](control.inc), [`table.c`](table.c) | [`TKickback.cpp`](../../SpaceCadetPinball/TKickback.cpp); [`TGate.cpp`](../../SpaceCadetPinball/TGate.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp) | source strike center, delayed fire and imported TGate entrance close after use (CMP2) | automatic one-use arming and early 0.12s gate transition; guided return and missing Hazard Target gating (F-001) |
| [`nudge.inc`](nudge.inc) | [`nudge.cpp`](../../SpaceCadetPinball/nudge.cpp); [`pb.cpp`](../../SpaceCadetPinball/pb.cpp) | additive impulse with delayed inverse | tuned Q6 impulses, heat and thresholds (F-005) |
| [`unstuck.inc`](unstuck.inc) | [`control.cpp`](../../SpaceCadetPinball/control.cpp)::UnstuckBall; [`TBall.h`](../../SpaceCadetPinball/TBall.h) | upward release, preserve ball count, exclude capture | Q6 progress watchdog, safe local moves (F-008) |
| [`plunger.inc`](plunger.inc) (inline in `game_step`) | [`TPlunger.cpp`](../../SpaceCadetPinball/TPlunger.cpp) | Hold/charge/release sequence and imported plunger frames | Faster charge and reduced velocity jitter (F-003) |
| [`flipper.inc`](flipper.inc) (inline in `game_step`) | [`TFlipper.cpp`](../../SpaceCadetPinball/TFlipper.cpp); [`TFlipperEdge.cpp`](../../SpaceCadetPinball/TFlipperEdge.cpp) | Source end positions and left/right activation | Sampled sweep, bounded incoming ball-path crossing guard, held impact boost and 5px center-tip assist (F-006) |
| [`control.inc`](control.inc) `game_step` | [`TPinballTable.cpp`](../../SpaceCadetPinball/TPinballTable.cpp); [`pb.cpp`](../../SpaceCadetPinball/pb.cpp); [`control.cpp`](../../SpaceCadetPinball/control.cpp) | Ball count, collision ordering, scoring events, drain and relaunch | Fixed 72.8 Hz Q6 step, source-width-aware 5px elevated ramp105 corridor, rising ramp101 3px and back-lane directional guards, simplified missions, side safety containment (F-001 through F-008) |

### DOS platform modules

| DOS file | Original code to consult | Matches / divergence |
| --- | --- | --- |
| [`video_shake.inc`](video_shake.inc) | [`render.cpp`](../../SpaceCadetPinball/render.cpp) | DOS-ONLY; nudge visual feedback; Tandy CRTC only; VGA disabled (F-007) |
| [`input.inc`](input.inc) | [`options.cpp`](../../SpaceCadetPinball/options.cpp); [`winmain.cpp`](../../SpaceCadetPinball/winmain.cpp) | ADAPTED; original left/right mouse flippers, middle mouse launch, F5 sound and F8 controls; DOS IRQ1 / INT33h, initials scan queue (F-007/F-009) |
| [`video.inc`](video.inc) | [`render.cpp`](../../SpaceCadetPinball/render.cpp); [`gdrv.cpp`](../../SpaceCadetPinball/gdrv.cpp) | ADAPTED; transparent source sprite compositing; banked B800h or A000h, 16-color, no Z layers (F-007) |
| [`hud.inc`](hud.inc) | [`TTextBox.cpp`](../../SpaceCadetPinball/TTextBox.cpp); [`high_score.cpp`](../../SpaceCadetPinball/high_score.cpp); [`winmain.cpp`](../../SpaceCadetPinball/winmain.cpp) | ADAPTED; 3x5 C89 text glyphs with legible R/P and distinct M/N, separate original numeric score digits, independently dirtied score, ball, command and status patches plus full view composition; not original prioritized text queue or long-name dialog (F-007/F-009) |
| [`audio.inc`](audio.inc) | [`Sound.cpp`](../../SpaceCadetPinball/Sound.cpp); [`TSound.cpp`](../../SpaceCadetPinball/TSound.cpp) | ADAPTED; eight priority-based short PSG/PC-speaker sequences for gameplay actions; original multichannel WAV sources not imported (F-007) |
| [`lamps.inc`](lamps.inc) | [`TLight.cpp`](../../SpaceCadetPinball/TLight.cpp); [`TLightGroup.cpp`](../../SpaceCadetPinball/TLightGroup.cpp) | ADAPTED; event-timed color lamps at existing source bumpers, sling cores, kickbacks and shooter; no original sprites, switches, mission groups or light scripts (F-007) |
| [`assets.inc`](assets.inc) | [`loader.cpp`](../../SpaceCadetPinball/loader.cpp); [`gdrv.cpp`](../../SpaceCadetPinball/gdrv.cpp) | DOS-ONLY; original source assets and SCP2/VGA1 validation remain unchanged (F-007) |
| [`round_ball.inc`](round_ball.inc) | [`render.cpp`](../../SpaceCadetPinball/render.cpp) | ADAPTED; unshaded 6x5 circle at 320x200 physical CRT aspect; `/sourceball` restores original 4x4 (F-007) |
| [`main.c`](main.c) DOS game loop | [`pb.cpp`](../../SpaceCadetPinball/pb.cpp); [`winmain.cpp`](../../SpaceCadetPinball/winmain.cpp) | DOS 18.2 Hz display and 4 substeps, keyboard/game status, diagnostics (F-007) |

## How it is compiled and why

`table.c` is an independently compiled object for file parsing, geometry
indexing and setup; `scores.c` is another separate C89 object for rare
startup/finished-game/menu operations and file persistence. `game.c` retains the time-critical
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
the gameplay API. `scores.h` supplies the separate top-five C89 API.
`build.py` builds `CADET.EXE` from
`main.c + game.c + table.c + scores.c`, and `TEST.EXE` from
`test_game.c + game.c + table.c + scores.c`. There are no new external data files,
no changed `TABLE.DAT` format, and no changes to original table art.

Source order matters: `game_step` still handles input/nudge/charging,
kickback capture, indexed walls, bumpers, slingshot containment, outer
rails, flippers, shooter/drain transitions, then the anti-stuck watchdog.
The renderer still draws restorations, flippers, plunger and ball in
their prior order. The `test_source_layout.py` CI test guards this
assembly; `test_menu_font.py` reads the relocated `hud.inc`
and `input.inc` instead of searching only `main.c`. The
[passing refactor workflow](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888)
also verifies native Watcom compilation, MZ/DGROUP limits, gameplay
smoke and Tandy/VGA final-frame parity. These tests do not verify
real 7.16 MHz V20 performance or full original-rule parity.

## Important missing original functions

**MISSING:** multiball, mission and rank progression, award/jackpot/bonus,
per-bumper control/scoring states, target/gate progression, full ramp
heights/collision groups, original rotating flipper collision, original
sound/music, complete Windows high-score dialog and multiplayer UI.
Basic persistent DOS top-five scores are implemented (F-009). The
remaining differences are tracked in
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

## VGA source-color presentation update (2026-11-08)

`convert.py --vga` follows `gdrv.cpp::display_palette` / `ApplyPalette` to
write VGA1 artwork and palette without changing SCP2. `assets.inc` validates
and loads it into the existing VGA background/sprite storage; `video.inc`
programs its DAC, retaining RGBI HUD indices. The thirteen sprite dimensions
and transparency masks match SCP2. `main.c` adds `/rgbi` for fallback/parity.
This supersedes the earlier statement that VGA uses only RGBI. See F-007:
320x200 downsampling and reserved UI colors remain presentation adaptations.

## Static crossing guards (2026-11-08)

`ball_collision.inc::guard_wall_crossing` follows the movement-ray intent of
`TLine::FindCollisionDistance` and `TEdgeManager::TestGridBox`, using Q6
signed-plane/segment checks on existing nearby static candidates. It retains
the approached side and passive reflection; it is not original continuous
3D/mask collision processing. `control.inc` also rechecks slings/outer rails
only after a paddle changes ball position. See the F-002 entry and tests.

## CMP1 original component import (2026-11-08)

`component_import.py` is owned by `convert.py`: TRamp.cpp attributes 1300,
1301/1302 and maths::find_closest_edge produce two ordered midpoint paths;
TOneway.cpp/TLine.cpp produce projected world-offset line pairs and masks.
`data/IMPORT.json` retains raw planes, ordered boundaries, group IDs and
source hash, plus the actual TGate 501/502 lines for comparison.
`table.c::read_components` validates the optional CMP1 tail and derives
bounds. `rail_collision.inc::assist_ramp_crest` tracks bounded path progress;
`collide_oneways` handles finite directional crossings. `state.inc` clears
traversal on feed and `main.c` reports the saturated counters.

MATCHES: source component identity, triangle adjacency, ordered entry/exit
matching, one-way line offset/direction and blocked-versus-pass distinction.
ADAPTED/MISSING: ground-only mask activation outside shooter, 2D restitution,
path-based forgiveness rather than FieldEffect, no height/control callbacks.
F-001 correction: the prior four-edge gate index/overlay is a DOS proxy,
not the actual source TGate geometry. See F-001/F-002 for exact limitations.
The existing `.inc` order is unchanged. CMP1 adds no runtime file beyond
TABLE.DAT and old executables reject the extended pack.

## Spent kicker and score orientation repair (2026-11-09)

`component_import.py::pack_components` adds actual TGate lines to CMP2;
`table.c::read_components` loads them while retaining CMP1 support.
`kickback.inc::collide_spent_gates` uses TGate enable/disable intent and
existing passive capsule/swept response; `control.inc` calls it before catch
and after positional contacts. Old proxy edges no longer collide in CMP2.
`lamps.inc::kicker_gates_render` draws the same diagonal entrance segment.
`unstuck.inc::stuck_allowed` no longer treats an in-play right kicker pocket
as a captured shooter merely because it is near the plunger. F-001/F-008
remain open for original target/timer rules and exact recovery semantics.
`score_font.h` corrects all packed digits offline by 180-degree rotation;
`hud.inc` uses those bytes unchanged for score, ball and player. F-007 tracks
low-resolution adaptation; runtime rendering cost and data size are unchanged.

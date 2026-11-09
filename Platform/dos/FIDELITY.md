# Original-game fidelity contract and audit (DOS / Tandy)

**Priority: P0 traceability requirement, subordinate only to real Tandy playability.**
Reviewed **2026-11-07**. Original audit baseline:
[`f78f87c`](https://github.com/ARMoir/SpaceCadetPinball/commit/f78f87c649edff4b4f0d02bdb65074ea4cfa3e9d);
the exception register below tracks later changes.
This is the standing design and review standard for `Platform/dos/`, **not**
a claim that the prototype is already faithful. For a definitive inventory
of features that work, are simplified, or are **not implemented**, see
[`FEATURE_STATUS.md`](FEATURE_STATUS.md).

## Project goal hierarchy

1. **Number one: playable and fun on the Tandy 1000 EX/V20.** Real 7.16 MHz
   responsiveness, reliable flipper contact, lively trajectories, correct
   relaunches, and lack of trapping are the highest decision criteria.
2. **Number two, very close: fidelity to the original Space Cadet.** Prefer
   source-defined physics, mechanics, and assets; avoid inventing gameplay
   differences merely for convenience. The two goals usually reinforce one
   another.
3. **When the goals conflict:** choose the smallest change that makes play
   enjoyable and reliable on real hardware; log original behavior, the
   specific playability regression or CPU/memory limit, evidence (hardware
   observation, if available), affected tests, and a restoration approach.
   **When original mechanics are not yet reproduced accurately and a
   simplification must be chosen, err toward an easier, enjoyable player
   experience rather than unforgiving misses caused by incomplete math.**
   Preserve genuine skill and the ability to lose a ball; do not silently
   invent unlimited saves. Never present an unmeasured gameplay workaround
   as an optimization.

## Non-negotiable rules

1. **Original gameplay is the specification when compatible with playability.** Prefer the original Space Cadet
   logic and resource data in `SpaceCadetPinball/` and Windows `PINBALL.DAT`.
   Preserve component state, scoring, triggers, collision direction, timers,
   activation/deactivation, randomness, and game progression whenever practical.
   Do not invent replacement rules merely because they are simpler to code.
2. **Optimize implementation before changing behavior.** For the ~7.16 MHz NEC V20,
   implement equivalent rules with C89, fixed-point/int arithmetic, precomputed
   tables, reduced drawing, bounded memory, and efficient broad-phase collision
   checks. Keep timings in real/game-time units, even if counters use substeps.
3. **No silent fidelity regressions.** If original behavior causes a
   demonstrated playability problem on the physical Tandy or exceeds its
   budget, choose a narrowly scoped adaptation and add/update an entry in the
   **Exception register below in the same change**. Capture original source references, exact divergence,
   technical reason, evidence, affected tests and a route back to fidelity.
   An actual hardware report of poor gameplay is a legitimate reason to
   investigate a change, though the replacement still needs validation.
   "Easier" and speculative performance improvements do not justify changing
   gameplay rules. An unmeasured limitation must be labeled *unverified*.
4. **Separate missing work from deliberate optimization.** An unported feature
   is **incomplete**, not "optimized". A temporary behavioral workaround is an
   **unapproved approximation** until justified and recorded. Do not claim
   faithful parity based only on successful synthetic tests.
5. **Prefer original data and deterministic comparisons.** Import source
   geometry, sprites, timing constants and object metadata whenever possible.
   Add tests that compare meaningful original behavior, including negative
   cases, rather than tests that merely codify the current workaround.
6. **Validate both environments.** Run converter tests, 16-bit Open Watcom
   builds, DOSBox-X core/video checks, and when available tests on actual Tandy
   1000 EX/V20 hardware. Use `CADET /record` for timing/input diagnostics.
   Passing DOSBox-X is not proof of 7.16 MHz responsiveness or correctness.
7. **Project workflow:** all work goes directly to
   `dos/tandy-native-prototype`; do not change `master`, create another
   branch, or open a pull request unless the project owner explicitly requests
   that. This policy is specific to this repository.

**Change rule:** Read this file *before* modifying gameplay, conversion,
physics, scoring, timing, rendering, or controls. Reference the relevant
exception IDs and original source paths in the commit message or adjacent
code documentation. Include the original-vs-DOS behavior and test evidence in
the change summary. Update the table and log in this file whenever the
fidelity status changes.

## Source of truth

**Input fidelity addition (2026-11-07):** the original desktop
[`options.cpp`](../../SpaceCadetPinball/options.cpp) assigns mouse
left/right buttons to the left/right flippers alongside Z and /. The
DOS port now restores these *same default flipper mappings*, using
optional DOS `INT 33h` mouse-driver detection. Held and fast-click
input is translated to the existing `KEY_LEFT`/`KEY_RIGHT` flags.
It does **not** modify flipper impulses, timing, scoring, collision or
the source table, so no gameplay-rule exception is needed. Mouse driver
latency and actual 7 MHz V20 performance remain **unverified** until
hardware testing. `/nomouse` restores keyboard-only polling cost, and
driver-absent systems have no extra mouse polling. **2026-11-07
follow-up:** source `options.cpp` also uses mouse middle button to
launch, F5 to toggle sound and F8 to show controls. DOS now maps these
to the same existing game/menu actions, conditionally polling the
middle button only when an INT 33h driver reports at least three
buttons. No physics or rule changes; live driver latency remains
UNVERIFIED.

Original code locations (read alongside any proposed DOS implementation):

| Concern | Original reference | Current DOS reference |
| --- | --- | --- |
| Ball movement / collisions | [`TBall.cpp`](../../SpaceCadetPinball/TBall.cpp), [`maths.cpp`](../../SpaceCadetPinball/maths.cpp), [`TEdgeSegment.cpp`](../../SpaceCadetPinball/TEdgeSegment.cpp) | [`ball_collision.inc`](ball_collision.inc) included by [`game.c`](game.c), [`convert.py`](convert.py) |
| Flipper sweep | [`TFlipper.cpp`](../../SpaceCadetPinball/TFlipper.cpp), [`TFlipperEdge.cpp`](../../SpaceCadetPinball/TFlipperEdge.cpp) | [`flipper.inc`](flipper.inc) in [`control.inc`](control.inc), [`state.inc`](state.inc) |
| Plunger / feeder | [`TPlunger.cpp`](../../SpaceCadetPinball/TPlunger.cpp) | [`plunger.inc`](plunger.inc), [`state.inc`](state.inc), [`main.c`](main.c) |
| Kickbacks, targets and gates | [`TKickback.cpp`](../../SpaceCadetPinball/TKickback.cpp), [`control.cpp`](../../SpaceCadetPinball/control.cpp), [`TGate.cpp`](../../SpaceCadetPinball/TGate.cpp) | [`kickback.inc`](kickback.inc), [`rail_collision.inc`](rail_collision.inc), [`control.inc`](control.inc), [`convert.py`](convert.py) |
| Bumpers / scoring | [`TBumper.cpp`](../../SpaceCadetPinball/TBumper.cpp), [`control.cpp`](../../SpaceCadetPinball/control.cpp) | [`bumper.inc`](bumper.inc), [`table.c`](table.c), [`convert.py`](convert.py) |
| Nudge / tilt | [`nudge.cpp`](../../SpaceCadetPinball/nudge.cpp), [`pb.cpp`](../../SpaceCadetPinball/pb.cpp) | [`nudge.inc`](nudge.inc), [`control.inc`](control.inc), [`video_shake.inc`](video_shake.inc) |
| Stuck ball recovery | [`control.cpp`](../../SpaceCadetPinball/control.cpp), `control::UnstuckBall`; [`TBall.h`](../../SpaceCadetPinball/TBall.h) `StuckCounter` | [`unstuck.inc`](unstuck.inc) included by [`game.c`](game.c), `watch_stuck` / `stuck_clear_at` |
| Resource import / projection | [`loader.cpp`](../../SpaceCadetPinball/loader.cpp), [`proj.cpp`](../../SpaceCadetPinball/proj.cpp) | [`convert.py`](convert.py), [`data/IMPORT.json`](data/IMPORT.json) |
| High-score ranking/persistence | [`high_score.cpp`](../../SpaceCadetPinball/high_score.cpp) | [`scores.c`](scores.c), [`scores.h`](scores.h), [`hud.inc`](hud.inc) (F-009) |
| Timed status messaging | [`TTextBox.cpp`](../../SpaceCadetPinball/TTextBox.cpp) | [`hud.inc`](hud.inc) (F-007) |

The original game also uses resource-driven component rules. Source constants
should be checked in context, not copied without their event/state transitions.

**Source-aligned 2026-11-07 refactor:** [SOURCE_MAP.md](SOURCE_MAP.md) now
maps the DOS function bodies to original C++ files and lists MATCHES,
DIVERGES, MISSING and UNVERIFIED details. `table.c` is a separate
setup-only C89 compilation unit. Collision, flippers, plunger, bumpers,
kickback, nudge, anti-stuck and control are now named implementation
fragments included once by `game.c`; DOS input/video/HUD/audio are included
once by `main.c`. The C statements and order were verified unchanged after
expanding those fragments, including the inline flipper/plunger branches.
No gameplay rule, format, Q6 constant, sprite, or per-tick function call
was intentionally altered. F-001 through F-008 remain open. This is
**source organization**, not a proof of upstream parity or real V20 timing.
The [source map](SOURCE_MAP.md) is the central navigation index for future
function-level work; update it and the module's provenance comment when
ownership, input/state behavior or fidelity changes. Preserve
`test_source_layout.py` and the source include order. The
[successful post-refactor GitHub Actions run](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888)
passed Open Watcom, source-layout, DOSBox-X Tandy/VGA and final-frame parity,
without certifying physical hardware performance or original-rule parity.

## Audit of the current prototype

| Priority | Area | Original behavior versus current DOS | Fidelity status | Register |
| --- | --- | --- | --- | --- |
| **P0** | Kickback rules | Original kickback component uses ~0.7 s timer plus ~0.1 s follow-up, with left/right Hazard Target triplets and gate state transitions. DOS currently arms each side automatically for one use per ball, catches near an imported strike center, holds ~0.12 s, then fires a fixed upward/inward impulse. Imported strike positions are an improvement but the rules are **not** preserved. | **Unapproved behavioral approximation**, not a measured performance necessity | [F-001](#f-001-kickback-activation-and-timing) |
| **P0** | Collision layers / one-way gates | Original has world-space collision groups, ramps and one-way/gate components. DOS projects collisions into one 2D plane and hard-codes an upper-left downhill-only screen-coordinate exception. The import inventory records omitted one-way gates, targets, sinks and ramps. | **Incomplete** with a temporary geometry workaround | [F-002](#f-002-collision-layer-and-one-way-projection) |
| **P1** | Plunger timing and variance | Original `TPlunger` has eight visual states, 25 ms increments, boost accumulation (approximately 2.5 s to full charge for the Windows configuration), and up to 10% randomized boost. DOS correctly imports eight original frames but charges in ~1.3 s with ~1% jitter to compensate for simplified shooter physics. | **Partially faithful; unapproved behavioral approximation** | [F-003](#f-003-plunger-charge-and-launch-variation) |
| **P1** | Bumper collisions / scoring | Original bumper response is radial and scoring can change with state. DOS precomputes one radial hit surface per imported eight-edge cap to prevent inward polygon trapping, but still flattens three elevated bumpers and uses static 500/1500 scores with shared hit delay. | **Improved contact fidelity; scoring/layers incomplete** | [F-004](#f-004-bumper-projection-and-scoring) |
| **P1** | Nudge and tilt | Original `nudge.cpp` applies additive velocity and an inverse after 0.4 s; `pb.cpp` accumulates nudge pressure over time. DOS preserves that shape but uses its own impulses, nudge-heat thresholds and decay schedule. | **Partially faithful; tuning differs** | [F-005](#f-005-nudge-and-tilt-parameters) |
| **P0** | Trapped ball / anti-stuck | The original defines `control::UnstuckBall`: exclude flipper/plunger capture zones, throw upward, eventually relaunch. DOS uses a small position-progress watchdog and staged, verified local moves before a free relaunch. | **Source-inspired playability safety net; exact source trigger/timing not established** | [F-008](#f-008-universal-trapped-ball-recovery) |
| **P1** | Flipper physics | Original angular flippers collide throughout rotation. DOS preserves endpoints but uses rest/midpoint/raised fixed-point edges and only two art states. | **Performance-oriented approximation; fidelity still unverified** | [F-006](#f-006-flipper-and-collision-response) |
| **P2** | Material physics / gravity | Original ball direction, spin, elasticity, smoothness, group masks and geometry are richer. DOS uses Q6, fixed restitution, 2D gravity, speed clamps and finite substeps. | **Known incomplete port** | [F-006](#f-006-flipper-and-collision-response) |
| **P2** | Audio, layout and effects | Original sound/visual states are richer. DOS has PSG substitutes and 16-color rendering; the compact HUD now displays timed status-event messages but not the original queued TTextBox priorities. Optional TABLE3D.TGA shading is precomputed, and the moving ball is the unchanged source sprite. | **Presentation-only adaptation**, gameplay parity unchanged; CRT benefit unverified | [F-007](#f-007-rendering-audio-and-frame-rate) |
| **P2** | High scores | Original `high_score.cpp` ranks and stores five entries with long names and desktop settings. DOS has a sorted top-five file and three-initial dialog, but not full original settings/UI. | **Partially implemented; DOS storage/UI adaptation** | [F-009](#f-009-high-scores-and-initials) |

**Verified progress, not full parity:** original-source table artwork, eight
plunger frames and kickback strike centers are imported; seven bumper outlines
score; synthetic tests include weak-shot return, kickbacks and 20,000-step
soak. The [post-refactor GitHub Actions run on November 7, 2026](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888)
passed the 16-bit build/memory checks, source-layout regressions, Tandy/VGA
gameplay tests and final-frame parity. This verifies tested trajectories,
**not** original game logic
or real V20 timing. See [`VALIDATION.md`](VALIDATION.md).

## Exception register (append/update whenever rules are broken)

Each entry is an **open fidelity debt** until the original behavior is
restored or a measured platform necessity is documented and approved. Do not
silently treat legacy exceptions as precedent for new ones.

### F-001 Kickback activation and timing

- **Status / priority:** OPEN, P0; no measured performance justification.
- **Original:** `TKickback.cpp` timer values 0.7 s and 0.1 s; `control.cpp`
  left/right Hazard Target triplets drive gate enable/disable and reset paths.
- **DOS deviation:** kickbacks start armed and are automatically available
  once per side per ball, using a ~0.12 s catch/wind-up. Original target
  progression, conditional gate state and full timing are absent.
  **2026-11-08 high-return revision:** the owner requested that left and
  especially right outlane saves send the ball reliably into the upper
  table rather than rebound off flattened lower rails/slings and re-drain.
  Every *earned, already-armed* kickback now follows a two-stage, fixed-point
  upward corridor from its imported strike center: first outside the
  lower slingshot, then inward until y<=48 (upper quarter of playfield).
  Only the troublesome flattened lower rails/slings are bypassed during
  the active return. Radial bumper collisions and normal scoring remain
  enabled, and full static collision resumes near the top. It lasts roughly 0.6 s at 72.8 substeps/s,
  preserves score and ball count, and never grants an unarmed/tilted save.
  The shot can pass over visually rendered 2D walls because the original
  3D layers are not imported: an **explicit forgiving playability deviation**
  from source TKickback, not original collision or gate behavior. Source
  projectile physics/gates and target arming should ultimately replace
  this approximation. Real 7.16 MHz V20 motion/timing is **UNVERIFIED**.
- **2026-11-08 kicker gate restoration (owner observation):** the right
  outlane door was not closing after the kickback fired. Original
  `control::RightKickerControl` sends `TGateEnable` to `gate2` when
  `TKickback::TimerExpired` completes; `TGate::Message` makes the
  source gate's collision edges active and displays sprite 0. The
  previous DOS model only cleared `kick_armed[1]`, leaving its imported
  gate edges *permanently collidable* and showing no closing door.
  DOS now identifies the actual four-edge source gate polygons next
  to both outlanes during `table_index` (left about x57..61/y119..132,
  right x162..167/y119..138). Their indexed edges are ignored while
  the gate is open, then activate when the simplified timed kickback
  finishes. A small source-positioned gray door leaf is drawn on close
  with 3px-wide dirty rectangles in both Tandy/VGA modes, even with
  `/nolights`. Once used, the gate stays closed and the kicker remains
  unavailable until the NEXT ball begins. This matches original
  **gate closure intent**, but source Hazard Target activation and
  exact 0.7+0.1-second TKickback sequence are still MISSING; DOS still
  starts each ball with free open/armed gates and a ~0.12s wind-up.
  Fixture tables lacking imported gate groups do not invent geometry.
  On real Tandy hardware the position, look and closing timing are
  **UNVERIFIED**.
- **Why / evidence:** temporary gameplay functionality with reduced component
  model; **not** demonstrated to be needed for 7 MHz performance.
- **Tests today:** imported strike centers, offsets on both sides,
  one-use gating, controlled right/left y<=48 return trajectories,
  monotonic upper travel and unchanged score/balls, restoration of normal
  wall collisions after the assisted shot, and negative cases for unarmed,
  upward, tilted and shooter-tube approaches. These still test the DOS
  substitute, not full original rules.
- **Remedy:** port target triplet state, activation signals and original
  timer phases with integer substep timers. Test disabled/enabled, consume,
  rearm and tilt/game transitions against original source behavior.

### F-002 Collision layer and one-way projection

- **2026-11-08 upper-ramp / back-lane playability adaptation:** Original
  `TRamp::FieldEffect` applies a plane-specific force and speed-based
  damping, while `TOneway::Collision` and `TGate` use directed
  3D collision groups. The SCP2 DOS projection has neither ramp plane
  attributes nor complete one-way collision masks. After physical
  Tandy feedback that shots stall very close to the upper-right ramp
  crest, `rail_collision.inc` provides a **single, bounded** assist
  when an already-rising, leftward-traveling ball crosses y=38 Q6
  within source-contour x139..166 on the right, or x62..83 on the left.
  The right-hand target is -96 Q6 vertical / -40 Q6 leftward,
  the left target -88 / -28; a stronger shot is not changed. There is
  **no vertical reset, teleport, extra per-tick force or scoring**
  and the source pack remains unmodified. The implementation requires
  source-identified slings and exterior rail anchors so that arbitrary
  test/demo geometry cannot activate these absolute screen zones.
  This is a near-crest playability compromise, **not** actual
  imported ramp plane physics. Real 7.16 MHz V20 improvement is UNVERIFIED.

  The other reported problem is flipper-powered reentry up the narrow
  corridors behind the slingshots. An invisible full-width wall would
  stop valid downward returns and might break real kickback paths.
  Instead a directional one-way **lower-sling apron deflector** redirects
  only upward and outward shots near each source hull base (y=bottom+10),
  or immediately after a genuine flipper contact in that apron.
  It gives the ball a modest inward horizontal velocity of +/-28 Q6,
  preserving its upward velocity and exact position. Backside sliding,
  downward returns, strong inward shots, shooter travel and earned
  kickback launches remain eligible to pass. No art, score, lives or
  assets are changed. The gate has no permanent solid hitbox.
  Two lightweight Game counters support positive/negative original-pack
  regressions. Long-term source-fidelity remedy is to import TRamp
  plane forces, TOneway and gate-group metadata with geometry rather
  than rely on fixed projected zones. Actual real-hardware performance,
  visual behavior and interactions with every ramp are **UNVERIFIED**.

- **Status / priority:** OPEN, P0; performance impact of fuller import unmeasured.
- **Original:** `TEdgeSegment.cpp`, `TGate.cpp`, one-way collision components,
  ramp/layer occlusion and source collision group masks.
- **DOS deviation:** 521 projected 2D edges, omitted dynamic components,
  selective upper-left downhill bypass based on pixel coordinates, and
  flattened upper bumper layers. **2026-11-07 playability repair:** after
  adjacent static-wall collisions, radial bumpers separate the ball again
  so missing level masks cannot push it into the upper-left bumper cap.
  Rare excursions past the *screen* top/side boundaries now reflect with
  reduced energy instead of instantly draining; this outer containment is a
  **temporary DOS safety rail**, not a substitute for source world-space
  walls, ramps or source gate/layer states.
  **2026-11-07 right-edge hardware finding:** owner-provided
  `FLIGHT(2).TXT` at frames 1432-1455 records a fast ball moving from
  x=164 to x=235 across the imported outer right rail (x about 181 at
  y=100), requiring two emergency screen-edge recoveries at frames 1440
  and 1455. It neither nudged nor tilted. The two-sided projected capsule
  edges sometimes let a fast ball escape the physical boundary of the
  table. DOS now chooses the source-projected long exterior right edge
  during `table_index`, then applies a **one-sided normal reflection**
  *only if* the ball has already crossed that edge after regular wall
  collisions, with an approximate 2px clearance. This avoids bouncing
  into the HUD, without teleporting the ball to the launcher or
  introducing randomness. `right_rail_saves` is separately counted
  from the older `boundary_hits` screen fallback. This is a safety
  adaptation, not original collision-layer semantics.
  **2026-11-07 outer-rail consistency pass:** the right exterior guard
  now excludes balls still in the valid shooter tube; the matching original
  *left* rail (projected roughly x=62 at y=40 to x=39 at y=166)
  is located from SCP2 geometry at table-index time. If an in-play ball
  crosses that line, a one-sided outward-normal reflection projects it
  two pixels inside the source rail, symmetrically with the right guard.
  It is active only between that rail's actual source endpoints, not
  across upper ramps, the shooter, lower outlanes or valid drains.
  The left/right counters (`left_rail_saves`, `right_rail_saves`) distinguish
  these responses from the last-resort screen boundary. This is an
  explicitly documented, temporary playability adaptation, **not**
  original 3D collision masks. Runtime costs: inexpensive integer
  interpolation for a rare out-of-bounds candidate, no per-frame memory
  allocation. Final V20 correctness/performance remains **UNVERIFIED**.
  **2026-11-07 slingshot solid-core repair:** on physical Tandy hardware,
  the ball repeatedly entered the **center of the right slingshot**
  (potentially left as well), a trapping artifact from treating the
  closed source outlines as independent two-sided capsules. The
  imported SCP2 has two closed, 14-edge slingshot outlines: approximately
  x=76..91/y=118..143 (left) and x=141..156/y=118..143 (right).
  During `table_index`, DOS now recognizes them by their CLOSED SOURCE
  outlines and relative flipper-pivot positions, builds a convex
  three-face hull with precomputed Q8 outward normals, and removes only
  their conflicting original perimeter capsules from the generic
  collision bands. Near a sling, a ball is projected outside the
  nearest hull face by its full 3px collision radius and reflected
  outward; an upper/side impact gets a small upward release to avoid
  hanging in place. Tandy and VGA use the **same** fixed-point logic.
  The ball cannot occupy the filled slingshot center after a physics
  step even when initially placed inside or moving into the wedge at
  maximum normal playfield speed. There is no invented outer wall,
  altered artwork, random teleport, ball-loss immunity, new scoring
  event or changed data file. Each rescue increments
  `sling_left_rescues` or `sling_right_rescues` in the existing
  `/record` summary. Runtime cost is an inexpensive bounding-box
  rejection except when the ball approaches a sling, with no dynamic
  allocation. This is an intentional playability accommodation for
  flattened 2D physics, not yet a faithful port of original collision
  groups. Real 7.16 MHz V20 timing and the owner's physical
  right-sling trajectory remain **UNVERIFIED**.
- **2026-11-08 slingshot back-face and duplicate geometry fix:** the
  owner observed artificial bounces from the backs of the slingshots
  along the launcher/kicker tube. Original `TEdgeManager` filters active
  collision groups; the DOS solid-hull adaptation treated EVERY face as
  active, including the outside/back facing the outlane. We now retain
  the hull's 3px positional exclusion but remove ONLY incoming normal
  velocity on those back faces (slide, no second powered bounce),
  and omit the anti-loop hit and tip upward kick there. The inward
  front faces keep their normal restitution and genuine bounce counts.
  The source table ALSO contains redundant interior artwork outlines
  (e.g. right near edges 450..453) that were not excluded by the
  old 14-edge loop filter. Fully contained, non-scoring detail segments
  are now excluded from the static wall bands once the convex hull
  is indexed. Source TGate1/TGate2 narrow collision shapes remain
  separate and are enabled only by their actual gate state (F-001).
  This is a source-inspired simplified collision-layer adjustment, not
  a proof that every 3D surface is correctly modeled. Tests check
  face direction, closed-hull containment, true forward hits, source
  gate index discovery and one-way/dynamic collision. Physical V20
  tube-wall trajectories and timing remain **UNVERIFIED**.
- **Why / evidence:** the owner also observed a ball disappear into an
  upper-left highlighted area after a bounce. Whether this was an off-screen
  exit, collision mask/occlusion, or a gameplay drain is **unverified**.
  The old code categorically treated off-screen top/side exits as drains,
  which is unlike the actual below-flipper drain. The temporary containment
  prioritizes preserving a playable ball while source collision-layer import
  is incomplete. Real-mode performance/format limitations remain unmeasured.
- **Tests:** original-data closed left/right sling recognition and
  per-band exclusion, direct center penetration, high-speed entries
  from above/below/left/right and sustained anti-trapping checks,
  alongside synthetic one-way throat, all seven bumper contacts,
  a radial penetration recovery at the upper-left cap, and top/side containment
  versus true lower drain. Original-table right-rail regression starts
  outside the imported source edge near the actual logged y=102,
  requires a safe inward reflection rather than a screen fallback,
  and confirms an inboard ball isn't disturbed. Additional
  original-table tests now cover a left-rail escape, unaffected inboard
  travel, and shooter-lane exemption from the right-rail guard. The
  `/record` flight schema 4 separates `LEFT_RAIL`, `RIGHT_RAIL` and
  `BORDER` recovery counters.
  `CADET /record` now captures the last 256
  rendered frames to `FLIGHT.TXT` (ball Q6 position/velocity, tube flag,
  charge, event, ball count and safety-rail contact counter), **buffered
  in memory until Esc** so gameplay has no disk writes.
- **Remedy:** import one-way direction and layer/group metadata from
  `convert.py` into the pack; use inexpensive per-edge integer filters.
  Test both passage directions, ramp return, both exterior rail sides,
  and the marked upper-left bounce area on real hardware. Compare
  horizontal/tangential velocity before/after real right-rail impacts;
  the guard must preserve normal shots in the shooter and on the playfield.
  Once accurate source barriers prevent all non-drain exits, evaluate
  whether the temporary source-rail and screen safety rails can be removed.

#### F-002 swept-wall and post-paddle guards (2026-11-08)

Source `TLine::FindCollisionDistance` / `maths::ray_intersect_line` and
`TEdgeManager::TestGridBox` test the ball's movement ray, not just its final
position. DOS endpoint capsule checks could push a center crossing a thin
wall onto the wrong side at legal speeds. A new regression reproduced this
before the fix. Indexed static contacts now check a signed-plane crossing
from the previous position, require the intersection to lie on the actual
segment, restore a 1.5px ball clearance on the approached side and reflect
only inward velocity with existing passive restitution. The existing
upper-chute one-way exclusion runs before this check. Endpoints do not gain
infinite barriers. Bumpers/slings retain their separate solid-contact code.

After a flipper changes position, source-derived sling/outer-rail containment
is rechecked so a positional paddle correction cannot undo those constraints.
This is still a 2D adaptation, not original collision groups, full swept
capsules or nearest-hit time integration. It discards the remainder of a
crossing substep rather than integrating the source residual travel time.
There are no extra per-step allocations or full-table scans; extra work is
restricted to existing nearby candidates and position-changing paddle hits.
One new 16-bit counter `wall_crossings` is included in smoke/record reports.

Tests cover both approaches at multiple speeds, diagonal contacts, retreat
and endpoint passage; existing one-way/shooter/drain cases remain mandatory.
Eight seeded 10,000-step games assert source-rail and solid-object bounds
after each step. These checks cover the modeled 2D surfaces, not every omitted
ramp/layer/gate state. Real V20 timing and hardware behavior are UNVERIFIED.
The restoration path remains source collision groups and nearest-time swept
contacts, measured against real Tandy responsiveness.

### F-003 Plunger charge and launch variation

- **Status / priority:** OPEN, P1; no direct performance justification.
- **Original:** `TPlunger.cpp` eight frames, 25 ms charge increments,
  Windows maximum boost 100, and random added boost of up to 10%.
- **DOS deviation:** eight original frames are imported (good), but charge
  saturates in about 1.3 s; launch jitter is only ~1%, tuned for current
  projected shooter path. **2026-11-08 READY-ball alignment correction:**
  the imported collision/launch anchor at (181,141) left the ball visibly
  hovering 15px above the first original plunger sprite (184,156), and
  slightly left of its center. The native DOS feed and same-ball weak-shot
  return now derive a seated resting center (186,153) from the existing
  plunger sprite width/top instead, with a one-row visual clearance
  between the 4px ball and the rod. This is an intentional low-resolution
  2D **position adaptation**, not a change to source `PINBALL.DAT`,
  source sprites, or original launch power. Synthetic/test tables without
  plausible plunger art retain the imported anchor. Physics launches
  from the seated ball to avoid a visible jump at release. Actual 7MHz
  Tandy observations and source 3D seating parity remain unverified.
- **2026-11-08 reported shooter regression and remedy:** After moving
  the resting ball from source collision/launch anchor (181,141) to the
  visible source plunger tip (186,153), the owner observed full-charge
  launches bouncing inside the tube instead of escaping. This exposes
  omitted lower-tube 3D collision layers, not insufficient plunger
  power. The F-003 correction interpolates the first 12px of release
  through six source-shooter-only physics substeps (~83ms), then resumes
  ordinary collision and the *same* -128..-129 Q6 full-charge impulse
  at (181,141). The ball visibly travels instead of teleporting; weak
  returns still feed to the seated ball. Four-seed charged-release
  trajectory tests now require upper-lane exit; real-hardware
  confirmation is **UNVERIFIED**.
- **Why / evidence:** full-power source-like variation currently destabilizes
  simplified 2D shooter trajectories, not a CPU-budget constraint.
- **Tests today:** original plunger/launch anchor and resting-sprite
  alignment, 12 partial-charge returns, full charge reaches a bumper,
  original frame positions. Regression also verifies that after a weak shot
  returns to READY, holding Space for a different charge launches that same
  ball again without decreasing the ball count.
- **Remedy:** restore the original charge curve and boost variance once source
  launcher response/curve and gate geometry work across tested shot strengths.

### F-004 Bumper projection and scoring

- **Status / priority:** OPEN, P1.
- **Original:** `TBumper.cpp` visual/threshold state and source
  `control.cpp` per-component score progression.
- **DOS deviation:** 2026-11-07 update: the original eight-edge outlines
  are still the imported representation and visible artwork is unchanged,
  but each closed bumper group is indexed once as a **radial collision
  primitive**. The old generic two-sided capsules could push a penetrating
  ball inward, especially at the upper-left bumper near a rail.
  The new radius is derived from the imported cap's maximum extent plus the
  ball contact radius (not yet the source world-space material radius).
  All seven are still flattened to one collision plane with fixed initial
  500/1500 scores and a shared cooldown.
- **Why / evidence:** the owner reported the ball trapped almost immediately
  at the upper-left bumper on the real Tandy after the stronger flipper
  update. Circular normal/correction is closer to the original `TBumper`
  interaction and is inexpensive (precompute at load, square bounds then
  integer sqrt only for contact candidates). **Hardware fix unverified.**
  Scoring simplification has not been justified by measurements.
- **2026-11-08 overlapping-cap regression:** The new F-006 paddle guard
  changed otherwise legitimate long-run trajectories and exposed an
  independent F-004 issue during the 80k-step randomized containment
  test (seed 8, step 4757). Imported upper-left radial caps 5 and 6
  overlap by about 1.7px when flattened to one plane. Resolving cap 6
  could push the ball a fraction of a pixel back inside cap 5, which
  had already been checked. A single extra radial recheck now runs
  **only after an actual bumper overlap**. It keeps the existing
  impulse/scoring conditions and restores the no-overlap invariant
  without changing source art or placing a broad artificial wall.
  This is a bounded projected-layer accommodation, not a claim of
  original bumper material or geometry accuracy. V20 timing remains
  **UNVERIFIED**.
- **Tests:** seven independent scoring contacts; targeted penetration and
  inbound hits at the upper-left imported cap, plus a deterministic
  two-cap cascade that previously re-embedded a ball in cap 5.
  All 80k varied-control physics steps must remain outside solid caps.
  Full gameplay/smoke and speed checks remain mandatory.
- **Remedy:** import actual source bumper radius and material parameters when
  collision layers are implemented; keep the radial normal but restore
  per-bumper source scoring state/cooldown. Confirm no remaining trapping on
  real V20 hardware.

### F-005 Nudge and tilt parameters

- **Status / priority:** OPEN, P1.
- **Original:** `nudge.cpp` additive impulse and inverse at 0.4 s;
  `pb.cpp` builds/decays a continuous nudge counter and tilts after a
  threshold. Original renderer shifts the view.
- **DOS deviation:** additive/inverse concept is preserved, but chosen
  impulse sizes, nine-substep debounce, heat +29, decay -1/12 substeps,
  warning 45 and tilt 78 are new constants. CRTC shake is a hardware-specific
  substitute for the renderer shift.
- **Why / evidence:** fixed-point/state-only alternative is appropriate;
  the gameplay thresholds are not measured equivalents.
- **Tests today:** A/D/X impulses, inverse, tilt lockout and status changes.
- **Remedy:** map source accumulation/decay onto integer fixed ticks and
  compare nudge cadence to source tilt/warning timings.

### F-006 Flipper and collision response

- **Status / priority:** OPEN, P1 for flippers and P2 for other physics.
- **Original:** `TFlipperEdge.cpp`, `TFlipper.cpp`, `TBall.cpp` and
  `maths.cpp` rotate through continuous collision space with richer material
  and ball-state handling.
- **DOS deviation:** fixed-point capsules, three-state sampled sweep,
  two-frame flipper art, tuned velocity/elasticity constants and no true
  ramp height/spin. A formerly fixed `vx/vy` flipper shot was replaced
  on 2026-11-07 with an **additive normal impulse** scaled by contact
  distance from the flipper pivot, retaining incoming reflected momentum.
  On moving sweeps the impulse faces the playfield even when an interpolated
  edge has an ambiguous contact side. Playfield velocity is capped at
  176 Q6 vertically (160 horizontally) while the source-sensitive shooter
  path retains its existing 132/128 Q6 caps.
- **F-006 active center-tip forgiveness (2026-11-07):** the owner
  reports that balls fall through the middle so often they cannot
  reasonably reach them, and explicitly requests favoring *easier* play
  whenever simplified source logic is ambiguous. The imported source
  geometry has raised tips around (107,149) and (125,149), leaving an
  18-pixel visual gap; the 2D fixed-point collision and sampled flipper
  sweep cannot reproduce the complete original angular reach or
  continuous ball dynamics. This gap exists in the original pinball
  table too, but actual loss rates cannot be ranked without repeated
  matched gameplay on original and DOS versions. DOS now precomputes
  a **five-pixel inward contact extension** on each raised flipper tip,
  using the same existing capsule collision and ordinary held-flipper
  power, only when the corresponding flipper is held and the ball is
  descending onto its upper face. It is **not** drawn, does **not**
  modify imported coordinates, the lower drain, scoring, or table files,
  and is inactive when a flipper is released, the table tilts, or the
  ball is still in the shooter. The exact center remains drainable even
  with both flippers held. Only a small inboard positional/velocity
  window performs the extra collision check. This is a deliberate
  **easier-than-original-geometry playability concession**, not a claim
  of faithful flipper physics. The assist length is
  `FLIPPER_TIP_ASSIST_PX` in `game.h`, so real-hardware comparison can
  tune or remove it without altering source data. Its speed and feel
  are still **UNVERIFIED** on physical Tandy/VGA DOS machines.
- **2026-11-08 user-observed paddle tunneling:** a ball crossing a thin
  projected flipper between physics samples could be resolved against the
  *underside* by the old two-sided capsule, letting it pass through
  despite correct paddle timing. Inspired by the source
  `TFlipper::FlipperCollision` swept-ray approach and
  `TFlipperEdge::EdgeCollision` response, DOS now retains the
  pre-integration Q6 flight segment and checks for genuine incoming
  **top-face crossings** of the stationary/swept flipper or held-only
  tip. It moves an actual crossing back to a shallow 3px top-face
  overlap and reuses existing restitution and lever-power response.
  No extra kick, random rescue, artwork, table data, score or center
  wall is introduced. It does not protect balls that begin underneath,
  miss the finite paddle, move away, or fall through the untouched
  center gap. Tilt still leaves the **resting** paddle physically
  solid with its power and moving sweep disabled. Nearby broad-phase
  rejection avoids
  division on normal flight ticks. This is a *ball-path* sweep, not
  a faithful rotating 3D flipper/capsule endpoint TOI simulation.
  Synthetic tests cover both sides and rest/held stroke, active
  midpoint, rapid held toe, underside, missed center and tilt.
  Actual ball-pass-through reduction and 7.16 MHz V20 timings
  remain **UNVERIFIED** pending owner hardware playback.
- **Why / evidence:** the owner reported on **real Tandy hardware** that
  flippers felt underpowered and repeated trajectories were too predictable.
  The old forced `vy=-88..-128` and universal `-132` limit made mechanically
  different contacts produce nearly the same ball motion. The new boost is
  source-inspired by `TFlipperEdge::EdgeCollision`, but it is **not an exact
  port** of original front/back contact logic or angular speed. Actual
  improvement, possible tunneling and 7 MHz performance remain **UNVERIFIED**
  until the owner tests a new build.
- **Tests:** synthetic heel/tip leverage, different incoming momentum,
  midpoint sweep contact, moving/stationary flippers and shooter relaunch;
  F-006 added symmetrical active near-center tip catches, idle drain,
  mid-gap drain despite dual buttons, tilt lockout, and underside
  negative checks. Imported-table launch, real flipper traces and
  long-running soak remain mandatory.
- **Remedy:** calibrate angular boost using original source parameters
  (attributes 803-805), preserve ramp/layer states, and expand the new
  ball-path guard toward original angular sweep/endpoint TOI only if
  hardware recordings show further missed contacts.
  Avoid random path perturbations that hide collision problems.

### F-007 Rendering, audio and frame rate

- **2026-11-08 tiny-letter legibility (owner-observed R ambiguity):**
  The 3x5 uppercase UI glyph for `R` had a heavy three-pixel top
  bar and a left-only final stroke, making it resemble `P`. The
  bitmaps now give `R` the conventional open-right upper bowl and
  two diagonally separated lower feet (##./#.#/##./#.#/#.#);
  `P` uses a matching upper bowl and a clearly straight left stem.
  The original `N` bitmap was identical to `M`, so `N` now
  includes a three-row stair-step diagonal. No spacing, font data
  size (36 two-byte glyphs), source numeric FONT.DAT glyphs, 80px
  layout, event timing, VRAM renderer or partial HUD redrawing
  changes. Python tests assert the actual 15-bit glyphs, uniqueness
  of all 26 letters and unchanged four-pixel advance. DOSBox-X
  parity/build and physical CRT legibility remain to be checked;
  visible improvement on the real Tandy is **UNVERIFIED**.

- **2026-11-08 score-only HUD redraw (physical V20 lag report):**
  A gameplay score event previously set the same `ui_dirty` flag as a
  menu transition, causing `draw_panel` to erase and reconstruct the
  entire 80x141 right-hand sidebar, including unchanged metallic frames,
  player numeral and title-adjacent rows. The DOS renderer now limits
  ordinary gameplay updates to four inner rectangles: 50x10 pixels
  for score digits, 11x9 for ball number, 74x26 for game-over command
  text, and 74x16 for event/status text. Repeated occurrences of the
  *same still-visible event message* change score digits without
  re-rendering the status text. Initial display, navigation, menu/help,
  pause and explicit UI transitions retain their full composition,
  including the one-time splash restore on exit. The original FONT.DAT
  numeric glyphs, title logo, palette, Tandy packed VRAM and VGA
  graphics remain unchanged; no gameplay, file I/O, assets, timers
  or allocations are modified. The score patch restores 500 pixels
  (250 packed Tandy VRAM bytes; `restore` operates on 25 bytes per
  row) instead of restoring 11,280 sidebar pixels (5,640 Tandy
  bytes), before the additional saved frame/glyph writes. This is a
  code-path comparison, **not** a measured speedup. Source-layout,
  sidebar dispatch, DOSBox-X smoke/video parity and physical
  7.16 MHz V20 latency all require validation; real timing remains
  **UNVERIFIED**.

- **2026-11-08 source-style UI revision:** the user supplied original
  Space Cadet sidebar screenshots and requested the source layout, not
  only a different font. The prior default DOS HUD listed BALL, SCORE,
  STATUS and controls as separate generic rows. The revised sidebar
  preserves the 76x53 original title image, adds the source-like red
  BALL number box, a player/score strip, a two-line action box and
  a larger lavender status box, all inside fixed 80px metallic frames.
  Original `FONT.DAT` is a 10-glyph 15x22 numeric score resource
  (copyright 1994 Cinematronics), **not** an alphabet. Its digits were
  mirrored from file storage orientation, box-sampled to 5x8 pixels,
  and packed as 100 bytes of 2bpp indexed color in
  `score_font.h`; the existing 3x5 text font remains for instructions.
  Only up to 10 score digits, not zero-padded, are drawn in the 54px
  score cell. The resource is converted offline, never loaded during
  DOS gameplay, and no original source glyph geometry is claimed
  at this lower resolution. Main view updates use the existing
  dirty rectangle and preserve menu/help/high-score/initials flows.
  Test coverage verifies glyph dimensions, palette and full score
  bounds, sidebar sections and shared source for both video modes.
  Physical CRT readability and performance are **UNVERIFIED**.


- **Status / priority:** OPEN, P2, platform-specific adaptation.
- **Original:** multi-resolution artwork, dynamic effects, original audio,
  original update/render pipeline.
- **DOS deviation:** palette reduction, low-res 16-color display, brief
  hardware CRTC shift, synthetic PSG tones and 18.2 Hz display loop with
  four substeps. **2026-11-07 optional visual enhancement:** a PC-side
  `enhance_table.py` produces `TABLE3D.TGA` from the original packed
  `TABLE.TGA` plus the same source `TABLE.DAT` scored bumper positions.
  It adds top-left gray-metal bevel highlights and lower-right neutral-gray
  shading/cast shadows, without changing colorful lamps, sprite assets,
  geometry, collision, projection, table dimensions, or fixed RGBI palette.
  The untouched original background remains accessible via `CADET /classic`;
  `CADET /3d` selects the enhanced look. The source 4x4 ball sprite
  remains in the original packs but is selected only with `/sourceball`
  since the owner's 2026-11-08 roundness-first visual revision below.
  No gameplay data or collision behavior changes.
- **F-007 offline art revision (2026-11-07):** strengthen the existing
  `enhance_table.py` effect for the **default** `TABLE3D.TGA` only:
  brighter upper-left edge rims on painted metallic rails, darker
  lower-right undersides, added short occlusion shadows on gray pixels
  around flipper guide and launcher hardware, and stronger dome highlights
  and lower-right contact shading on the seven imported bumper caps.
  The second pass colors only *existing neutral/gray/white pixels*
  and leaves the original fixed 16-color palette, all colored light
  pixels, `TABLE.TGA`, `TABLE.DAT`, sprite assets, and all 16-bit C
  gameplay/rendering code unchanged. It adds **zero runtime operations**.
  At the time of this offline pass, the moving-ball glint/shadow was
  unchanged; it was subsequently removed as documented below. The source
  screenshot comparison remains unverified on the physical CRT.
- **F-007 optical-height illusion revision (2026-11-07):** added a
  third offline-only shading pass to the default `TABLE3D.TGA` generator.
  It infers a tiered **visual height mask** from the original painted
  metal and the seven already-imported bumper cap centers. A 1-pixel
  bright/dark Mach-band pairing emphasizes raised edges; 1-3 pixel
  internal sidewalls and ordered-dither contact shadows suggest
  thickness; the high bumper tier gets slightly longer down/right
  cast shadows. The mask is not serialized into `TABLE.DAT`, and no
  sprite, C code, collision rules, runtime drawing, screen dimensions,
  palette, or source geometry changed. All recolored pixels were
  already neutral gray/white in the original table. `TABLE.TGA` and
  `/classic` stay untouched. The additional **static effects have
  zero runtime execution or memory cost** compared to the prior build.
  Tests check source color preservation and deterministic output.
  Appearance and readability on the physical CRT are still UNVERIFIED.
- **F-007 VGA compatibility revision (2026-11-07):** standard
  VGA/MCGA detection via BIOS INT 10h AX=1A00h selects VGA 320x200
  mode 13h by default where available; /vga forces it, and /tandy
  forces the original Tandy/PCjr 320x200 mode 09h. /vga rejects
  machines where VGA BIOS detection is absent. Both modes use the
  **same** TABLE.DAT, 32768-byte TABLE.TGA / TABLE3D.TGA,
  sprites, gameplay/input core and fixed 16-color RGBI indices.
  VGA expands the banked image **once at startup** into a linear
  64000-byte far background and writes A000h VRAM. Tandy's original
  four-bank B800h memory and 32KB far background remain intact.
  The 16 Tandy RGBI colors are programmed into VGA DAC entries 0..15;
  VGA's other 240 palette entries are not used or represented as
  faithful new artwork. The existing Tandy CRTC screen shake is
  disabled on VGA. Standard PC-speaker event tones replace Tandy PSG
  writes **only on VGA**, avoiding port 0xC0 on non-Tandy hardware.
  This changes video/audio backend behavior and memory requirements,
  **not** ball movement, scoring, collision, or mission rules.
  The executable remains real-mode 16-bit C89. Higher VGA background
  memory usage is intentional and tested separately in verify_build.py.
  DOSBox-X tests run Tandy auto/forced and VGA auto/forced modes;
  a full 64000-pixel decoded screen parity test has passed with zero
  color or gameplay-frame differences on the emulator.
  **Hardware VGA speed, CRT/monitor output and PC-speaker sound
  are still UNVERIFIED**; DOSBox timing is not a benchmark.
- **F-007 round silhouette revision (2026-11-08):** The owner explicitly
  prioritized a truly round ball over metallic 3D styling. Inspection of
  `TABLE.DAT` showed the original 4x4 source sprite has an irregular
  transparency mask and a bright bottom-right pixel, contributing to
  a lopsided appearance on a physical Tandy CRT. The DOS default now
  replaces the displayed ball sprite *after* successful original
  TABLE.DAT and VGA.DAT validation with a perfectly symmetric 6x5
  pixel disk. The 4:3 aspect of 320x200 graphics means a pixel is
  approximately 1.2x taller than wide, giving a physical circular
  appearance to a 6x5 mask. The fill is uniform RGBI white (15), rim
  is neutral dark gray (8), transparency is 255; there are no highlights,
  fake perspective, cast shadows or per-frame effects. `/sourceball`
  preserves the original 4x4 art for direct comparison, in Tandy 09h
  or VGA 13h, regardless of `/classic` or `/3d`. The replacement uses
  the existing sprite buffer (30 bytes written once), existing source
  coordinates and dirty rectangle calculations, with **no** gameplay
  geometry, fixed-point physics, scoring, launch or collision-radius
  modifications. The original converted table and VGA art files remain
  byte-for-byte unchanged. CI checks 6x5 vertical/horizontal symmetry,
  footprint, palette and default/optional switch, and emulator parity
  confirms identical video output in RGBI Tandy/VGA. Physical V20
  responsiveness and CRT silhouette are still **UNVERIFIED**.
- **F-007 earlier ball silhouette correction (2026-11-07):** physical Tandy CRT
  feedback found the optional three dark floor-shadow pixels and one
  forced white glint made the original tiny ball look less round. Remove
  both runtime overlays from `main.c`, but preserve the source-derived
  4x4 ball sprite from `convert.py` (Windows `PINBALL.DAT`), its existing
  6x6 restore region, collision, coordinates and redraw conditions.
  The original sprite is now identical under `/classic`, `/3d`,
  Tandy 09h and VGA 13h. `SMOKE.TXT` keeps the diagnostic field
  `moving_ball_shadow=0` for compatibility. This removes four
  cosmetic pixel writes on each moving frame, with no extra memory
  and no gameplay change. Prior physical feedback supports removal;
  the corrected silhouette and frame timing on real hardware are
  **UNVERIFIED** pending a new CRT test.
- **F-007 timed status messages (2026-11-07):** original
  `TTextBox.cpp` stores prioritized, queued, timed text messages.
  The Tandy port uses its already available 80px HUD status line to show
  the last reported physics event for 26 rendered frames (~1.4s): bumper,
  launch, drain, kickback or tilt. This intentionally **does not**
  implement source message priority/queue, full mission/status wording,
  or original text rendering. It adds an integer timer and a short
  status redraw only on event/start/expiry, without a full-table redraw
  or source physics modifications. The memory/time cost on actual
  7.16 MHz V20 hardware is UNVERIFIED. Tests exercise the display
  contract, and DOSBox-X Tandy/VGA parity remains required.
- **F-007 owner-supplied title/menu art (2026-11-08):** the 600x600
  Space Cadet splash supplied by the owner is pre-scaled with a small
  ordered-violet-dither adaptation to the fixed 16-color Tandy RGBI
  palette. A compressed, checksum-verified text asset becomes `MENU.DAT`
  (4-byte SCM1 signature, 24,000 packed bytes for a 240x200 left menu
  background, plus 2,014 packed bytes for the 76x53 HUD logo).
  Both standard VGA 13h and Tandy 09h show the same source pixels. Tandy
  uses a 120-byte bank-aware scanline copy, VGA expands high/low nibbles
  once when a menu is entered. Unlike the desktop's window/menu bar,
  the DOS menu is a **paused full-height splash beside the existing
  80-pixel text menu**; compact dialog controls are unchanged. It
  replaces the previous tiny source-derived logo when the pack exists;
  missing/malformed MENU.DAT retains the old title and original menu.
  Physics, sprites, collision geometry and background packs are untouched.
  The splash loads into a 26,014-byte far allocation before gameplay,
  requires one full-screen playfield restoration on menu exit, and does
  not access disk, decompress, redraw menu art or allocate memory
  during game physics. The upload is user-provided; the original source
  title is `SpaceCadetPinball/splash_bitmap.bmp` and the desktop menu
  flow is in `winmain.cpp`. The exact desktop artwork framing, title
  readability on Tandy RGB CRT, memory pressure and V20 menu-transition
  latency are **UNVERIFIED**. Unit checks cover pack integrity, VGA/Tandy
  index parity, normal/missing pack behavior and menu redraw isolation.
  A new actual Open Watcom/DOSBox-X build and physical-hardware
  acceptance remain pending.
- **F-007 active feedback revision (2026-11-08):** owner requested more
  sounds and working table lights. The original `Sound.cpp` plays
  overlapping WAV samples, `TLight.cpp`/`TLightGroup.cpp` control
  independently timed light sprites, and `control.cpp` links lamps to
  many target/mission events. The DOS port still lacks those component
  events and indexed on/off art, so this revision does **not** claim
  authentic sampled audio or full Space Cadet mission lamps. Instead,
  `audio.inc` now synthesizes eight distinct prioritized non-blocking
  note sequences for bumper, plunger, drain, earned kickback, tilt,
  flipper press, nudge and slingshot; Tandy hardware uses PSG tones
  (optional two-voice cues) and VGA uses PIT2 PC-speaker notes at the
  matching approximate pitch. No interrupt takeover, PCM buffer, new
  file or update slower than the existing 18.2Hz frame timer.
  `lamps.inc` illuminates source-derived bumper caps, earned left/right
  kickbacks, and left/right slingshot areas. The added shooter-ready
  indicator was **removed on 2026-11-08** after the user noticed its
  erroneous floating dot beside the ball: it was placed at a guessed
  (launch_x+7,launch_y-15), not a source `TLight` sprite position. The
  READY state is still visible in the right-hand HUD, so the indicator
  added no value.
  Bumper hits and sling contacts flash briefly, and unused kickback
  saves show a steady armed light. Only a
  5x5 pixel patch is restored from existing source artwork on state
  changes, and small RGBI highlights are redrawn if the old ball wipes
  them. The ball and plunger repaint last, avoiding trails. No physics,
  scoring, geometry, SCP2 data format, palette or startup image changes.
  New `/nolights` disables overlays for hardware-performance comparison;
  `/nosound` and F5 mute preserve prior semantics. These are deliberately
  **approximate** lamps because source switch/mission state is not yet
  imported. Both physical Tandy PSG tone quality and 7.16 MHz V20 lamp
  overhead remain **UNVERIFIED**, despite emulator build/parity tests.
- **Why / evidence:** owner requested more visible depth on their son's
  first impression of the CRT playfield. Original game relies on art,
  highlights and occlusion for a 3D impression. Baking effects offline
  costs **zero extra runtime table-rendering instructions**. The earlier
  moving-ball overlay incurred pixel writes, which the silhouette fix
  removes. This is a cosmetic *option*, not evidence of original artwork
  equivalence or hardware performance.
- **Tests:** `test_enhance_table.py` validates nibble/bank fidelity,
  32768-byte size, deterministic enhancement, unchanged colored pixels,
  original source asset immutability and no changes outside the playfield,
  plus positive coverage for stronger rail bevels, bumper dome lighting,
  and nearby contact shadows. `test_run_tests.py` checks that smoke
  output explicitly reports no dynamic ball overlay, including rejection
  of absent or re-enabled overlays.
  DOSBox-X smoke exercises 3D default. /classic remains the lossless
  original Tandy image; needs side-by-side CRT confirmation.
- **Remedy:** measure actual CRT readability, especially colored targets and
  near-flipper ball visibility, plus frame drops using /record. If the
  enhanced style introduces distracting artifacts or a V20 slowdown,
  prefer `/classic` and revise the offline shader, never gameplay
  collision behavior for decorative depth.

#### F-007 source-color VGA upgrade (2026-11-08)

Original `gdrv.cpp::display_palette` and `ApplyPalette` use the indexed
Windows resource palette. VGA previously displayed only the reduced Tandy
RGBI artwork. VGA now loads optional `VGA.DAT` (VGA1): 320x200 source-derived
art, a 256-entry DAC palette and thirteen moving sprites. Slots 0..15 retain
RGBI for the HUD; slots 16..251 reproduce source colors 10..245. Box-filtered
background colors and sprite colors map to this palette; this is not exact
full-resolution Windows rendering. Positions, transparency masks, collision
geometry, physics, scoring and Tandy artwork are unchanged. No AI artwork.

The VGA far background adds 768 palette bytes (64768 total); sprite storage
is reused. No per-frame conversion or extra pixel writes are introduced.
Missing VGA.DAT or `/rgbi` selects the prior RGBI path; a malformed present
pack fails startup. `/classic` and `/3d` still select Tandy/RGBI backgrounds;
source-color VGA uses its original-art background for both switches.

Tests validate palette reservation, all thirteen sprite bounds/transparency,
truncation/mismatch rejection, richer-color emulator capture, and unchanged
VGA simulation counters. RGBI Tandy/VGA parity is still tested separately.
Physical VGA display quality and timing remain UNVERIFIED. Restore closer
source presentation through richer source lighting/animation states in future;
this upgrade does not resolve missing 3D collision/occlusion or mission rules.

### F-008 Universal trapped-ball recovery

- **Status / priority:** OPEN, P0 playability safeguard; 7.16 MHz V20
  timing/impact unverified.
- **Original:** `SpaceCadetPinball/control.cpp` defines
  `control::UnstuckBall(TBall&, int)`. When the ball is outside the
  two flipper control bounds and plunger control bounds, the routine
  initially calls `throw_ball` upward along `{0,-1,0}` with strength
  90 while `StuckCounter <= 20`; beyond that it disables the ball
  and invokes `PlungerRelaunchBall` at zero delay.
  `SpaceCadetPinball/TBall.h` contains `StuckCounter`. The precise
  source-side detection/update/call path **was not established** in the
  inspected code; this documents the recovery philosophy, not a proven
  identical Windows runtime timer.
- **DOS behavior:** `unstuck.inc::watch_stuck` (included by `game.c`) evaluates actual Q6 ball
  position after collisions, once per physics substep (~72.8 Hz).
  Staying within a **3px-wide** x and y travel envelope for **110
  steps (~1.5 seconds)** triggers a short upward plus inward velocity
  correction. It does not rely on velocity alone, so collision jitter
  that reports nonzero speed but no displacement is still detected.
  If the ball remains near the original trapped site for **73 more
  steps (~1 second)**, try a verified-clear local 2-3px upward/inward
  reposition. The watchdog may make one more verified-clear small move
  after another 73 steps, and performs a **free same-ball plunger
  relaunch** after another 73 steps if the ball still cannot escape.
  Each candidate is checked against the packed table's static wall
  bands, radial bumper caps, closed solid sling hulls and safe screen/
  drain margins. If no local candidate is clear, do not teleport into
  another obstacle; proceed toward the eventual free relaunch.
  Six consecutive substeps at least **10px away** from the trapped
  anchor cancel escalation.
- **Intentional exclusions:** disabled for waiting/over balls, live
  shooter-tube travel, held kickback wind-up, both original-flipper
  rest/raised capture envelopes, the plunger vicinity and the normal
  lower drain. Paused/menu gameplay does not advance `game_step`.
  This preserves intentional paddle traps and legitimate losses.
- **Gameplay tradeoff:** DOS has only projected 2D physics and omits
  original ramps, gates and some collision groups. The staged watchdog
  intentionally favors reliable, forgiving play rather than leaving
  a ball imprisoned by an approximation. It **does not** add score,
  consume a ball, reset kickback/rule state, change art or table pack,
  or randomly jump to the center. The low-cost progress tracker uses
  a few integer fields; candidate edge testing occurs only on rare
  recovery attempts. Conventional-memory headroom and real V20 timing
  must be checked; no physical measurement yet.
- **2026-11-08 slingshot oscillation safeguard (owner-reported):** a ball
  that repeatedly crosses from the **left** slingshot to the **right**
  and back travels much farther than the existing 3px no-progress
  envelope. Its old watchdog can reset on every crossing, so there
  is no upper bound on a repeated two-sling cycle. DOS now records only
  actual incoming/reflecting contacts with the two source-derived
  solid sling hulls (F-002), rather than overlapping their collision
  boxes. **2026-11-08 revision:** only THREE alternating contacts
  (not four), within 144 Q6 substeps (~2 seconds) of each other,
  trigger a near-vertical inward escape. The first two hits are
  undisturbed. The earlier four-hit / -64 vertical / +/-24 inward
  correction could still feed the repeated loop; the new -96 Q6
  vertical and +/-18 Q6 horizontal vector carries the ball out of
  the y=105..165 loop zone before it can traverse back to the
  opposite sling. This is less energetic than a normal powered
  flipper shot, not a return to the old excessive -144/-48 vector. Physical
  effectiveness remains **UNVERIFIED**. The immediate return is ordinary physics: the ball is **not**
  teleported or moved through walls, has no score or ball count changes,
  and remains vulnerable to normal drains. The sequence expires after
  a quiet interval, leaving the y=105..165 pixel sling zone, entering
  a shooter/kickback/tilt state, or on a ball feed; contact on the same
  sling does not advance it. Local true-jitter rescue remains separate.
  This is a **DOS playability adaptation** caused by 2D projected
  geometry and reduced source targets/layers; original
  `control::UnstuckBall` lacks this verified behavior. No new table
  assets or expensive collision tests are introduced, only three bytes
  and a small integer counter in `Game`, plus a few integer checks.
  Real 7.16 MHz V20 effectiveness and natural replay frequency are
  **UNVERIFIED**.
- **Diagnostics:** `/record` writes cumulative
  `sling_loop_breaks` plus existing `unstick_nudges`,
  `unstick_moves`, `unstick_relaunches` into
  `INPUT.TXT`; `/smoke` writes the same counters to `SMOKE.TXT`.
- **Tests:** synthetic alternating genuine approach-side reflections
  (third hit on both sides, correct near-vertical vector), two-hit
  ordinary volleys, same-side recontacts, expired progress, upper-table
  departure, tilt exclusion; plus prior
  synthetic stuck/jitter loops with zero or nonzero velocity;
  110-step detection, 73-step safe moves, eventual free relaunch with
  unchanged balls/score; healthy escape cancels stages; exclusions for
  shooter, waiting plunger, kickback, flipper captures and drain.
  Existing imported table trajectories, 20k soak and both Tandy/VGA
  playback/visual parity must continue passing.
- **Acceptance/remedy:** verify several real stalls and deliberate
  flipper catches on the physical 7.16 MHz V20; check that rescues are
  rare, graceful and do not hide a repeated geometry bug. If needed,
  tune `UNSTICK_*` thresholds with recorded evidence or restore the
  original component-specific approach when full layer/gate data
  becomes available. **Hardware success is UNVERIFIED.**

### F-009 High scores and initials

- **Status / priority:** OPEN, P2, partial original UI and persistence.
- **Original source behavior:** `SpaceCadetPinball/high_score.cpp`
  maintains five scores, moves lower scores down on insertion, accepts
  player names up to 31 characters via a desktop modal dialog, and
  saves entries with a verification checksum in application settings.
  `options.cpp` supplies keyboard shortcuts.
- **DOS behavior / difference:** `scores.c` implements five ordered
  `unsigned long` scores; three uppercase ASCII initials are captured
  by `input.inc` and shown in `hud.inc`. At game-over, a positive
  qualifying score opens entry; Enter inserts/saves, Esc skips.
  The source-relative ordering is retained for equal scores, but
  DOS stores a fixed-width, versioned text `SCORES.DAT` in the
  current directory, not settings. A missing/corrupt file resets
  the table and file save failures are shown. Menu HIGH SCORES
  displays the current top five; no original long-name dialog,
  multiple players or settings persistence.
- **Why:** 80px side HUD and 16-bit DOS do not include the desktop
  ImGui/settings UI. Three letters fit without a new font,
  large image assets or a window manager; the save is outside
  active gameplay. This is a UI/storage adaptation, not a measured
  CPU performance requirement. Actual V20/disk compatibility is
  UNVERIFIED.
- **Evidence/tests:** `TEST.EXE` covers rank ordering, tie/overflow
  rejection, name validation, valid save/load and corrupted-file
  rejection. Host regression tests check the input queue and
  menu/shortcut source. The main DOSBox-X smoke remains intentionally
  score-I/O free; manual physical verification of three-initial entry,
  saved-file reload, disk-full/readonly save failure, and exit/return
  is still required.
- **Restoration:** preserve the current top-five logic, then compare
  Windows score eligibility, multi-player handling and long-name
  entry against source before considering richer UI. Do not add disk
  I/O to the active physics loop.

## Required fidelity review for EVERY gameplay-affecting change

Record these answers in the commit message or the relevant exception entry:

- [ ] **What does the original do?** Name the original source file,
      entry point, constant/state machine and data group.
- [ ] **What does DOS do?** Identify the code/data and whether the outcome
      matches. Note all new/random/timing or state changes.
- [ ] **Why is a difference required?** Prefer source-preserving fixed-point
      or rendering optimizations. For a behavior change, cite measured
      performance/memory data or label the reason *unverified*.
- [ ] **Did I update the exception register?** Add/edit F-### with source,
      effect, reason, evidence, tests, priority and a restoration path.
- [ ] **Do tests cover both positive and negative original behavior?**
      Update `test_game.c`, converter tests and source-table trajectories
      as applicable; run DOSBox-X / smoke / CI.
- [ ] **What remains to test on the physical V20?** Record limits honestly
      in `VALIDATION.md`. Do not claim parity or performance from CI alone.
- [ ] **Does the deployment use matching assets?** For SCP2 changes, update
      `CADET.EXE`, `TABLE.DAT`, `TABLE.TGA` together.
- [ ] **Did I follow branch restrictions?** Only the existing
      `dos/tandy-native-prototype` branch; no PR/new branch without request.

### New exception template

```markdown
### F-### Short name
- **Status / priority:** OPEN | RESOLVED; P0 | P1 | P2
- **Original source behavior:** [file/function and asset/group]
- **DOS behavior / exact difference:** ...
- **Reason for divergence:** measured hardware limit + data, or UNVERIFIED
- **Impact on gameplay / presentation:** ...
- **Evidence and tests:** ...
- **Restoration plan / acceptance criteria:** ...
- **Introduced / changed in commit:** ...
```

Closing an exception requires tests demonstrating restored original behavior,
a matching source reference, and validation evidence. Keep resolved entries
for history, rather than deleting them.

## Priority order

**First address demonstrated playability defects** on the physical Tandy:
flippers that cannot drive the ball, overly repetitive trajectories, trapped
balls, missed contact, shooter relaunch failures, and slow frames. **Then
address fidelity debt by severity:** P0 source kickback/target/gate and
one-way/layer rules; P1 plunger timing, bumper progression, nudge thresholds,
and flipper fidelity; P2 remaining physics/presentation/audio. A P0 label means
high fidelity importance but cannot force an unplayable build. Document
playability-versus-fidelity exceptions and compare them to original code.

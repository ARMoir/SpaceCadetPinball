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
   Never present an unmeasured gameplay workaround as an optimization.

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

Original code locations (read alongside any proposed DOS implementation):

| Concern | Original reference | Current DOS reference |
| --- | --- | --- |
| Ball movement / collisions | [`TBall.cpp`](../../SpaceCadetPinball/TBall.cpp), [`maths.cpp`](../../SpaceCadetPinball/maths.cpp), [`TEdgeSegment.cpp`](../../SpaceCadetPinball/TEdgeSegment.cpp) | [`game.c`](game.c), [`convert.py`](convert.py) |
| Flipper sweep | [`TFlipper.cpp`](../../SpaceCadetPinball/TFlipper.cpp), [`TFlipperEdge.cpp`](../../SpaceCadetPinball/TFlipperEdge.cpp) | [`game.c`](game.c) |
| Plunger / feeder | [`TPlunger.cpp`](../../SpaceCadetPinball/TPlunger.cpp) | [`game.c`](game.c), [`main.c`](main.c) |
| Kickbacks, targets and gates | [`TKickback.cpp`](../../SpaceCadetPinball/TKickback.cpp), [`control.cpp`](../../SpaceCadetPinball/control.cpp), [`TGate.cpp`](../../SpaceCadetPinball/TGate.cpp) | [`game.c`](game.c), [`convert.py`](convert.py) |
| Bumpers / scoring | [`TBumper.cpp`](../../SpaceCadetPinball/TBumper.cpp), [`control.cpp`](../../SpaceCadetPinball/control.cpp) | [`game.c`](game.c), [`convert.py`](convert.py) |
| Nudge / tilt | [`nudge.cpp`](../../SpaceCadetPinball/nudge.cpp), [`pb.cpp`](../../SpaceCadetPinball/pb.cpp) | [`game.c`](game.c), [`main.c`](main.c) |
| Resource import / projection | [`loader.cpp`](../../SpaceCadetPinball/loader.cpp), [`proj.cpp`](../../SpaceCadetPinball/proj.cpp) | [`convert.py`](convert.py), [`data/IMPORT.json`](data/IMPORT.json) |

The original game also uses resource-driven component rules. Source constants
should be checked in context, not copied without their event/state transitions.

## Audit of the current prototype

| Priority | Area | Original behavior versus current DOS | Fidelity status | Register |
| --- | --- | --- | --- | --- |
| **P0** | Kickback rules | Original kickback component uses ~0.7 s timer plus ~0.1 s follow-up, with left/right Hazard Target triplets and gate state transitions. DOS currently arms each side automatically for one use per ball, catches near an imported strike center, holds ~0.12 s, then fires a fixed upward/inward impulse. Imported strike positions are an improvement but the rules are **not** preserved. | **Unapproved behavioral approximation**, not a measured performance necessity | [F-001](#f-001-kickback-activation-and-timing) |
| **P0** | Collision layers / one-way gates | Original has world-space collision groups, ramps and one-way/gate components. DOS projects collisions into one 2D plane and hard-codes an upper-left downhill-only screen-coordinate exception. The import inventory records omitted one-way gates, targets, sinks and ramps. | **Incomplete** with a temporary geometry workaround | [F-002](#f-002-collision-layer-and-one-way-projection) |
| **P1** | Plunger timing and variance | Original `TPlunger` has eight visual states, 25 ms increments, boost accumulation (approximately 2.5 s to full charge for the Windows configuration), and up to 10% randomized boost. DOS correctly imports eight original frames but charges in ~1.3 s with ~1% jitter to compensate for simplified shooter physics. | **Partially faithful; unapproved behavioral approximation** | [F-003](#f-003-plunger-charge-and-launch-variation) |
| **P1** | Bumper collisions / scoring | Original bumper response is radial and scoring can change with state. DOS precomputes one radial hit surface per imported eight-edge cap to prevent inward polygon trapping, but still flattens three elevated bumpers and uses static 500/1500 scores with shared hit delay. | **Improved contact fidelity; scoring/layers incomplete** | [F-004](#f-004-bumper-projection-and-scoring) |
| **P1** | Nudge and tilt | Original `nudge.cpp` applies additive velocity and an inverse after 0.4 s; `pb.cpp` accumulates nudge pressure over time. DOS preserves that shape but uses its own impulses, nudge-heat thresholds and decay schedule. | **Partially faithful; tuning differs** | [F-005](#f-005-nudge-and-tilt-parameters) |
| **P1** | Flipper physics | Original angular flippers collide throughout rotation. DOS preserves endpoints but uses rest/midpoint/raised fixed-point edges and only two art states. | **Performance-oriented approximation; fidelity still unverified** | [F-006](#f-006-flipper-and-collision-response) |
| **P2** | Material physics / gravity | Original ball direction, spin, elasticity, smoothness, group masks and geometry are richer. DOS uses Q6, fixed restitution, 2D gravity, speed clamps and finite substeps. | **Known incomplete port** | [F-006](#f-006-flipper-and-collision-response) |
| **P2** | Audio, layout and effects | Original sound/visual states are richer. DOS has PSG substitutes and 16-color rendering; an optional offline-generated 3D-shaded TABLE3D.TGA retains the untouched classic art and identical collision data. The moving ball adds a tiny shadow/glint in an existing dirty rectangle. | **Presentation-only adaptation**, gameplay parity unchanged; CRT benefit unverified | [F-007](#f-007-rendering-audio-and-frame-rate) |

**Verified progress, not full parity:** original-source table artwork, eight
plunger frames and kickback strike centers are imported; seven bumper outlines
score; synthetic tests include weak-shot return, kickbacks and 20,000-step
soak. The latest [GitHub Actions run](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42383212680)
passed. This verifies those tested trajectories, **not** original game logic
or real V20 timing. See [`VALIDATION.md`](VALIDATION.md).

## Exception register (append/update whenever rules are broken)

Each entry is an **open fidelity debt** until the original behavior is
restored or a measured platform necessity is documented and approved. Do not
silently treat legacy exceptions as precedent for new ones.

### F-001 Kickback activation and timing

- **Status / priority:** OPEN, P0; no measured performance justification.
- **Original:** `TKickback.cpp` timer values 0.7 s and 0.1 s; `control.cpp`
  left/right Hazard Target triplets drive gate enable/disable and reset paths.
- **DOS deviation:** kickbacks start armed, are automatically available once
  per side per ball, use a ~0.12 s hold and fixed return velocities. Original
  target progression, conditional gate state and full timing are absent.
- **Why / evidence:** temporary gameplay functionality with reduced component
  model; **not** demonstrated to be needed for 7 MHz performance.
- **Tests today:** imported strike centers, three offsets per side, collision
  containment and one-use gating. These test the substitute, not original rules.
- **Remedy:** port target triplet state, activation signals and original
  timer phases with integer substep timers. Test disabled/enabled, consume,
  rearm and tilt/game transitions against original source behavior.

### F-002 Collision layer and one-way projection

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
- **Why / evidence:** the owner also observed a ball disappear into an
  upper-left highlighted area after a bounce. Whether this was an off-screen
  exit, collision mask/occlusion, or a gameplay drain is **unverified**.
  The old code categorically treated off-screen top/side exits as drains,
  which is unlike the actual below-flipper drain. The temporary containment
  prioritizes preserving a playable ball while source collision-layer import
  is incomplete. Real-mode performance/format limitations remain unmeasured.
- **Tests:** synthetic one-way throat, all seven bumper contacts, a radial
  penetration recovery at the upper-left cap, and top/side containment
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

### F-003 Plunger charge and launch variation

- **Status / priority:** OPEN, P1; no direct performance justification.
- **Original:** `TPlunger.cpp` eight frames, 25 ms charge increments,
  Windows maximum boost 100, and random added boost of up to 10%.
- **DOS deviation:** eight original frames are imported (good), but charge
  saturates in about 1.3 s; launch jitter is only ~1%, tuned for current
  projected shooter path.
- **Why / evidence:** full-power source-like variation currently destabilizes
  simplified 2D shooter trajectories, not a CPU-budget constraint.
- **Tests today:** 12 partial-charge returns, full charge reaches a bumper,
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
- **Tests:** seven independent scoring contacts; targeted penetration and
  inbound hits at the upper-left imported cap, with radial separation and
  outbound reflection. Full gameplay/smoke and speed checks remain mandatory.
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
  imported-table launch and long-running soak remain mandatory.
- **Remedy:** calibrate angular boost using original source parameters
  (attributes 803-805), preserve ramp/layer states, and add swept segment
  collision only where measured hardware tests show missed contacts.
  Avoid random path perturbations that hide collision problems.

### F-007 Rendering, audio and frame rate

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
  `CADET /3d` selects the enhanced look. The original 4x4 ball sprite gains
  one highlight and three shadow pixels when rendered in 3D mode, entirely
  within the *existing* 6x6 dirty region; sprite and gameplay data unchanged.
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
  The previously existing tiny moving-ball glint/shadow is unchanged.
  The source screenshot comparison remains unverified on the physical CRT.
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
- **Why / evidence:** owner requested more visible depth on their son's
  first impression of the CRT playfield. Original game relies on art,
  highlights and occlusion for a 3D impression. Baking effects offline
  costs **zero extra runtime table-rendering instructions**. The ball
  shadow/glint adds a few pixel writes on moving frames, so calling that
  part zero-cost would be inaccurate. This is a cosmetic *option*, not
  evidence of original artwork equivalence or hardware performance.
- **Tests:** `test_enhance_table.py` validates nibble/bank fidelity,
  32768-byte size, deterministic enhancement, unchanged colored pixels,
  original source asset immutability and no changes outside the playfield,
  plus positive coverage for stronger rail bevels, bumper dome lighting,
  and nearby contact shadows.
  DOSBox-X smoke exercises 3D default. /classic remains the lossless
  original Tandy image; needs side-by-side CRT confirmation.
- **Remedy:** measure actual CRT readability, especially colored targets and
  near-flipper ball visibility, plus frame drops using /record. If the
  enhanced style introduces distracting artifacts or a V20 slowdown,
  prefer `/classic` and revise the offline shader, never gameplay
  collision behavior for decorative depth.

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

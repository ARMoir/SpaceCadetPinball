# Original-game fidelity contract and audit (DOS / Tandy)

**Priority: P0 project requirement.** Last reviewed: **2026-11-07**, DOS commit
[`f78f87c`](https://github.com/ARMoir/SpaceCadetPinball/commit/f78f87c649edff4b4f0d02bdb65074ea4cfa3e9d).
This is the standing design and review standard for `Platform/dos/`. It is
not a claim that the current prototype is already faithful.

## Non-negotiable rules

1. **Original gameplay is the specification.** Prefer the original Space Cadet
   logic and resource data in `SpaceCadetPinball/` and Windows `PINBALL.DAT`.
   Preserve component state, scoring, triggers, collision direction, timers,
   activation/deactivation, randomness, and game progression whenever practical.
   Do not invent replacement rules merely because they are simpler to code.
2. **Optimize implementation, not behavior.** For the ~7.16 MHz NEC V20,
   implement equivalent rules with C89, fixed-point/int arithmetic, precomputed
   tables, reduced drawing, bounded memory, and efficient broad-phase collision
   checks. Keep timings in real/game-time units, even if counters use substeps.
3. **No silent fidelity regressions.** If behavior has to change for a proven
   platform constraint, add/update an entry in the **Exception register below in
   the same change**. Capture original source references, exact divergence,
   technical reason, evidence, affected tests and a route back to fidelity.
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
| **P1** | Bumper collisions / scoring | Original material/collision response and scoring can change with state. DOS fits eight-sided hit edges to all seven visible caps, flattens three elevated bumpers, and uses static 500/1500 values with shared hit delay. | **Incomplete** and visually guided approximation | [F-004](#f-004-bumper-projection-and-scoring) |
| **P1** | Nudge and tilt | Original `nudge.cpp` applies additive velocity and an inverse after 0.4 s; `pb.cpp` accumulates nudge pressure over time. DOS preserves that shape but uses its own impulses, nudge-heat thresholds and decay schedule. | **Partially faithful; tuning differs** | [F-005](#f-005-nudge-and-tilt-parameters) |
| **P1** | Flipper physics | Original angular flippers collide throughout rotation. DOS preserves endpoints but uses rest/midpoint/raised fixed-point edges and only two art states. | **Performance-oriented approximation; fidelity still unverified** | [F-006](#f-006-flipper-and-collision-response) |
| **P2** | Material physics / gravity | Original ball direction, spin, elasticity, smoothness, group masks and geometry are richer. DOS uses Q6, fixed restitution, 2D gravity, speed clamps and finite substeps. | **Known incomplete port** | [F-006](#f-006-flipper-and-collision-response) |
| **P2** | Audio, layout and effects | Original sound and visual states are richer; DOS has PSG substitute sounds, a 16-color sidebar and cheap CRTC shake. | **Platform adaptation**, visual/audio parity not claimed | [F-007](#f-007-rendering-audio-and-frame-rate) |

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
  flattened upper bumper layers.
- **Why / evidence:** real-mode performance/format limitations have been
  assumed; no like-for-like V20 benchmark has yet shown source-defined one-way
  segments or layer state is too costly.
- **Tests today:** synthetic one-way throat and several imported trajectories.
- **Remedy:** emit one-way direction and layer/group metadata from
  `convert.py` into the pack; keep inexpensive per-edge integer filters.
  Test passage in permitted direction, blocking in reverse, and ramp return.

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
  original frame positions.
- **Remedy:** restore the original charge curve and boost variance once source
  launcher response/curve and gate geometry work across tested shot strengths.

### F-004 Bumper projection and scoring

- **Status / priority:** OPEN, P1.
- **Original:** `TBumper.cpp` visual/threshold state and source
  `control.cpp` per-component score progression.
- **DOS deviation:** fitted octagonal collision outlines with fixed initial
  500/1500 point awards; higher-layer caps are flattened into one collision
  plane; common hit-delay replaces independent component states.
- **Why / evidence:** playable coverage within 16-bit constraints; scoring
  simplification has not been shown necessary for performance.
- **Tests today:** all seven caps can score one initial hit.
- **Remedy:** preserve per-bumper original score state/cooldown, material
  behavior and collision-layer metadata; test successive hits and rule changes.

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
  ramp height/spin.
- **Why / evidence:** integer arithmetic and bounded collision work are
  credible V20 optimizations; **specific gameplay changes need comparison
  tests and, where practical, hardware cost measurements**.
- **Tests today:** both flippers, raised/midpoint contact, imported trajectories.
- **Remedy:** fixed-point port of more original edge impulse and angular sweep;
  prioritize missed contacts and realistic rebounds without full table scans.

### F-007 Rendering, audio and frame rate

- **Status / priority:** OPEN, P2, platform-specific adaptation.
- **Original:** multi-resolution artwork, dynamic effects, original audio,
  original update/render pipeline.
- **DOS deviation:** palette reduction, low-res 16-color display, brief
  hardware CRTC shift, synthetic PSG tones and 18.2 Hz display loop with
  four substeps. These also affect presentation/timing.
- **Why / evidence:** available Tandy display and sound hardware plus V20
  memory/CPU limits. DOSBox-X tests pass, real-hardware measurement is needed
  for each claimed speed benefit.
- **Tests today:** Tandy mode video smoke and cleanup; no definitive hardware
  assessment of motion/sound/shake.
- **Remedy:** preserve the original look/rules within the hardware format;
  measure image quality, latency, sound and frame timing on the real Tandy.

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

**P0 first:** source kickback/target/gate state and one-way/layer correctness.
**P1 second:** source plunger timing/variance, bumper rules, source nudge
thresholds and better flipper contact. **P2 third:** material fidelity,
presentation, audio and optional enhancements. A P2 cosmetic feature must not
supersede an unresolved P0 gameplay mismatch without explicit direction.

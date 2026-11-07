# DOS / Tandy 1000 EX feature inventory and project handoff

**Status: playable experimental prototype, not a full Space Cadet port.**
Reviewed November 7, 2026, on `dos/tandy-native-prototype`.
This is the **authoritative checklist for features not implemented** in the
native real-mode DOS/V20 build. It describes code and converted resource
coverage, **not a claim that the final build has passed physical-hardware
acceptance testing**.

**Project priorities:** (1) playable, responsive, fun on the actual Tandy,
(2) authentic original-game behavior and artwork, a very close second.
See [`FIDELITY.md`](FIDELITY.md) for original source references, engineering
rules, exceptions F-001 through F-007, and why deviations currently exist.

## What works in the current prototype

These functions exist in the DOS build and have automated emulator or core
test coverage; final revisions still need hands-on verification on the real
Tandy 1000 EX/V20.

| Function | Present implementation | Evidence / limitations |
| --- | --- | --- |
| Native executable | Real-mode 16-bit Open Watcom C89 `CADET.EXE`; no Windows, FPU, XMS/EMS, SDL or extender | MZ/header and DGROUP checks in `verify_build.py` |
| Tandy display | BIOS mode 09h, 320×200, 16-color banked VRAM, original projected playfield | 360-frame video smoke and screenshot |
| Appearance | Classic `TABLE.TGA`; optional default `TABLE3D.TGA` with **offline-only static** fake-depth shading; `/classic` comparison | Both styles share the same `TABLE.DAT`; original colored artwork remains intact |
| Ball and paddles | Imported ball and two rest/raised flipper sprites, fixed-point movement and collisions | Playable but simplified physics, not continuous original sweep |
| Plunger | Eight original animation frames, hold/release, full/weak launch; weak shooter returns can be launched again | DOS charge curve differs from source (F-003) |
| Game lifecycle | Three balls, score, drain, game over, restart, basic ball reset | No complete rules/bonus/multiplayer |
| Bumpers | Seven source-derived visible caps have radial contact, static initial 500/1500 scoring | No original per-bumper rule progression or vertical layers (F-004) |
| Kickback saves | Left/right outlane catch, wind-up and return; one use per side per ball | **Not** original Hazard Target/gate state logic (F-001) |
| Nudge / tilt | X / A / D impulses and tilt-warning/lockout | Custom thresholds, not exact original accumulation (F-005) |
| Inputs and UI | Z and / flippers, Space plunger, keyboard menu, pause, new game, help/controls, sound toggle, exit | Menu is a compact substitute, not complete original dialogs |
| Sound | Short Tandy PSG tones, `/nosound`, S toggle | Original sound samples and music not ported |
| Diagnostics | `/record`, `INPUT.TXT`, `FLIGHT.TXT` (schema 3), slow-frame and right-rail counters; PC-side `analyze_logs.py` | Buffer until exit, not long-term historical telemetry |
| Build/repro | Converter, test suites, Open Watcom build, DOSBox-X core/video smoke, `CADET.zip` artifact | Emulator pass does not prove real 7.16 MHz timing |

## Partially implemented, approximated, or not faithful to source

| Area | What is present | What is still **not** implemented | Fidelity audit |
| --- | --- | --- | --- |
| World physics | Q6 fixed-point 2D ball, wall normals, gravity, speed clamps, collision bands | Original 3D/world-space position and height, spin, full material friction/elasticity, stochastic drag, continuous sweep, exact collision responses | F-002, F-006 |
| Ramps and layers | Initial ramp art and selected projected ground rails | Physical ramp elevation, traversable upper/lower layers, occlusion masks, level-dependent rail collisions and accurate ramp exits | F-002 |
| One-way rails and gates | Special upper-left passage and source-derived right-rail containment guards | General original `TOneway`, gate orientation, open/closed state and per-group collision filtering | F-002 |
| Flippers | Source rest/raised geometry; midpoint and seven-substep assisted motion, leverage-based impulse | Original continuous angular sweep/velocity and exact front/back collision, trap/hold and original power curve | F-006 |
| Shooter | Original plunger frames and re-catch of weak return | Original 25 ms boost progression, ~2.5 s charge curve, full ~10% boost variance and exact shooter chute transitions | F-003 |
| Bumpers | Seven radial caps, basic initial scoring | Variable score state, independent component cooldowns, full original material/layer interaction | F-004 |
| Kickbacks | Two timed saves, strike centers from source | Hazard Target triplet activation, gate enable/disable, source timer/state/rearm, actual mission coupling | F-001 |
| Nudge/tilt | Three directions, reversal impulse and warning/lockout | Exact source threshold, accumulation/decay and physically accurate displacement | F-005 |
| Visual effects | Static initial-state lights, color-reduced artwork, small ball shadow/glint and optional CRTC shake | Original state-driven lamp animations, ramps/targets changing visually, full image-depth/occlusion rendering | F-007 |
| Audio | Synthetic PSG event beeps | Source samples, original soundtrack/music playback and full event sound mapping | F-007 |

**Important:** The small one-sided right-rail and top/side screen safeguards
are **temporary playability repairs**, not restored source collision groups.
If the ball repeatedly tunnels, gets stuck or vanishes on real hardware,
those issues remain open, even if synthetic tests pass.

## Not implemented as gameplay systems

These original game features are **absent**, not optimized away and not
secretly available behind a menu:

- **Complete missions and rank/progression:** mission objectives and
  advancement, target-set completion, flashing status, rewards and sequencing.
- **Full scoring system:** skill-shot awards, multiplier/bonus computation,
  jackpots and their qualification, mission-dependent points and end-of-ball
  bonus tally. The prototype simply accumulates basic scored bumper hits.
- **Multiball:** locking, spawning and managing multiple concurrent balls,
  ball-specific collision/state and multiball awards.
- **Physical target and ramp event logic:** proper rollover lanes and switches,
  stand-up/drop/spot target events, sinks, holes, kickouts, tripwires,
  flags, live gate logic, ramp entry/exit and their state-driven animations.
- **Multiplayer and high-score system:** multiple alternating players,
  high-score table, initials entry, persistent scores/settings and the full
  Windows Game/Options dialogs.
- **Original audiovisual presentation:** complete sound/music resources,
  synchronized lamps and other dynamic object sprites, full-resolution art
  and original renderer/layered camera.
- **Other table support:** this target supports Space Cadet from Windows
  `PINBALL.DAT` only; it does not support the other Full Tilt tables
  or Full Tilt `CADET.DAT`.

### Import inventory: what the converted table leaves out

The included [`data/IMPORT.json`](data/IMPORT.json) records the exact
resource provenance and importer omissions:

- **521** source-projected collision segments are included in `TABLE.DAT`;
  **62** source collision objects are included in the import inventory.
- **80** source collision objects are excluded from the *generic collision
  import*, including named `a_targ*` targets, `a_roll*` rollovers,
  `s_onewy*` one-way components, `v_gate*` gates, `v_sink*` sinks,
  `s_trip*` triggers, `ramp_hole`, flags, and kickout objects.

This inventory is **not** a count of completely missing rendered objects.
Some excluded types, notably the plunger, drain, and kickbacks, have **separate
simplified DOS implementations** and imported art/anchors. Others may still
be visible in the static playfield image but have **no active gameplay
functionality**. The importer is in [`convert.py`](convert.py).

## Actual Tandy hardware: observed issues and unverified final fixes

Earlier physical testing demonstrated the DOS prototype can run on the
owner's machine. It also exposed real issues that led to subsequent fixes.
**The latest revision has not been re-certified as fully playable on that
machine**, and those prior symptoms are not claimed solved:

| Hardware observation | Current mitigation | Final physical verification |
| --- | --- | --- |
| Right flipper underpowered or ball passed through | Stronger held/front-face response and longer seven-substep swept contact | **Pending**: rapid taps, held keys, tip/heel impacts, several drains |
| Ball wedged near the upper-left bumper | Radial contacts, separate neighboring static-wall correction | **Pending**: repeated full-strength launches and chute returns |
| Ball seemed to disappear into the upper-left playfield | Top/side safety rail, event/flight tracing | **Pending**: confirm correct ramps and no hidden trapping |
| High-speed ball flew to upper-right HUD boundary | One-sided source-projected right outer rail guard, `right_rail_saves` logs | **Pending**: confirm ball remains inside playable table without unnatural bounce |
| Tandy V20 responsiveness and 3D art legibility | Fixed-point, dirty rectangles, offline shading, `/classic` | **Pending**: sustained play, input response, `slow_frames` and side-by-side CRT inspection |

For investigation, run `CADET /record`, quit with Esc immediately after a
failure, then collect `INPUT.TXT` and `FLIGHT.TXT`. `analyze_logs.py` on a
PC can summarize them. `FLIGHT.TXT` stores only the last 256 rendered
frames (~14 seconds), so it is not a permanent replay.

## Repeatable build / final handoff

Use the latest **successful** GitHub Actions run on
[`dos/tandy-native-prototype`](https://github.com/ARMoir/SpaceCadetPinball/tree/dos/tandy-native-prototype).
Its `CADET` artifact extracts as `CADET.zip`. Failed runs publish
`CADET-DIAGNOSTICS` instead; do not treat those as playable downloads.

Copy these **matching files from the same ZIP** to a directory on the Tandy:
`CADET.EXE`, `TABLE.DAT`, `TABLE.TGA`, and `TABLE3D.TGA`. Do not mix
older SCP1/SCP2 data with a newer EXE.

```dos
CADET
CADET /classic
CADET /record
```

The two image styles use identical gameplay rules; static faux-3D shading
costs no additional runtime processing compared to the classic background.
Tests use `test_convert.py`, `test_analyze_logs.py`,
`test_enhance_table.py`, `test_run_tests.py`, the 16-bit `TEST.EXE`
`/original` scenarios, `verify_build.py`, and 360-frame DOSBox-X smoke
runs. See [`VALIDATION.md`](VALIDATION.md) for evidence and caveats.

## Project disposition

**Feature work is paused by owner preference after this stabilization pass.**
The port is a useful demonstration of Space Cadet running in native 16-bit
Tandy graphics, **not a feature-complete original game**. Remaining gaps
are recorded here and in [`FIDELITY.md`](FIDELITY.md), not silently waived.
Do not imply that absent systems were eliminated for measured V20 speed
without actual evidence. Further development should focus on an observed
hardware playability problem, not more decorative shading or feature creep.

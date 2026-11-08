# DOS / Tandy 1000 EX feature inventory and project handoff

**Status: playable experimental prototype, not a full Space Cadet port.**
Reviewed November 7, 2026, on `dos/tandy-native-prototype`.
This is the **authoritative checklist for features not implemented** in the
native real-mode DOS/V20 build. It describes code and converted resource
coverage, **not a claim that the final build has passed physical-hardware
acceptance testing**.

**Project priorities:** (1) playable, responsive, fun on the actual Tandy,
(2) authentic original-game behavior and artwork, a very close second.
Read [`SOURCE_MAP.md`](SOURCE_MAP.md) for the refactored implementation
and original C++ counterparts. [`FIDELITY.md`](FIDELITY.md) remains the
authoritative audit of F-001 through F-008 differences and their rationale.

## What works in the current prototype

These functions exist in the DOS build and have automated emulator or core
test coverage; final revisions still need hands-on verification on the real
Tandy 1000 EX/V20.

| Function | Present implementation | Evidence / limitations |
| --- | --- | --- |
| Native executable | Real-mode 16-bit Open Watcom C89 `CADET.EXE`; no Windows, FPU, XMS/EMS, SDL or extender | MZ/header and DGROUP checks in `verify_build.py` |
| Tandy/VGA display | Tandy/PCjr BIOS mode 09h in banked B800h 16-color VRAM, or VGA BIOS mode 13h with linear A000h 8bpp VRAM; both 320x200 and same 16-color source art | Explicit /tandy and /vga overrides and BIOS VGA auto-selection; CI tests both video backends and full 64000-pixel final-frame parity; physical VGA performance unverified |
| Appearance | Classic `TABLE.TGA`; optional default `TABLE3D.TGA` with **offline-only static** fake-depth shading; `/classic` comparison | Both styles share the same `TABLE.DAT`; original colored artwork remains intact |
| Ball and paddles | Imported ball and two rest/raised flipper sprites, fixed-point movement and collisions | Playable but simplified physics, not continuous original sweep |
| Plunger | Eight original animation frames, hold/release, full/weak launch; ball seats immediately above visible original rod tip, and weak shooter returns can be launched again | DOS charge curve and 2D resting position are adapted from source; physical shooter testing pending (F-003) |
| Game lifecycle | Three balls, score, drain, game over, restart, basic ball reset and persistent local top-five high scores with initials | No complete scoring rules/bonus/multiplayer (F-009 is the simplified high-score file/entry) |
| Bumpers | Seven source-derived visible caps have radial contact, static initial 500/1500 scoring | No original per-bumper rule progression or vertical layers (F-004) |
| Kickback saves | Left/right outlane catch, wind-up and protected high-return; original narrow kicker door opens while armed and physically/visually closes after fire; one use per side per ball | Original Hazard Target arming and 0.7+0.1s timer not yet ported; gate state and source-positioned metal overlay adapted (F-001); physical V20 validation pending |
| Nudge / tilt | X / A / D impulses and tilt-warning/lockout | Custom thresholds, not exact original accumulation (F-005) |
| Universal anti-stuck | Q6 position watchdog with upward nudge, two verified-clear moves and same-ball relaunch; additional bounded upward/inward impulse after 4 alternating left/right sling reflections, without teleporting | Original `UnstuckBall` timer and DOS-only cyclic-sling guard do not reproduce original rules; physical V20 outcomes pending (F-008) |
| Inputs and UI | Source-style four-box 80px sidebar with original score digits, title/ball strip, player/score row, action and status messages; Z and / flippers, optional left/right mouse; **middle mouse plunger** on three-button drivers; Space launch; F5 sound, F8 controls, temporary event messages, initials/top-five panel | DOS INT 33h mouse driver required; keyboard and file/menu flow need physical V20 verification; reduced original TTextBox behavior (F-007/F-009) |
| Sound | Eight distinct short prioritized non-blocking cues on Tandy PSG (some two-tone) or VGA PC speaker: bumper, launch, drain, kickback, tilt, flipper, nudge, sling; `/nosound`, S/F5 toggle | Original mixed WAV samples/music and exact source timing not ported; physical PSG/VGA quality pending (F-007) |
| Live table lamps | 7 source-derived bumper caps flash on hit, source sling lights flash on contact, kickback lamps show armed/used state; misplaced invented shooter-ready dot removed; `/nolights` disables | Original `TLight` sprite overlays, target-based missions and timed lamp-group scripts not implemented; physical V20 cost unverified (F-007) |
| Diagnostics | `/record`, `INPUT.TXT`, `FLIGHT.TXT` (schema 4), slow-frame and both outer-rail counters; PC-side `analyze_logs.py` | Buffer until exit, not long-term historical telemetry |
| Build/repro | Open Watcom C89 compilation of `main.c`, `game.c`, `table.c`, `scores.c` with named `.inc` fragments; source-layout, converter, high-score and gameplay tests; DOSBox-X Tandy/VGA parity and `CADET.zip` artifact | [Post-refactor CI run 42422864888](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888) passed; emulator pass does not prove physical V20 or VGA timing |

**Video compatibility addition (2026-11-07):** both standard BIOS
Tandy/PCjr mode 09h and VGA mode 13h are implemented in the **same**
16-bit executable. Default behavior detects a VGA BIOS and uses mode
13h if available; otherwise the original Tandy renderer is selected.
Use `/tandy` or `/vga` to force a supported backend. The VGA
palette has 256 available entries, but the game **uses only the same
16 RGBI colors** as the Tandy conversion. No second game, table, or
background art dataset has been created. VGA 13h needs a 64000-byte
far background versus the Tandy's 32768-byte one. The physics, original
resource geometry, mouse flipper controls and game rules are identical.
The Tandy CRTC shake is disabled on VGA; PC-speaker beeps replace
Tandy-only PSG output. Both emulated backends have produced identical
final frames and scored gameplay, but real VGA hardware compatibility
and frame timing remain unverified.

## Partially implemented, approximated, or not faithful to source

| Area | What is present | What is still **not** implemented | Fidelity audit |
| --- | --- | --- | --- |
| World physics | Q6 fixed-point 2D ball, wall normals, gravity, speed clamps, collision bands, bounded anti-stuck watchdog | Original 3D/world-space position and height, spin, full material friction/elasticity, stochastic drag, continuous sweep, exact collision responses | F-002, F-006 |
| Ramps and layers | Initial ramp art and selected projected ground rails | Physical ramp elevation, traversable upper/lower layers, occlusion masks, level-dependent rail collisions and accurate ramp exits | F-002 |
| One-way rails and gates | Special upper-left passage, source-derived **left/right** exterior rail containment and **solid left/right slingshot hulls** that reject the ball from closed sling interiors | General original `TOneway`, gate orientation, open/closed state and per-group collision filtering | F-002 |
| Flippers | Source rest/raised geometry; midpoint and seven-substep assisted motion, leverage-based impulse, plus 5px inward active-tip forgiveness for near-center descents | Original continuous angular sweep/velocity and exact front/back collision, trap/hold and original power curve; intentional easier-than-original gap handling | F-006 |
| Shooter | Original plunger frames and re-catch of weak return | Original 25 ms boost progression, ~2.5 s charge curve, full ~10% boost variance and exact shooter chute transitions | F-003 |
| Bumpers | Seven radial caps, basic initial scoring | Variable score state, independent component cooldowns, full original material/layer interaction | F-004 |
| Kickbacks | Two timed saves, strike centers from source | Hazard Target triplet activation, gate enable/disable, source timer/state/rearm, actual mission coupling | F-001 |
| Nudge/tilt | Three directions, reversal impulse and warning/lockout | Exact source threshold, accumulation/decay and physically accurate displacement | F-005 |
| Visual effects | Reduced-color artwork, small dynamic event/armed lamp overlays, original metallic ball sprite, optional Tandy CRTC shake | Full source-derived light sprite animations, switch/mission control and depth/occlusion rendering | F-007 |
| Audio | Eight synthetic PSG/PC speaker multi-note cues with audio priority | Source WAV samples, original soundtrack/music playback, independent source audio channels and all target/mission event mapping | F-007 |

**Universal recovery (2026-11-07):** The DOS port adds a
source-inspired F-008 anti-stuck watchdog. The original has an
`UnstuckBall` routine that first launches upward and eventually
relaunches; DOS uses 110 near-stationary physics steps before a gentle
upward push, followed by small geometry-checked relocation attempts
every 73 steps, then a free relaunch if nothing works. It does not
consume balls or award points. Flipper trapping, waiting/plunger
movement, kickback hold and normal drains are exempt. Recovery
attempts appear in `INPUT.TXT` under `unstick_nudges`,
`unstick_moves`, and `unstick_relaunches`. This remains a
source-inspired approximation, not verified original timing.

**Important:** The source-derived one-sided **left/right** exterior-rail
guards, two filled slingshot collision hulls, and top/side screen
safeguards are **temporary playability repairs**,
not restored source collision groups. The right guard skips valid shooter
travel. Both only cover the vertical intervals of their source rail segments.
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
- **Full multiplayer/settings system:** multiple alternating players,
  original long-name/high-score dialog and desktop configuration-backed
  persistence, plus the full Windows Game/Options dialogs. The DOS
  top-five `SCORES.DAT` and three-initial entry are now implemented,
  but remain a simplified standalone version of `high_score.cpp` (F-009).
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
| Unreachable center drains even with well-timed flippers | 5px invisible collision reach on each **held** raised toe; no assist on release or tilt, gap not fully closed | **Pending**: repeated center descents on physical hardware, check save frequency/CPU timing |
| Ball wedged near the upper-left bumper | Radial contacts, separate neighboring static-wall correction | **Pending**: repeated full-strength launches and chute returns |
| Ball seemed to disappear into the upper-left playfield | Top/side safety rail, event/flight tracing | **Pending**: confirm correct ramps and no hidden trapping |
| High-speed ball flew to upper-right HUD boundary | One-sided source-projected right outer rail guard, `right_rail_saves` logs, shooter exemption |
| Ball becomes trapped inside right slingshot (and potentially left) | Solid collision-only triangular sling hulls from original closed outlines; outward ejection and separate `sling_right_rescues`/`sling_left_rescues` counters | **Pending**: real Tandy right-sling play test and 7 MHz cost check | **Pending**: confirm ball remains inside playable table without unnatural bounce |
| Tandy V20 responsiveness and 3D art legibility | Fixed-point, dirty rectangles, offline shading, `/classic` | **Pending**: sustained play, input response, `slow_frames` and side-by-side CRT inspection |

**Optional mouse flippers follow-up (2026-11-07):** When a compatible
DOS driver is loaded, the left and right mouse buttons mirror Z and /
flippers via INT 33h button state and press counts. A quick click is latched
even if released between display frames. Both mouse buttons can be held at
once, and keyboard/mouse inputs are combined. No cursor/mouse movement
feature is included; `/nomouse` disables mouse support. Without a driver
the keyboard remains fully functional. **Physical serial mouse and V20
latency/performance validation is still pending.** This adds an input option,
not any original gameplay rules.

**Latest controls and bounds follow-up:** The 3×5 menu font previously
rendered `A` exactly like `H`; the F1 help now clearly says
`A LEFT BUMP`, `D RIGHT BUMP`, and `X UP BUMP`. `H` is not assigned.
A new source-derived *left* rail recovery is tested alongside the right
rail, and `FLIGHT.TXT` schema 4 records `LEFT_RAIL` and `RIGHT_RAIL`
counters. These are safety repairs, not new game rules. Hardware validation
remains open.

For investigation, run `CADET /record`, quit with Esc immediately after a
failure, then collect `INPUT.TXT` and `FLIGHT.TXT`. `analyze_logs.py` on a
PC can summarize them. `FLIGHT.TXT` stores only the last 256 rendered
frames (~14 seconds), so it is not a permanent replay.

## UI and high-score additions (2026-11-07)

The existing side panel now displays short event messages from the
original event codes. The menu exposes `HIGH SCORES` and the program
prompts for three initials only if a completed game enters its top
five. `scores.c` validates and persists five ordered scores in local
`SCORES.DAT`; it is not a Windows settings file. Middle-click launch,
F5 sound and F8 controls restore original options.cpp bindings. This
work has **no new physics behaviors** and adds no disk I/O to the
active gameplay loop. The full original `TTextBox` queue, Windows
score dialog/settings and source missions remain incomplete (F-007/F-009).

## Source-aligned implementation

The [source map](SOURCE_MAP.md) identifies each gameplay and DOS platform
fragment and its original C++ counterpart. `table.c` is compiled
independently for SCP2 loading and indexing; the other functions are
grouped into `.inc` implementation fragments included by `game.c` or
`main.c` in their original order. `flipper.inc` and `plunger.inc`
remain inside `game_step`; they are not separately compiled or called
as new per-frame functions. `test_source_layout.py` guards this layout.
Source organization does not implement absent rules or close any
[F-001 through F-008](FIDELITY.md) fidelity exceptions. See
[successful CI run 42422864888](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888);
physical V20 responsiveness is still unverified.

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

Both artwork modes use the same gameplay logic and original imported ball
sprite without the removed glint/shadow. The `TABLE3D.TGA` effect is
precomputed offline and adds no per-frame shading work.
Tests include `test_source_layout.py`, `test_menu_font.py`,
`test_convert.py`, `test_analyze_logs.py`, `test_enhance_table.py`,
`test_run_tests.py`, the 16-bit `TEST.EXE /original` scenarios,
`verify_build.py`, and 360-frame DOSBox-X smoke runs. See [`VALIDATION.md`](VALIDATION.md) for evidence and caveats.

## Project disposition

The port demonstrates Space Cadet running in native 16-bit Tandy
graphics but remains **incomplete**. Future work can use
[SOURCE_MAP.md](SOURCE_MAP.md) to restore original behavior systematically,
prioritizing real Tandy playability and documenting deviations in
[FIDELITY.md](FIDELITY.md). Missing systems are not optimizations,
and an emulated pass is not measured V20 performance.

## VGA graphics update (2026-11-08)

VGA1 source-color table and thirteen moving sprites are now implemented at
320x200, with source-derived palette colors plus reserved RGBI UI colors.
This supersedes earlier RGBI-only VGA notes. `/rgbi` retains the older look
and parity tests. Source geometry and gameplay are unchanged; original
high-resolution presentation, dynamic lighting and Z-buffer layers are still
missing. See F-007 and SOURCE_MAP.md for the adaptation and hardware limits.

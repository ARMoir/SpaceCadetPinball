# Repository instructions: DOS Space Cadet project

These instructions apply to **ARMoir/SpaceCadetPinball**, especially the
`Platform/dos/` native Tandy 1000 EX/V20 port. They do not set policy for any
of the owner's other repositories.

## Mandatory first read

Before editing anything under `Platform/dos/`, read
**[`Platform/dos/FIDELITY.md`](Platform/dos/FIDELITY.md)**. It is the
**standing engineering audit and exception register**. The project's
**top priority is playable, responsive, enjoyable gameplay on the real
7.16 MHz V20 Tandy 1000 EX**. A very close second is accuracy to original
Space Cadet gameplay and presentation. Consult the actual `SpaceCadetPinball/` C++ source and
Windows `PINBALL.DAT` data, not memory, when judging original behavior.

## Priority order for decisions

1. **Playability on actual hardware first.** Responsive controls, reliable
   collisions, enjoyable ball motion, and stable performance on the real
   Tandy take precedence when the two goals genuinely conflict.
2. **Original accuracy a very close second.** Start from the source code and
   preserve its mechanics wherever doing so still plays well. Favor source-
   inspired corrections over arbitrary impulses, random paths, or shortcuts.
   **Owner's 2026-11-07 preference:** where original mechanics are missing
   and a simplified gameplay decision is genuinely ambiguous, favor a
   forgiving, enjoyable player experience. Keep real skill and the chance
   to drain; document any easing as an explicit fidelity exception.
3. **Every tradeoff must be traceable.** Record the source behavior and the
   Tandy-specific rationale in `Platform/dos/FIDELITY.md`, especially if
   playability requires different mechanics. A user-observed hardware problem
   is evidence of a playability problem; the replacement still needs tests
   and hardware verification before being called solved.

## Rules for changes

- **Keep original-to-DOS provenance current.** Read
  [`Platform/dos/SOURCE_MAP.md`](Platform/dos/SOURCE_MAP.md) before
  modifying any mapped component. Update its matches, differences and
  file ownership as implementations evolve, while preserving
  `FIDELITY.md` as the authoritative exception register.
  The `.inc` files are implementation fragments included by
  `game.c` or `main.c`, never independently compiled; preserve their
  include order and run `test_source_layout.py`.
- **Preserve original logic and rules.** Port original state, component
  triggers, timings, scoring, collision direction, physics semantics and
  visual resources wherever feasible. Simplify *how the code runs*, not
  *what the game does*. C89, integer/fixed-point math, bounded collision
  indexing and dirty-rectangle video are acceptable.
- **Do not sacrifice real Tandy playability for literal parity.** If a
  source-faithful implementation makes gameplay slow, unreliable, or less
  enjoyable on the V20, choose the smallest practical Tandy adaptation and
  document the tradeoff in the exception register. Convenience alone is not
  a valid reason to invent replacement rules. Missing work is not a
  performance optimization.
- **Any deviation must be explicit.** Before shipping a gameplay-changing
  approximation, update the exception register in
  `Platform/dos/FIDELITY.md`, with original source reference, difference,
  gameplay impact, reason, measured evidence (or **UNVERIFIED**), tests and
  plan to restore source behavior. Reference the associated **F-###**
  exception ID in the commit and/or code comment.
- **Use original component data before screen-coordinate heuristics.** For ramp,
  gate, one-way, target, or collision-layer corrections, first inspect the
  corresponding original C++ component and PINBALL.DAT attribute fields.
  Prefer projected source endpoints, group masks, and component states
  indexed at load time over hardcoded global x/y regions. When an original
  source asset is unavailable, explicitly mark the fallback as provisional:
  do not claim its geometry or direction comes from omitted source data.
- **Do not infer component identity from a similar outline.** Preserve original
  object type, group ID, attribute IDs and input SHA-256 in the import report.
  A four-edge static shape is not a TGate merely because it resembles a door.
  Generated component extensions must be regenerated from the same DAT and
  validated with their executable; preserve legacy-pack rejection/fallback tests.
- **Bound and audit every forgiving gameplay intervention.** Specify a
  precise trigger, phase/approach side, a maximum velocity or displacement
  effect, exclusions (tilt, kickback, plunger, legitimate return, etc.),
  reset/timeout and retained failure/drain paths. Do not continuously
  accelerate, teleport, or invent scoring. Add tests of positive, negative,
  simultaneous, and repeated-contact cases on source-derived geometry.
- **Instrument gameplay assistance for real-hardware diagnosis.** Every
  ramp crest, directional deflection, collision rescue, anti-loop or
  relaunch intervention must increment a bounded counter reported by
  CADET /record in INPUT.TXT and the deterministic /smoke report.
  Counters are descriptive, not proof of a good outcome. Do not add disk
  I/O inside the physics loop; use small in-memory integer counters.
- **Synchronize the three ownership documents.** In the same change as
  modified behavior, reconcile FEATURE_STATUS.md (present/missing
  functionality), FIDELITY.md (source behavior, exception and evidence),
  and VALIDATION.md (automated tests versus unverified physical acceptance).
  Update SOURCE_MAP.md and AGENTS.md when provenance or rules change.
  Retire contradictory historical claims in summaries without deleting
  the history of justified exceptions.
- **Resolve high-impact playability issues first.** Hardware-reported
  weak flippers, repetitive ball paths, missed contacts, stuck balls,
  unreliable shooter relaunch, and slow frames outrank less visible fidelity
  debt. Within equal playability impact, address P0 original-mechanics debt
  (kickback rules and one-way/layer behavior) before lower-value features.
  See the full audit for details and current status.
- **Test against the source, not just current DOS behavior.** Run
  `test_source_layout.py`, `test_menu_font.py`, `test_convert.py`,
  `test_run_tests.py`, `build.py`, `verify_build.py`, and
  DOSBox-X gameplay/smoke tests where applicable.
  Record real Tandy performance separately; successful emulation does not
  prove a 7.16 MHz V20 budget.
- **Use matching data.** When the format or imports change, ship
  `CADET.EXE`, `TABLE.DAT`, `TABLE.TGA`, `TABLE3D.TGA`,
  `TABLESRC.TGA`, `VGA.DAT`, and `MENU.DAT` from one tested build together.
  Optional assets retain explicitly documented fallback behavior. Keep
  the Actions archive named `CADET.zip`.

## Repository workflow specific to this project

- Use **only `dos/tandy-native-prototype`** for ongoing DOS work.
- **Never edit `master`**, create a new branch, or open a pull request
  without the project owner's explicit request.
- For a gameplay change, include source references, affected F-### IDs,
  positive/negative regression tests, and any remaining hardware uncertainty
  in the change summary.
- A documentation-only fidelity audit does not authorize behavioral changes.

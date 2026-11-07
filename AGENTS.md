# Repository instructions: DOS Space Cadet project

These instructions apply to **ARMoir/SpaceCadetPinball**, especially the
`Platform/dos/` native Tandy 1000 EX/V20 port. They do not set policy for any
of the owner's other repositories.

## Mandatory first read

Before editing anything under `Platform/dos/`, read
**[`Platform/dos/FIDELITY.md`](Platform/dos/FIDELITY.md)**. It is the
**P0, highest-priority engineering specification** for preserving original
Space Cadet gameplay. Consult the actual `SpaceCadetPinball/` C++ source and
Windows `PINBALL.DAT` data, not memory, when judging original behavior.

## Rules for changes

- **Preserve original logic and rules.** Port original state, component
  triggers, timings, scoring, collision direction, physics semantics and
  visual resources wherever feasible. Simplify *how the code runs*, not
  *what the game does*. C89, integer/fixed-point math, bounded collision
  indexing and dirty-rectangle video are acceptable.
- **Original behavior always wins over convenience.** Do not replace source
  rules with an invented shortcut just because the shortcut is playable or
  easier to implement. Missing work is not a performance optimization.
- **Any deviation must be explicit.** Before shipping a gameplay-changing
  approximation, update the exception register in
  `Platform/dos/FIDELITY.md`, with original source reference, difference,
  gameplay impact, reason, measured evidence (or **UNVERIFIED**), tests and
  plan to restore source behavior. Reference the associated **F-###**
  exception ID in the commit and/or code comment.
- **Resolve existing priority debt before adding lower-value features.**
  P0: kickback target/gate state and source one-way/layer behavior.
  P1: plunger timing/variance, bumper scoring, nudge/tilt calibration,
  flipper motion. See the full audit for details and current status.
- **Test against the source, not just current DOS behavior.** Run
  `test_convert.py`, `test_run_tests.py`, `build.py`,
  `verify_build.py`, and DOSBox-X gameplay/smoke tests where applicable.
  Record real Tandy performance separately; successful emulation does not
  prove a 7.16 MHz V20 budget.
- **Use matching data.** When the format or imports change, ship
  `CADET.EXE`, `TABLE.DAT`, and `TABLE.TGA` together. Keep the Actions
  artifact download named `CADET.zip`.

## Repository workflow specific to this project

- Use **only `dos/tandy-native-prototype`** for ongoing DOS work.
- **Never edit `master`**, create a new branch, or open a pull request
  without the project owner's explicit request.
- For a gameplay change, include source references, affected F-### IDs,
  positive/negative regression tests, and any remaining hardware uncertainty
  in the change summary.
- A documentation-only fidelity audit does not authorize behavioral changes.

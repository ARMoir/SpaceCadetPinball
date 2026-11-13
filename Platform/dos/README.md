# Space Cadet: native 16-bit DOS / Tandy and VGA prototype

This is a separate C89, 16-bit real-mode target for an NEC V20 / 8088-class
Tandy 1000 **and standard VGA-compatible DOS PCs**. It does not use SDL,
Windows, a DOS extender, an FPU, XMS or EMS.
The desktop version is unchanged. This is a playable experimental prototype,
**not** a feature-complete port or a final hardware-certified release.

**Documentation guide:** [SOURCE_MAP.md](SOURCE_MAP.md) maps the refactored
DOS implementation to original C++ code; [FEATURE_STATUS.md](FEATURE_STATUS.md)
lists implemented and missing features; [FIDELITY.md](FIDELITY.md) owns
the open F-001 through F-008 exceptions; and
[VALIDATION.md](VALIDATION.md) records CI evidence and physical
hardware checks. Consult [AGENTS.md](../../AGENTS.md) before edits.

## Project priorities: playability first, authenticity a very close second

**First priority:** make Space Cadet smooth, reliable, playable, and fun on the
real Tandy 1000 EX with a 7.16 MHz V20. Ball motion, flipper strength, input
response, shooter return, and stable frame timing must work on the machine.

**Very close second:** preserve original Space Cadet logic, rules, physics,
artwork, and behavior wherever practical. Start from the source and optimize
implementation (fixed-point math, precomputed geometry, bounded collision
checks, and efficient Tandy rendering) before changing mechanics. If restoring
source behavior conflicts with actual hardware playability, use the smallest
playable adaptation and document the difference and its evidence. Where the
original behavior cannot be reproduced accurately with simplified DOS
mechanics, choose the **more forgiving, fun** interpretation rather than
making the player lose balls because of the approximation. Keep the
possibility of genuine missed shots and normal drains.

Before each DOS code or data change, consult the standing
[**fidelity audit, source references and exception register**](FIDELITY.md)
and the repository [`AGENTS.md`](../../AGENTS.md). If a necessary hardware
constraint changes gameplay behavior, document the deviation, original
source, reason, measurement (or explicitly **UNVERIFIED**), tests, and return
path in the same change. Reference its **F-###** entry in the commit summary.
An unported feature or unmeasured workaround is not a justified optimization.

**Work order:** prioritize hardware-reported weaknesses such as underpowered
flippers, repeated ball paths, missed contacts, stuck balls, and launcher
recovery. Next, restore the highest-impact original rules (P0 kickback/gate and
collision-layer behavior, then P1 plunger, bumpers, nudging and flippers).
A successful emulator build does not establish hardware performance or parity.

The F1 help menu now clearly spells out **A LEFT BUMP**, **D RIGHT BUMP**,
and **X UP BUMP**. An earlier tiny-font defect made the letter A look
exactly like H; **H is not a control key**. The A glyph is now distinct
from H and checked by an automated font regression test.

## Space Cadet title graphics for Tandy and VGA (F-007)

The owner's 600x600 Space Cadet splash is now the source for both DOS
menu presentations. An **offline**, checksum-checked RGBI conversion
produces `MENU.DAT` from `data/menu_art.b64` using
`build_menu_art.py`. It includes a **240x200** full-height, left-side
Space Cadet illustration and a **76x53** copy for the right-hand HUD
logo. The six-file runtime package includes `MENU.DAT`; copy it with
the other files from the same build.

Open **M** to display the new title illustration on the left and the
keyboard-controlled game menu at right. Other dialogs (controls, high
scores and initials) retain that title illustration. The gameplay
playfield is restored when you close the menu, with ball, flippers and
plunger repainted. **Neither physics nor the imported playfield/sprite
artwork is changed.** Both video modes use precisely the same 16 RGBI
indices. Tandy copies 120 packed bytes per image scanline into mode 09h
banked B800h video; VGA expands those pairs to A000h mode 13h. This is
a one-time menu-transition operation; there is **no image decoding,
file access or full-screen redraw on each gameplay frame**.

The pack uses 26,014 bytes of additional far memory only if the file is
available, and is loaded before gameplay. If `MENU.DAT` is missing,
corrupt or cannot be allocated, the earlier source-derived title/menu
still works without it. `build_menu_art.py` also generates
`MENU_PREVIEW.png` for review without DOSBox-X. This title conversion
preserves the supplied composition with the fixed Tandy palette, not
its original 24-bit colors. The reference video illustrates the desktop
game, but DOS remains a simplified 320x200 port; exact desktop visual
parity and actual 7.16 MHz V20 timings remain **UNVERIFIED**.

## November 9 source-ramp clearance and contact feel (F-002/F-006)## November 13: source-based ramp traversal audit (F-002)

The Windows source contains two TRamp objects: **group101**, a short
18-segment rising curve on the left, and **group105**, the long flat
upper-right elevated path. Cadet now uses both source paths to handle
ball-radius-aware entry at segment joins and to suppress projected
ground collisions while the ball is on an actual elevated ramp.
The long upper-right path also has a consistent 5px fast-search
boundary; the 3px detector remains on the rising ramp.

The 18-segment ramp previously let the ball fly straight through the
curve. A **bounded steering correction** follows the source tangent,
maximum 24 Q6 sideways velocity per physics substep, only when the
ball is already traversing the rising ramp forward. It does **not**
teleport the ball, guarantee completion or add a speed boost; the
existing weak-shot boost still triggers only at 90% progress.
The flat upper-right path is unsteered and unboosted.

For hardware A/B testing, try two sessions with `CADET /record`, one
on each ramp. When you exit, `INPUT.TXT` reports separate
`ramp_short_entries`, `ramp_short_crests`,
`ramp_right_entries`, `ramp_right_crests`, and
`ramp_steer_ticks`. A crest count is stronger evidence than simply
entering a ramp; compare `FLIGHT.TXT` for actual trajectories.
The original upper-layer walls, true plane damping/gravity and
full entry/exit mask switching are still not ported, and improvement
on a physical 7.16MHz V20 remains to be verified.



The original Windows PINBALL.DAT identifies the upper-right passage as
source TRamp group 105, a **flat elevated plane**, not the rising ramp
group 101. In 2D, nearby original **ground** walls had become a false
barrier near its crest. The latest DOS collision code honors a bounded
source-path elevated mask, ignoring ground walls only during an actual
upper-right traversal and for a few steps beyond its exit. It does
**not** add a stronger launch or unconditional "easy" ramp kick.
Full upper-layer side rails, gravity, and height transitions are
still approximations and need physical hardware testing.

For less repetitive flights, meaningful bumper and boosted-flipper
contacts receive an independently seeded tiny tangential velocity
variation (maximum 8 Q6 units) representing missing spin/material
effects. Ordinary wall bounces, shooter returns and earned kickbacks
remain deterministic. All changes use integer math and occur on
contact rather than continuously.

On the Tandy, try `CADET /record`, play several upper-right ramp
climbs and boosted flipper shots, then quit normally. `INPUT.TXT`
reports `ramp_layer_ticks` and `physics_variations` on its
comment lines. These counts show whether the new behavior activated,
not whether it actually felt correct. The last seconds of flight
remain in `FLIGHT.TXT` for PC-side analysis with `analyze_logs.py`.
Compare with the earlier tested build if the ball travels through
a formerly solid sidewall or you notice an unfair new trajectory.
The exact 7.16 MHz V20 CPU impact and real-screen experience are
UNVERIFIED.

## November 8 gameplay tuning: ramps, slingshot loops and rear-lane shots

A nearly completed climb on the source-projected upper-right or upper-left
ramp now receives a small **one-shot** uphill velocity correction.
These positions remain a DOS-only approximation; the original ramp
planes and TOneway lines have not yet been imported. For actual Tandy
playability diagnosis, /record and /smoke now include
`ramp_crest_assists` and `sling_backflow_redirects` counters in
the same report as `sling_loop_breaks`. The
slingshot anti-loop guard triggers at the **third** alternating real front
contact, launching mostly upward rather than toward the opposite sling.
A directional lower-sling entry deflector redirects outward flipper shots
away from the rear tube while leaving downward return balls, the normal
shooter lane and genuine kicker flights unchanged. No global invisible
walls, teleporting, extra score or new runtime assets are involved.
See [F-002 and F-008](FIDELITY.md) for the source-deviation details.
Actual 7.16 MHz V20 behavior remains to be tested.

## Source-style sidebar and original score numerals (F-007)

During gameplay, score changes now update only the small score-digit inset, and
new event messages update only the status text. The static title, frames and
other HUD fields are not redrawn on each bumper hit. Full sidebar composition
is reserved for menu/view changes and explicit UI transitions (F-007).


The default 80-pixel sidebar now follows the original game's boxed
layout shown in the owner's source screenshots: large title artwork and
red BALL counter at the top; separate player number and score boxes;
a two-line F2/continue/game-over action box; and a large lavender
3D PINBALL / SPACE CADET status panel. The former generic BALL/SCORE/
STATUS/control stack is no longer shown during regular gameplay.
Menu, controls, initials, and high-score screens still use the same
keys and provide the existing information.

The displayed score digits were extracted from the owner's original
`FONT.DAT` (PARTOUT(4.0)RESOURCE, copyright 1994 Cinematronics).
The ten 15x22 source bitmaps are converted offline to 5x8 2-bit
shaded RGBI glyphs, requiring **100 bytes** of static DOS font data.
Typical scores use one pixel of glyph spacing; 9/10-digit scores
tighten spacing so they still fit the 54-pixel score box. The full
font resource is **not loaded by the DOS game**. The existing 3x5
alphabet is preserved for long controls/menu instructions.

Both Tandy mode 09h and VGA mode 13h draw the same 16-color borders,
labels and original numeral shapes. The menu art still comes from
`MENU.DAT` (with the prior fallback when absent). Only the sidebar
dirty rectangle changes on score, ball or status updates; the
playfield, flippers, physics, and 18.2Hz frame cadence are unchanged.
The appearance is source-inspired rather than pixel-identical,
particularly because the Windows sidebar is much larger. Physical
Tandy CRT readability and real V20 timing are **UNVERIFIED**.

## Working event lights and richer sound (F-007)

The native 16-bit game now uses **eight short hardware tone patterns** for
bumper hits, plunger releases, ball drains, kickback saves, tilt alarms,
flipper clicks, nudges and slingshot rebounds. The Tandy outputs them
through its PSG, with simple two-voice accents on launch/kickback, while
VGA PCs use PIT channel 2 and the PC speaker. Sounds are paced by the
existing 18.2Hz frame clock, without sample streaming or delays.
**F5** or **S** mutes/unmutes, and `CADET /nosound` starts silently.

Playfield lights also respond: the seven original imported bumper caps
flash briefly on scoring hits, both slingshot areas flash on rebounds,
left/right outlane indicators show unused saves and pulse when consumed.
The earlier shooter-ready dot was removed because its estimated position
floated next to the ball; the HUD already displays READY. These are small
source-positioned 16-color highlights with dirty-rectangle restoration,
not the original Windows
animated light sprites or complete mission logic. The existing artwork,
ball sprite and physics remain unchanged. Use `CADET /nolights` to
compare 7MHz V20 responsiveness or disable visual flashing. Both
Tandy mode 09h and VGA mode 13h share the same lamp positions and
color indices; no additional disk assets are required. Hardware sound
quality and real CRT timing remain to be confirmed.

## Shooter resting ball alignment (F-003)

The original imported plunger sprite begins at x=184,y=156, while the
projected source launch anchor was x=181,y=141. This made the 4-pixel
ball appear suspended too far above the plunger. The DOS port now seats
it immediately over the original unmodified rod tip (center x=186,
y=153). All subsequent launches and free returns begin from that
visible resting position, with no sudden snap on release. No source
assets or gameplay impulses were changed. The source 3D-to-2D placement
is still approximated, and real Tandy CRT inspection remains pending.

## Passive slingshot backs (F-002)

The inward rubber faces of the two slingshots still produce normal
bounces. Their outer backs, facing the kicker/outlane tubes, instead
act as unpowered guides: they keep the ball outside the triangle,
but slide off incoming normal motion instead of kicking it back
toward the launcher. These rear contacts also do not count toward
the four-hit left/right anti-loop rule or create a false sling
flash. Duplicate internal collision outlines in the source pack
are filtered at table loading. The source-positioned kicker gate
on the right is separate and only becomes solid after the kicker
fires. This corrects a reported physical-Tandy bounce artifact;
actual hardware trajectories remain to be confirmed.

## Preventing endless slingshot ping-pong (F-008)

A ball that keeps bouncing left/right between the two triangular
slingshots moves too far for the original 3-pixel anti-stuck watchdog.
The native DOS simulation now detects genuine incoming alternating
sling collisions. **Three or fewer consecutive left/right contacts are
normal pinball gameplay.** On the fourth sustained alternating hit
(with no gap over roughly 2 seconds), the ball receives a modest
upward-and-inward escape impulse. Physics continues normally: no
teleport, no free ball, no invented points and no altered kickback
availability. Contacting the same sling repeatedly or leaving the
area resets/does not advance the sequence. The fix adds three bytes
of state and a cumulative `sling_loop_breaks` counter to `/record`
and `/smoke` reports. It targets the simplified DOS collision-layer
model; whether it prevents every hardware-reported loop on the actual
7 MHz Tandy is **UNVERIFIED**. See [F-008](FIDELITY.md).

## High-return outlane saves (F-001)

After either *armed* outlane kickback catches a descending ball, the DOS
simulation holds the original strike location briefly, then sends it
upward in two stages. The right side first climbs outside the right
slingshot before angling inward. Both sides reach the upper quarter of
the table (y<=48) before ordinary collisions resume. The temporary
guided flight is a forgiving adaptation for the port's incomplete 3D
collision layers, not a replacement for original target/gate rules.
Each side still saves **only once per ball**; unarmed contacts and
tilted balls are never protected. The source-derived narrow gates
now open while the kicker is armed and *close visually and physically*
after their kickback timers fire, just as `RightKickerControl` calls
`TGateEnable` in the original. The door is drawn using a tiny metal
leaf over the imported x/y gate location, independently of `/nolights`,
and reopens when a new ball is served. The full original Hazard Target
arming sequence and timing remain unported. Real 7.16 MHz V20
behavior remains to be tested. See [F-001](FIDELITY.md).

## Build and play

1. Unpack Open Watcom 2 and set `WATCOM` to its absolute installation directory.
2. From the repository root, run `python Platform/dos/build.py` (Python 3.9+).
   This builds `CADET.EXE` from `main.c`, `game.c`, and `table.c`,
   and `TEST.EXE` from `test_game.c`, `game.c`, `table.c`.
   Both link `scores.c`, a separate setup/menu-only C89 high-score module.
   Named `.inc` fragments are included by their parent files, never
   compiled as independent objects.
3. Copy `TABLE.DAT`, `TABLE.TGA` and `IMPORT.json` from
   `Platform/dos/data` into `build/dos`, or regenerate these from a Windows
   `PINBALL.DAT` using
   `python Platform/dos/convert.py /path/to/PINBALL.DAT --output build/dos`.
   Full Tilt CADET.DAT is not supported. `make_demo.py` produces an asset-free
   diagnostic table, not Space Cadet.
4. Generate the optional, CPU-free enhanced static background on your PC:
   `python Platform/dos/enhance_table.py --table build/dos/TABLE.TGA --pack build/dos/TABLE.DAT --vga-source build/dos/VGA.DAT --output build/dos/TABLE3D.TGA --preview build/dos/PREVIEW3D.png`.
   The additional source-faithful RGBI version is generated with
   `python Platform/dos/source_art.py --vga build/dos/VGA.DAT --pack build/dos/TABLE.DAT --classic build/dos/TABLE.TGA --output build/dos/TABLESRC.TGA --preview build/dos/PREVIEWSRC.png`.
   GitHub Actions runs both offline operations and packages all three looks.
5. Copy **`CADET.EXE`, `TABLE.DAT`, `TABLE.TGA`, `TABLE3D.TGA`, `TABLESRC.TGA`, `VGA.DAT`, and `MENU.DAT`** into
   the same DOS directory. `CADET` uses `TABLESRC.TGA` by default on Tandy;
   `CADET /classic` loads the original unmodified background.
   `CADET /3d` selects the earlier shaded style; `CADET /sourceart`
   explicitly selects the new default Tandy look. All styles preserve the
   same metallic ball without extra shadow pixels. Missing `TABLESRC.TGA`
   falls back to `TABLE3D.TGA`, then `TABLE.TGA`; a missing default
   `TABLE3D.TGA` falls back to `TABLE.TGA`.
   **Both video backends use exactly these same raw 32768-byte Tandy-bank
   source images.** VGA expands the selected image once at startup, into
   its linear 64000-byte background. No additional VGA assets are required.
   The source files are *not* Targa images. Keep the EXE and SCP2 `TABLE.DAT`
   from the same build.

Controls: **Z** or **left mouse button** for the left flipper,
**/** or **right mouse button** for the right flipper. Any combination
works simultaneously. Hold **Space** or the **middle mouse button** to
pull back the plunger and release to launch (about 1.3 seconds to full
power, with ~1% launch variation). Middle click requires a driver
reporting three buttons. **A** nudges from the left, **D** from the
right, **X** nudges upward. **F5** (or S) toggles sound, **F8** (or F1)
shows controls, **F2** (or N) starts a new game and **F3** pauses/resumes.
**M** opens the menu, including **HIGH SCORES**. **Esc** goes back or
exits from normal play. There are three balls.

**Top-five high scores:** qualifying game-over scores open an initials
entry screen automatically. Type up to three A-Z initials (unfilled
positions default to A); **Backspace** edits, **Enter** saves, and
**Esc** skips the entry. The five highest scores are visible from the
menu, including after saving. `SCORES.DAT` is a small local file in the **current DOS working
directory**. Start CADET from its installed directory to keep scores
beside the executable. The file is optional: a missing or malformed file
starts an empty table, and an unwritable directory displays
`SAVE FAILED`. The game **never writes score files while a ball is in
play**, and `CADET /smoke` reads/writes no score file. To retain
records across game upgrades, preserve `SCORES.DAT` when copying a
new CADET build. The original desktop five-entry high-score ordering
is preserved, but DOS uses three-letter initials instead of the
desktop's longer names and does not persist other menu settings (F-009).

**Optional DOS mouse controls:** load a standard DOS-compatible mouse
driver (such as MOUSE.COM or CTMOUSE) **before** starting CADET. The game
detects it using `INT 33h`. Without a mouse driver, no mouse polling
occurs and Z and / work normally. The mouse cursor is not needed, and
moving the pointer does not move the ball or paddle. Held buttons act
like held keys; DOS mouse-driver press counters also catch quick clicks
between the 18.2 Hz display frames. A driver reporting three buttons
also enables middle-click plunger input (held and quick-click).
Mouse polling occurs only once per rendered frame, never in the
72.8 Hz physics steps.

Start with `CADET /nomouse` to disable mouse detection and polling if
your driver is incompatible or you want to compare real V20 performance.
Emulator smoke tests intentionally disable optional mouse detection to
keep their keyboard input reproducible. On the physical Tandy EX, a
working serial-mouse driver is required; an attached mouse alone does
not provide an `INT 33h` driver. Mouse support is still pending physical
hardware verification.
`/nosound` starts with sound disabled, and **S** can re-enable it during
play. **F5** also toggles sound using the original desktop shortcut.
Switching sound off silences any currently playing tone immediately.
`/nomouse` disables optional DOS mouse input without affecting the keyboard.
`/noshake` disables optional hardware viewport shaking (use this if the
display shifts incorrectly on actual hardware). The small one-character
shake is done by a short MC6845 CRTC start-address offset and is restored
on pause, menu and exit, with no full-screen redraw.
### Video mode selection

| Startup | Behavior |
| --- | --- |
| `CADET` | **Auto-detect** standard VGA/MCGA via BIOS INT 10h AX=1A00h. Use VGA if detected, otherwise preserve the original Tandy mode |
| `CADET /vga` | Force **VGA BIOS mode 13h**, 320x200, 256-color-capable (existing **16-color artwork** is preserved), linear A000h framebuffer |
| `CADET /tandy` | Force the original **Tandy/PCjr BIOS mode 09h**, 320x200, 16 colors, banked B800h framebuffer |
| `CADET /vga /classic` | Standard VGA with the untouched original artwork |
| `CADET /tandy` | Default source-color-paired `TABLESRC.TGA` art, on explicit or BIOS-selected Tandy |
| `CADET /tandy /3d` | Previous enhanced static-shaded artwork, still available |
| `CADET /tandy /sourceart` | Tandy graphics with original VGA1-guided two-color RGBI artwork, baked offline |
| `CADET /vga /rgbi /sourceart` | The identical 16-color source-art background in VGA mode (without `/rgbi`, source-color VGA art takes precedence) |

These are **standard BIOS graphics modes**, not custom VGA timings, VESA,
or unchained VGA. `/vga` refuses to run if the VGA BIOS detection fails,
rather than blindly writing to unavailable video hardware. On a non-VGA
machine without compatible Tandy/PCjr graphics, the Tandy fallback still
requires actual Tandy-compatible hardware: plain CGA is not sufficient.

The only differences are display and sound hardware:
- Tandy mode uses packed 4bpp `B800h` with four interleaved 8KB banks
  and the existing Tandy PSG. The original 32KB far background, Tandy
  CRTC shake, sprite/pixel paths, and timing priorities are retained.
- VGA mode sets the 16 original RGBI colors into the standard VGA DAC,
  uses linear 8bpp `A000h` video and a **64000-byte far background**,
  copies matching 2-pixel dirty rectangles, and plays short PC-speaker
  tones instead of accessing Tandy PSG ports. The additional VGA colors
  are not populated from the full-resolution source resources, and
  **VGA still shows the same 320x200, 16-color artwork**.
- The 32768-byte Tandy source image expands into VGA memory only at
  startup. There is no full-screen redraw during normal gameplay.
  There is no extra conversion in the ball/physics loop, and the
  background is not duplicated in RAM.
- The Tandy-only CRTC screen jolt is intentionally disabled on VGA,
  where its addressing behavior differs. The selected artwork style
  can only be changed when launching the program.

The native program has a private IRQ1 make/break keyboard handler.
It restores the previous keyboard vector and BIOS video mode on normal
exit. Quit with Esc rather than terminating the emulator to test
cleanup. No PIT reprogramming: the BIOS timer drives 18.2 FPS with
four fixed physics substeps (about 72.8 Hz). Late frames are counted
and dropped rather than running unbounded catch-up work.

**Memory:** Tandy continues using a single 32KB far background; VGA
allocates one 64000-byte far background. Both use the same near-data
game and trace state; conventional DOS memory is required, and 640KB
is recommended. VGA rendering speed and PC-speaker output on a
physical PC are **not yet verified**. The BIOS chooses the initial
video mode, and both styles restore the previous text/graphics mode
when exiting normally.

## Source-aligned architecture

The refactor groups the DOS port by its original C++ responsibilities.
The [source map](SOURCE_MAP.md) identifies source functions, **limited
matches**, explicit divergences, missing features, and the corresponding
F-### exceptions. File names do not imply exact source-rule equivalence.

| DOS implementation | Responsibility | Original C++ references |
| --- | --- | --- |
| [`table.c`](table.c) | SCP2 file loading, geometry indexing, bumper/slingshot detection | `loader.cpp`, `proj.cpp`, `TEdgeManager.cpp` |
| [`scores.c`](scores.c), [`scores.h`](scores.h) | Top-five score ranking, validated file load/save after qualifying game ends | `high_score.cpp`; three-letter local file adaptation (F-009) |
| [`game.c`](game.c) + [`state.inc`](state.inc), [`control.inc`](control.inc) | Fixed-point state, feed/reset, frame-step collision and rules dispatch | `TBall.cpp`, `TPinballTable.cpp`, `control.cpp` |
| [`ball_collision.inc`](ball_collision.inc), [`bumper.inc`](bumper.inc), [`slingshot.inc`](slingshot.inc), [`rail_collision.inc`](rail_collision.inc) | Static/dynamic contact and simplified source-projected geometry | `TBall.cpp`, `TBumper.cpp`, `TEdgeManager.cpp` |
| [`flipper.inc`](flipper.inc), [`plunger.inc`](plunger.inc) | Inlined branches inside `game_step` | `TFlipper.cpp`, `TFlipperEdge.cpp`, `TPlunger.cpp` |
| [`kickback.inc`](kickback.inc), [`nudge.inc`](nudge.inc), [`unstuck.inc`](unstuck.inc) | Outlane saves, tilt and recovery | `TKickback.cpp`, `nudge.cpp`, `control.cpp` |
| [`main.c`](main.c) + [`input.inc`](input.inc), [`video.inc`](video.inc), [`video_shake.inc`](video_shake.inc) | DOS main loop, IRQ/mouse input and Tandy/VGA output | `winmain.cpp`, `options.cpp`, `render.cpp` |
| [`hud.inc`](hud.inc), [`audio.inc`](audio.inc), [`assets.inc`](assets.inc) | Transient event messages, HUD, initials/high-score panels, sound hardware and assets | `TTextBox.cpp`, `high_score.cpp`, `Sound.cpp`, `gdrv.cpp` |
| [`game.h`](game.h), [`convert.py`](convert.py) | Shared 16-bit state and offline asset conversion | `maths.cpp`, `loader.cpp` and original resource data |

**Compilation rule:** `table.c` and `scores.c` are separate C89 objects,
the latter used only for startup, game-over and paused-menu operations.
Table loading and indexing occur outside active gameplay. `game.c` includes
the physics/control fragments in their existing order, preserving
static-helper visibility with no additional V20 per-collision calls.
`flipper.inc` and `plunger.inc` are inserted *inside* `game_step`,
not called as new functions. `main.c` similarly contains its DOS
platform fragments in one translation unit. The source layout preserves `Game`/`Table` structures, physics
constants, SCP2 format and imported assets. High scores and messages
change only the DOS user-interface layer; no physics rule is added.

**Contributor workflow:** Start with the matching original
`SpaceCadetPinball/*.cpp` behavior; update [SOURCE_MAP.md](SOURCE_MAP.md)
for code ownership and matches/divergences; update
[FIDELITY.md](FIDELITY.md) for behavioral changes and
[FEATURE_STATUS.md](FEATURE_STATUS.md) for feature changes.
Keep include order and run `test_source_layout.py`, the native
Open Watcom build and DOSBox-X tests. The refactor passed
[CI run 42422864888](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888),
but physical 7.16 MHz V20 performance is not yet verified.

## Optional static faux-3D enhancement (F-007)

The 16-color original background remains **byte-for-byte unchanged** in
`TABLE.TGA`. `TABLE3D.TGA` is generated **offline** by
[`enhance_table.py`](enhance_table.py): existing gray ramps/rails gain
top-left highlights and down-right shading, and the seven source-derived
bumper caps receive subtle bevels/cast shadows. **The second, stronger offline
shading pass** adds clearer top-left metallic rims, darker lower-right rail
undersides, short contact shadows on existing gray metal near flipper guides
and shooter-lane hardware, and brighter domes/deeper undersides on the seven
original bumper caps. It only recolors pixels that were already gray or
white in the source image. **A third, purely offline trompe-l'œil pass**
derives a three-tier *visual-only* height mask from already painted metal
and the original seven bumper centers. One-pixel bright/dark Mach-band edges,
1–3 pixel *internal* sidewall shading, small ordered-dither contact shadows,
and longer offset shadows for higher bumper housings simulate extrusion.
The masks are not exported to the game or used for ball collisions.
**The final cleanup pass (F-007)** removes false black pits in existing
gray metal, one-pixel speckles, disconnected source-grey shading and
unwanted halos next to colored lamps. It preserves continuous metal
bevels and original pixel edges rather than blurring the bitmap. This
reduces checkerboard artifacts on the 320x200 Tandy CRT without adding
any drawing work at runtime. A fresh `PREVIEW3D.png` is generated as
validation evidence with each CI build. The original classic bitmap
is still available for direct A/B comparison.
Colored lamps and target art remain unchanged, and no perspective warp
or new collision geometry is introduced. `TABLE.DAT`, original ball/flipper/plunger sprite data,
gameplay state and scoring are identical between looks.

The default moving ball is now a **6x5 symmetrical circle** instead
of the original 4x4 shaded metallic sprite. At 320x200 on a 4:3 CRT,
pixels are about 20% taller than wide, so the 6x5 mask should appear
round. A uniform white center and dark gray rim give clear edges with
no fake 3D, corner glint or floor shadow. This is a runtime visual
override applied once after loading the unchanged original asset
packs, and it reuses the existing efficient sprite/dirty-rectangle
renderer. Run `CADET /sourceball` to restore the original 4x4 sprite
for an on-CRT comparison. This works with both video modes and both
`/classic` and `/3d`. Physics, collision radius and launch position
are completely unchanged. Visual results on a physical Tandy CRT
remain to be confirmed.

Compare the same shot in `CADET /classic` versus `CADET /3d`. Use
`CADET /record /classic` or `CADET /record /3d` to capture timing and
confirm the enhanced appearance does not produce an unacceptable slowdown.
The two artwork modes use the **same** collision and gameplay logic.

## Lightweight gameplay feedback

The HUD now displays the latest collision/event message for about
1.4 seconds in its existing status area: `BUMPER HIT`,
`BALL LAUNCHED`, `BALL LOST`, `KICKBACK` or `TILT`.
The original `TTextBox.cpp` uses prioritized queued timed messages.
This compact DOS version displays one event at a time, without allocating
a queue or redrawing the whole 320x200 playfield. A score-entry panel
or menu suspends game physics. These are user-interface additions
rather than new target/mission rules (F-007, F-009).

## What comes from the original

* Actual playfield bitmap, default-state object artwork, ball sprite, and
  resting/raised flipper sprites and eight plunger frames from PINBALL.DAT; scaled and quantized offline
  to the fixed RGBI palette. Display aspect assumes a 4:3 Tandy monitor.
* Original camera projection, ground-level static wall paths, flipper endpoints,
  plunger feed/animation positions, kickback strike centers and drain height.
  All seven bumper caps have scoring collision outlines fitted to their artwork.
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
  collision detection are absent. The shooter speed limits reach about
  2.06 pixels/substep vertically; powered playfield motion can reach about
  2.75 pixels/substep, before collision corrections.
* All seven source bumper caps have original artwork and imported eight-edge
  footprints. In F-004, the runtime now precomputes one **radial hit surface**
  per cap, with a consistent outward normal. This follows the original
  `TBumper` circular contact principle and prevents the old eight two-sided
  segments from pushing the ball into their shared interior. The three
  elevated launch bumpers are still flattened into one plane, so the source
  collision-layer behavior is incomplete. Most remaining static segments are
  two-sided capsules; collision groups are not reproduced. The upper-left skill-shot
  throat now has a small downhill-only passage, approximating the original
  TOneway gates so a ball does not get trapped by flattened ramp walls.
  **Both** original projected exterior rails are identified from
  `TABLE.DAT`: if a fast in-play ball slips outside a rail, a one-sided
  normal reflection catches it near the *table* edge, not the screen edge.
  The source-derived left guard applies only across its actual y=40..166
  interval; the right guard is **disabled while the ball is legitimately
  in the shooter tube**. The emergency top/side screen limits remain a
  final fallback. These are temporary F-002 playability safeguards, not
  original dynamic gates, ramps, or collision-layer semantics.
  The full ramp/one-way mechanics and final hardware verification remain
  incomplete. Other original gates and one-way edges still need explicit
  data import.
* Missed shots that return down the diagonal shooter tube return to the
  plunger without using a ball. The game tracks whether the ball has entered
  the main playfield, then catches a weak shot near the tube's lower end
  (before the shared drain line). Normal playfield drains still count.
* Ramps, elevation/occlusion, holes, sinks, gates, kickouts and rollover/target
  rules are not implemented. Their initial artwork can still be visible.
  Ground-only filtering prevents upper-level rails becoming invisible walls.
* The plunger uses eight original frames at their imported positions, with
  integer charge selecting the frame. Animation uses small dirty rectangles.
* Original rest/raised flipper endpoints are retained. Collision checks also
  cover a midpoint during a seven-substep fixed-point sweep, approximating
  the original moving flipper without trigonometry. **Powered contact now
  adds a normal-direction impulse scaled by pivot distance to the reflected
  incoming velocity**, rather than replacing it with a largely fixed shot.
  Playfield shots can reach 176 Q6 vertically; the shooter retains its
  tuned 132 Q6 cap. This prioritizes the hardware-reported need for more
  lively, controllable flippers. It is source-inspired, not exact angular
  physics; see [F-006](FIDELITY.md#f-006-flipper-and-collision-response).
  **Center-gap forgiveness (F-006):** the imported raised flipper tips
  remain about 18 pixels apart. Because the 2D sampled flipper sweep can
  miss shots that the original continuously rotating geometry might reach,
  DOS adds five pixels of *collision-only* inward toe reach when each
  flipper is deliberately held. A descending ball can bounce off this
  small extension just above the active raised tip, but released/tilted
  flippers retain normal geometry. The very center can **still drain**
  even with both buttons held. The tip extensions are never rendered,
  do not affect the art or `TABLE.DAT`, and use the existing impulse
  calculation rather than random saves. They are deliberately easier,
  not exact original flipper physics; hardware feel/CPU cost still need
  testing. The adjustable constant is `FLIPPER_TIP_ASSIST_PX`.
  The artwork still has only two states. Timing curves and accurate
  trapping are not yet reproduced. Static lights do not animate.
* Single ball/player, basic bumper scoring, drain/relaunch and restart only.
  Missions, jackpots, rank, bonus, skill-shot and multiball remain future work.
  A V20-friendly approximation of original nudge.cpp now applies additive
  velocity, its inverse after about 0.4 seconds, three bump directions
  (A, D, X), and a decaying tilt meter with warning and flipper lockout
  until the ball drains. This is not a 3D rigid-body simulation.
* Both outlane kickbacks use a short wind-up, then throw the ball upward
  and inward. Each side gets one save per ball. The original named Hazard
  Target triplets and gate enable/disable rules are not yet imported, so
  their per-side recharge logic remains an intentional approximation.
  Strike centers come from the source collision segments; trigger widths,
  timing and impulses are simplified. Walls stay solid throughout the return.
  The HUD shows `KICK L R`, or a dash for a consumed side.
* **Solid slingshots (F-002):** the original 14-segment closed left and
  right sling outlines are now automatically recognized from `TABLE.DAT`.
  Their centers are treated as solid triangular collision surfaces
  instead of 14 independent two-sided edges. Balls approaching from
  any side, especially the right slingshot's hollow middle, are
  pushed to the nearest outside surface and rebound outward, with a
  slight upward release at side/top faces to prevent sticking.
  The source art, sprites and table pack are unchanged. An inexpensive
  bounding-box check avoids costly math away from the slings. This
  deliberately favors reliable DOS playability over trapping artifacts
  from a flattened 2D collision import. `CADET /record` reports
  `sling_left_rescues` and `sling_right_rescues` in `INPUT.TXT`;
  re-test the actual right sling and performance on your V20.
  Other missing ramp/layer collisions may still trap the ball; nudge
  with X, A, or D if necessary, but repeated nudges can tilt.
* **Universal anti-stuck watchdog (F-008):** follows the original
  `control::UnstuckBall` strategy (exclude the flippers/plunger, first
  throw upward, eventually relaunch) while using a lighter Q6 position
  detector. A ball whose actual travel stays inside a 3-pixel window for
  about **1.5 seconds** gets a gentle upward/inward nudge. If it remains
  trapped, retry at one-second intervals with up to two small 2-3px
  inward/upward moves **only when the candidate position is clear** of
  indexed walls, radial bumpers and solid sling cores. If neither
  approach works, a last-resort free plunger return preserves the
  remaining-ball count and score. Waiting balls, live shooter travel,
  kickback holds, flipper capture areas and normal lower drains are
  exempt; physics is frozen in menus/pauses as before. Sustained travel
  away from the trapped position cancels escalation. Moving normally
  incurs only a handful of integer comparisons per physics step;
  geometry checks run only during rare recovery attempts.
  `CADET /record` logs `unstick_nudges`, `unstick_moves` and
  `unstick_relaunches` in `INPUT.TXT`. Timing and perceived behavior
  on the real Tandy/V20 still need verification.
* Short synthesized Tandy PSG tones replace original samples and music.
* Whole-table view prioritizes memory and frame cost over fine visual detail.

## Tests and performance

`TEST.EXE` runs the same core using actual 16-bit arithmetic: progressive
plunger pullback, weak and full launch, bounded variation, wall reflection,
bumper scoring, shooter-lane return versus regular drain, left/right flippers,
contact-dependent flipper leverage and incoming-momentum response,
active flipper tip forgiveness with no idle/tilt saves, more energetic
playfield speed, three-direction nudges and return impulses, tilt, both timed
outlane kickbacks and single-use gating, solid slingshot center
recognition/anti-trapping, **universal stuck-ball detection, upward
release, verified local relocation, free same-ball relaunch, and
negative checks for shooter/flipper/normal drain**, drain/restart
and a 20,000-step soak. It also tests middle-button plunger
mapping and the top-five scores' ordering, persistence and corrupted-file
handling. It writes `TEST.OK` on success.
`CADET /smoke` runs 360 timed frames, writes `SCREEN.BIN` (32768 bytes
in Tandy mode, 64000 in VGA mode) and `SMOKE.TXT`, and restores DOS.
The CI runner tests Tandy auto-detection, forced VGA, and VGA auto-detection
in DOSBox-X, and verifies that the **entire final video frame matches
pixel-for-pixel** between backends. It does not prove physical latency.
`TEST /original` additionally checks all seven bumpers, both kickbacks at
three entry offsets, original plunger frame positions, 12 partial-charge
shooter-return trajectories, successful recharging/relaunching of a returned
ball, and full-power scoring shots. Tilted flippers
retain passive resting collisions. The deterministic smoke test must complete
360 frames / 1440 steps and a fully charged launch; with `--original`, the
runner also requires a bumper hit and at least 500 points. Python tests reject
incomplete, uncharged and zero-score smoke reports.
`CADET /record` uses real keyboard input and writes `INPUT.TXT` on Esc,
including observed control bits, launches, nudges, kickbacks, tilts,
top/side safety contacts (`boundary_hits`), source outer-rail recoveries
(`left_rail_saves` / `right_rail_saves`), and slow-frame count.
It also writes **`FLIGHT.TXT`**, a rolling trace of the **last 256 rendered
frames (about 14 seconds)**: Q6 ball position and velocity, remaining balls,
READY/shooter status, charge, event code and both separate cumulative
`BORDER`, `LEFT_RAIL`, and `RIGHT_RAIL` recovery counters
(flight schema 4).
Flight data is buffered in memory,
**not written to disk during gameplay**, so normal play is unaffected. If the
ball disappears near the upper-left chute, press **Esc immediately** and
inspect/share `FLIGHT.TXT` and `INPUT.TXT`. These show whether it exited the
screen, drained, or remained inside an unexpected collision area.

The small memory model keeps code and near data individually below 64KB;
the immutable background is one far allocation of 32KB (Tandy) or
64000 bytes (VGA). Only changed ball and flipper rectangles are restored. Collision candidates use 25 narrower 8-pixel vertical bands and
precomputed per-edge bounds before fixed-point dot products. Moving sprites
are drawn scanline-by-scanline without per-pixel video address division;
stationary flippers are not repainted unnecessarily. No per-frame allocation,
floating point, full-screen blit, or full-table collision scan is used.
Build maps and executable headers are checked by `verify_build.py`.
`test_source_layout.py` verifies the ordered implementation-fragment
layout and `table.c` build contract; `test_menu_font.py` checks the
relocated `hud.inc` and `input.inc` routines. The
[successful post-refactor CI run](https://github.com/ARMoir/SpaceCadetPinball/actions/runs/42422864888)
passed the source-layout checks, real-mode build, Tandy/VGA smoke and
final-frame parity, **not** physical V20 timing.

See [`VALIDATION.md`](VALIDATION.md) for measured emulator results and
remaining hardware checks, and [`FEATURE_STATUS.md`](FEATURE_STATUS.md)
for the complete inventory of missing original Space Cadet features.

## Format and source references

`convert.py` follows `SpaceCadetPinball/partman.h`, `partman.cpp`, `gdrv.cpp`,
`GroupData.cpp`, `loader.cpp`, `proj.cpp`, `TFlipperEdge.cpp`, and
`TEdgeSegment.cpp`. Generated packs are little-endian SCP2: 4-byte magic,
four signed shorts (edge count, launch X/Y, drain Y), 12 flipper coordinate
shorts, four kickback center shorts (left X/Y, right X/Y), nine shorts per
edge, then five ball/flipper sprites followed by eight plunger sprites. Each sprite has X/Y/W/H
shorts and W*H bytes (0..15 or 255 for transparent). Caps are enforced by
both converter and runtime. Coordinates are pixels; tangents Q8 and lengths Q6.

Tandy layout reference: Tandy 1000 Technical Reference Manual, video section
(four 8KB banks, mode 09h). Standard VGA reference: BIOS INT 10h modes 13h
and AX=1A00h detection; A000h linear 64000-byte video, VGA DAC 3C8/3C9h.
Compiler: https://github.com/open-watcom/open-watcom-v2

## Runtime-only downloads and published builds (2026-11-08)

The Actions **CADET** artifact downloads as **CADET.zip** with exactly seven
files at its root: `CADET.EXE`, `TABLE.DAT`, `TABLE.TGA`, `TABLE3D.TGA`,
`TABLESRC.TGA`, `VGA.DAT`, and `MENU.DAT`. The enhanced image preserves
the default Tandy source-colored appearance; `/classic` selects the unmodified RGBI baseline
and `/3d` selects the earlier shading. Extract all seven
together and run CADET from that
folder. Keep an existing `SCORES.DAT`: scores are created locally and never
bundled. No emulator, test program, Python, maps or logs are needed to play.

Successful pushes/manual runs on `dos/tandy-native-prototype` also publish
a uniquely tagged GitHub prerelease containing `CADET.zip`. Publishing waits
for the native core, Tandy/VGA auto/forced smoke and video-parity checks.
Pull requests and other branches do not publish releases. Test evidence is
available separately as **CADET-VALIDATION**; failed runs expose diagnostics
without publishing a playable package. Physical V20 timing remains unverified.

## Improved VGA artwork (2026-11-08)

VGA now automatically uses `VGA.DAT` for richer original-source colors,
smoother metal shading and colored ball/flipper/plunger sprites at 320x200.
The HUD retains its existing readable colors. This adds 768 bytes to the
VGA graphics allocation; Tandy keeps its banked 32KB background and original
16-color assets. Physics, layout and collision positions are unchanged.

`CADET /vga /rgbi` selects the earlier 16-color look; a missing VGA.DAT also
falls back to it. `/classic` and `/3d` affect only the Tandy/RGBI background,
while source-color VGA always uses the newly converted original artwork.
To regenerate: `python Platform/dos/convert.py /path/to/PINBALL.DAT --vga
--output Platform/dos/data` (on one line). VGA1 contains four magic bytes,
768 six-bit DAC bytes, 64000 pixel indices, and thirteen SCP2-compatible
sprite records. No extra runtime dependencies or source archive are needed.

## Ball-position guards (2026-11-08)

Static walls now reject center crossings between physics samples, including
fast diagonal hits, instead of pushing a penetrating ball through to the
opposite side. Existing endpoint openings and the upper one-way passage
remain valid. Position-changing paddle contacts recheck sling and outer-rail
containment. `/record` and `/smoke` report `wall_crossings` separately.

The tests now include eight varied-control games (80,000 additional physics
steps), checking rail boundaries and solid bumper/sling contacts after each
step. Normal drains, weak-shot returns and both kickbacks remain tested.
This improves guards for the current 2D model; omitted source ramps, layers
and state-dependent gates remain incomplete. Hardware timing is unverified.

### CMP1 source components (2026-11-08)

Regenerate TABLE.DAT and IMPORT.json with `convert.py` and the original
Windows PINBALL.DAT. The SCP2 prefix/sprites stay byte-identical; a CMP1
extension adds two triangle-derived ramp paths and 18 directional lines.
Use the matching CADET.EXE and TABLE.DAT from one tested CADET.zip. Legacy
packs without the extension load without these components; old executables
reject new packs. All Tandy/VGA artwork remains unchanged.

CMP1 encoding is little-endian signed 16-bit words: signature, ramp count,
directional-line count; each ramp has group/count/total-Q6-length/assist,
then count records of x/y/ex/ey (Q6), tangent x/y (Q8), length/start (Q6).
Each directional record has group/mask/solid, x/y/ex/ey (Q6), normal x/y
(Q8), length (Q6). Bounds are derived once at load; no floats or allocations
are added to physics. Counts are bounded by game.h; truncated, unknown and
trailing extension data fail startup.

This is partial source fidelity, not full TRamp forces/height or TGate
rules. See F-001/F-002 and VALIDATION.md, including shooter/elevated-mask
exemptions and pending physical hardware acceptance.

### CMP2 spent-kicker entrances (2026-11-09)

CMP2 retains CMP1's ramp and one-way records and appends exactly two
little-endian 9-short Edge records for original TGate 501/502 (left/right).
Their x/y/ex/ey are pixels, tangent is Q8, length is Q6, score/boost are zero.
TABLE.DAT grows 36 bytes to 12148. The new executable reads CMP1, CMP2 and
legacy SCP2; older executables reject CMP2, so use all files from one release.
Closed gates collide and render at these actual entrance lines, replacing
legacy stand-ins. `/record` adds `spent_gate_blocks`. Score and ball/player
digits have also been corrected to upright orientation offline.

### V20 renderer micro-optimizations (2026-11-12)### Source-faithful Tandy image, using WolfensteinCGA color-pair ideas (F-007)

`CADET /tandy` (or `/sourceart`) loads `TABLESRC.TGA`, a third **optional**
prepacked 32768-byte table image. Its source is the existing `VGA.DAT`
built from the original Windows `PINBALL.DAT`. Unlike `/3d`, the new
generator starts from the higher-color *source projection* itself.
It evaluates pairs of actual Tandy RGBI colors and alternates two colors
on a fixed 2x2 pattern where the local source is sufficiently uniform.
This can make the average CRT appearance of midtones resemble the
Windows artwork more closely. On one-pixel rails, edges, high-contrast
decals, and colored lamps it chooses a solid color rather than producing
a jagged dither. The original backdrop geometry, HUD region and video
coordinates are not rescaled or blurred.

This is inspired by the palette-pair search in
[jhhoward/WolfensteinCGA](https://github.com/jhhoward/WolfensteinCGA),
not its raycaster or generated code. No Wolfenstein assets or source
files are copied. **There is still an offline color conversion:** the
Tandy does not gain extra colors, and a single pixel remains one of its
16 RGBI colors. `/sourceart` is now the Tandy default, while `/classic` still loads
the earlier 16-color conversion unchanged. Only the static background
changes. No additional DOS framebuffer, executable decoder, CPU cycles
per frame, RAM, sprites, collisions, physics or scoring are introduced.
The optional asset costs 32 KB of disk/CF space in the ZIP.

The build/test workflow checks file packing, palette bounds, large flat
midtone pairs, fine one-pixel lines, deterministic generation, source
pack/geometry integrity, two DOSBox-X video modes, 64,000-pixel Tandy/VGA
frame parity and unchanged game counters. **Which look is clearest on the
physical IBM/CRT display remains to be tested.** Compare:
`CADET /tandy /classic`, `CADET /tandy /3d`, and
`CADET /tandy /sourceart`.

### Sharper original-color-guided Tandy playfield (F-007)

The default `TABLE3D.TGA` now gains a **build-time** contrast refinement
guided by the higher-color source pixels in `VGA.DAT`. Only existing
gray metal (rails, bumpers and playfield hardware) can change, by one
RGBI gray/white shade per pixel, with deliberate protection for lamps,
colored labels and the HUD. Small one-pixel gaps in connected white-metal
rims can be joined. The image is still 320x200, 16 colors, 32768 bytes,
with no blur, upscale, moving-object changes, collision changes, memory
increase or runtime CPU cost. This is not a new video mode.

The build command is `python Platform/dos/enhance_table.py --table
build/dos/TABLE.TGA --pack build/dos/TABLE.DAT --vga-source
build/dos/VGA.DAT --output build/dos/TABLE3D.TGA`.
Compare `CADET /tandy /classic` with `CADET /tandy /3d` on the
actual Tandy. Original classic art and VGA1/source-color VGA artwork are
not modified. Physical CRT improvement is not yet verified.



Inspired by the *techniques* in jhhoward/WolfensteinCGA, not by its
raycaster or copyrighted game content. These optimizations affect only the
native DOS display backend; gameplay, ball geometry, physics, timing,
score values, source assets and user-visible images are intentionally
unchanged:

- `video.inc` computes a single 200-entry table of 16-bit scanline offsets
  once at startup. Both VGA 13h and four-bank Tandy 09h sprite and
  dirty-rectangle paths use it instead of repeatedly multiplying by the
  video stride and calculating Tandy bank selection.
- Tandy sprite compositing now handles two aligned, 4-bit pixels per VRAM
  byte. Opaque pairs are written in one store without a VRAM read;
  transparent and partially transparent pairs preserve the other nibble.
  Odd X coordinates and clipped edges use masked single-nibble writes.
  The VGA sprite path is unchanged aside from its row-offset lookup.
- Source-style score digits iterate their original 5x8 bitmap by row and
  column rather than dividing/modulo by five for each pixel.

The existing build already uses preconverted table/sprite/menu assets,
fixed-point collision geometry, selective redraws, and 2-pixel dirty
rectangles, so these were not duplicated. Wolfenstein's ray-per-column
tracing and runtime generated wall scalers do **not** apply to this
2D, mostly stationary playfield; duplicating them would waste the
Tandy's limited 16-bit memory budget.

`test_video_blitter.py` models the former per-pixel renderer versus the
packed-pair renderer over transparent, opaque, mixed, odd-width, clipped
and randomized rows, and validates lookup limits. CI also builds the
Open Watcom 8086 executables, runs actual DOSBox-X Tandy and VGA game
smoke tests, and requires pixel-perfect RGBI frame parity. These tests
verify output correctness; they **do not** establish a speedup on a physical
7.16 MHz V20.

For a hardware A/B comparison, keep the last known-good 298f33a build
and a new tested build in separate directories (each with its own matching
TABLE.DAT). Compare like-for-like gameplay at the same monitor settings
with `CADET /tandy`. Record responsiveness, missed updates, and optionally
run `CADET /tandy /record` to inspect the cumulative late-frame count in
INPUT.TXT. Treat the count as coarse: it includes gameplay and display
cost and is not a CPU-cycle benchmark.

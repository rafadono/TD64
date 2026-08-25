# TD64 host-native unit tests

Most of TD64 is tightly coupled to N64 hardware (`rdpq`, `joypad`, the
Controller Pak's Joybus I/O) and can only really be exercised on the Ares
emulator or real hardware — that's the existing manual test loop, and it's
not what lives here.

What lives here are unit tests for the handful of modules that are pure
logic with no rendering/hardware dependency: scoring formulas, pathfinding
math, camera scroll clamping, and button-binding logic. These compile with
a **plain host `gcc`**, not the N64 `mips64-elf` cross-compiler, and run in
milliseconds — no Docker toolchain bootstrap, no emulator.

## Why this works without the real SDK

The production `.c` files under test (`score.c`, `pathfinding.c`,
`effects.c`, `controls.c`) all transitively `#include <libdragon.h>` via
`src/core/engine.h`. `tests/fakes/libdragon.h` is a small stand-in for that
header — just enough types (`color_t`, `sprite_t`, `joypad_buttons_t`) and
no-op stubs for the few `rdpq_*` calls those specific files happen to make,
so the real, unmodified production code compiles on a host machine. It is
never used by the real N64 build (the makefile doesn't reference `tests/`
at all) and isn't a general libdragon replacement — it only covers what
these four files need.

`tests/fakes/save_stub.c` similarly stands in for `save.c`'s Controller Pak
I/O for `test_controls.c` only, so tests can dictate "pak present/absent/
valid config" scenarios directly instead of needing real hardware.

## Running

Needs a host `gcc` (or any C11 host compiler via `CC=...`). The same
`ghcr.io/dragonminded/libdragon` Docker image already used to build the ROM
happens to ship one, so this works with no extra setup:

```bash
docker run --rm -v "$(pwd):/src" -w /src ghcr.io/dragonminded/libdragon:latest bash tests/run_tests.sh
```

Or directly, if you have a host gcc/clang available:

```bash
bash tests/run_tests.sh
```

## What's covered

- **`test_score.c`** — combo scaling/cap/timeout, wave-clear/perfect/speed/
  time-penalty formulas, the map-complete difficulty multiplier (which
  rescales the *whole* accumulated score, not just its own bonus — easy to
  miss), campaign-complete and final-score bonuses, and the `RunLog`
  highlights timeline (which events get logged, the 48-event cap).
- **`test_pathfinding.c`** — `path_scale`, and `path_follow`'s
  multi-waypoint-per-frame advancement and diagonal/axis-aligned speed
  parity (a regression guard for the exact "diagonal movement runs
  sqrt(2)x faster" bug class flagged and fixed elsewhere this session).
  Also `path_init_from_terrain`'s free-form corridor tracer, including a
  real limitation found while writing these tests: the trace buffer caps
  at `MAX_WAYPOINTS - 2` (14) cells, so a corridor longer than that (e.g. a
  full-width straight line across the 20-cell-wide custom map grid) fails
  to trace and silently falls back to a preset path.
- **`test_camera.c`** — `camera_ensure_visible`'s margin clamping and
  map-bounds re-clamping (the function that drives all camera scrolling),
  `camera_apply`'s screen-space offset, and `camera_shake`/`camera_update`'s
  intensity decay and "don't let a weaker shake cut a stronger one short"
  rule.
- **`test_controls.c`** — `button_matches` for every physical button,
  binding get/set bounds-checking, the "C-up can never be a default
  binding" design invariant, and `controls_init`'s "defaults, only
  overridden by a *valid* saved Controller Pak config" contract.

## What's deliberately NOT covered here

`entities.c` (combat/AI), `game.c` (wave composition/update loop),
`campaign.c`, `save.c`'s real Controller Pak I/O, and anything UI/rendering
— these are either genuinely hardware-dependent or coupled enough to the
full `GameState`/entity pool that a meaningful host test would need to fake
most of the engine, at which point it stops testing anything real. They
stay covered by manual testing on Ares/hardware.

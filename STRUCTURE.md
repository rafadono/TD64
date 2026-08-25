# Complete Project Structure

## Directory Tree

```
faction_td/
├── src/                    # Source code organized by category
│   ├── core/              # Base engine
│   ├── config/            # Configuration and data
│   ├── entities/          # Units and projectiles
│   ├── world/              # Maps and terrain
│   ├── game/               # Game logic
│   ├── systems/            # Auxiliary systems (includes save/Controller Pak, localization)
│   ├── ui/                 # User interface (includes the map editor)
│   └── resources/          # Asset loading
│
├── assets/                # Original assets
│   ├── sprites/           # Sprite PNGs, terrain tiles and reference maps
│   └── audio/             # Audio files (WAV for BGM and SFX) — empty, no audio yet
│
├── filesystem/           # Converted assets (auto-generated)
├── build/                # Compiled binaries (auto-generated)
├── tools/                # Asset generation scripts
├── docs/                 # Additional documentation
│
├── makefile              # Build system
├── README.md             # Main documentation
├── STRUCTURE.md           # This file
├── prompts_graficos.md    # Prompt guide for AI image generators (optional use)
└── .gitignore             # Git ignore rules
```

---

## Complete File Listing

### `src/core/` - Base Engine (3 files)

| File | Purpose |
|---------|-----------|
| **screen.h** | `SCREEN_WIDTH/HEIGHT` and `WORLD_WIDTH/HEIGHT` — the single source of truth for the game's resolution and the biggest scrollable map size. Dependency-free so world/terrain.h and systems/save.h can derive from it without a circular include |
| **engine.h** | Main engine header. Includes every subsystem and defines global types (Entity, GameState, GameFlowState — 11 states, including STATE_MAP_EDITOR, STATE_CUSTOM_MAP_MENU, STATE_CONTROLS_MENU, and STATE_STATS_MENU) |
| **main.c** | Main game loop. Input via `joypad_*` (routed through the remappable-action layer, see systems/controls.h), real delta time, render/update dispatch by state, D-pad cursor movement with camera auto-scroll |

### `src/config/` - Configuration and Data (4 files)

| File | Purpose |
|---------|-----------|
| **game_config.h** | Master balance file for the game. Economy, waves, maps, score, debug defaults |
| **factions.h** | Faction enums (DAWNGUARD, IRONBONE, ASHCLAW, VEILSTORM) and unit types (SCOUT, WARRIOR, etc) |
| **units_data.h** | UnitStats struct with every field of a unit (HP, damage, cost, etc) |
| **units_data.c** | Full stat table for the 24 units (4 factions x 6 types), each with its own active ability |

### `src/entities/` - Units and Projectiles (6 files)

| File | Purpose |
|---------|-----------|
| **entities.h** | Entity types, spawn and update functions |
| **entities.c** | Towers, runners, projectiles. Combat, movement, the 24 unique special abilities, rendering in color-grouped passes (health bars/dots/debug) |
| **animation.h** | Sprite-sheet animation system (AnimDef with `start_frame`/`frame_width` for sub-rects) |
| **animation.c** | Animator implementation |
| **collision.h** | AABB, collision detection, distance calculations |
| **collision.c** | Basic physics implementation |

### `src/world/` - World and Terrain (6 files)

| File | Purpose |
|---------|-----------|
| **terrain.h** | Terrain types, grid (sized `WORLD_WIDTH x WORLD_HEIGHT`, big enough for the largest map), stat modifiers |
| **terrain.c** | `terrain_compose()` composes a map's valid area once (on load/edit) into a `WORLD`-sized offscreen surface using real textures; `terrain_render(cam_x, cam_y)` blits a `SCREEN`-sized window of that surface starting at the camera's world position, one draw call per frame regardless of map size |
| **maps.h** | MapData struct (includes `width`/`height`, the map's actual play-area size), map loading functions (fixed and custom) |
| **maps.c** | Definition of the 4 fixed maps (Greenfield, Desert, Frozen, Volcanic) — each `WORLD`-sized, tiling their terrain pattern into 4 quadrants and scaling their path 2x/2x — + `map_load_custom()` for saved/edited maps (always `SCREEN`-sized) |
| **pathfinding.h** | Waypoints, Path, path-following functions |
| **pathfinding.c** | Pathfinding algorithms (curve, zigzag, spiral, straight) plus `path_init_from_terrain()` (traces a free-form path painted with `TERRAIN_PATH` tiles into real waypoints) and `path_scale()` (stretches a preset path onto a bigger canvas); `path_follow()` consumes distance per frame instead of moving at a fixed speed (prevents a fast enemy from skipping a waypoint) |

### `src/game/` - Game Logic (4 files)

| File | Purpose |
|---------|-----------|
| **game.h** | Match logic prototypes, includes `game_start_custom_map()` |
| **game.c** | Game loop, staggered wave spawning (`game_process_spawns`), economy, victory/defeat, custom map startup |
| **campaign.h** | Campaign struct, campaign functions, `GameDifficulty` enum (Easy/Normal/Hard/Extreme) and its accessors |
| **campaign.c** | 4 campaigns (one per faction), rival selection, map progression, saves the best score to the Controller Pak on campaign completion; also implements the difficulty scale and its unlock rules (Hard needs 1 completed campaign, Extreme needs all 4) |

### `src/systems/` - Auxiliary Systems (16 files)

| File | Purpose |
|---------|-----------|
| **score.h** | ScoreSystem struct (includes `run_elapsed` and a `RunLog`), scoring prototypes, `RunEventType`/`RunEvent`/`RunLog` and `run_log_add()`/`run_log_clear()` |
| **score.c** | Multi-factor score: combo, speed bonus, perfect wave, time penalty — wired to wave/map/campaign complete; also logs a curated run event timeline (map started, perfect/hero-interval waves, hero kills, victory/defeat) into `ScoreSystem.log` for the Stats screen's "highlights" page |
| **leveling.h** | UnitLevel struct, XP functions |
| **leveling.c** | Tower level-up system (gain XP from kills), capped at level 10 |
| **effects.h** | Camera (persistent world-space scroll + transient shake), particles, floating text |
| **effects.c** | `camera_ensure_visible()` (auto-scrolls to keep a world point on screen with a margin — drives all camera movement), particle/floating-text implementation with real damage numbers |
| **debug.h** | DebugState struct with every overlay/cheat flag, plus `debug_track_gold_earned()`/`debug_track_damage_dealt()` for the Economy overlay's real-time rates |
| **debug.c** | Unified debug menu: one button (C-up) opens a navigable list of all 18 overlays/cheats, replacing the old scattered C-button toggles and hidden L+R+C-* cheat combos. `gold_per_second`/`dps_total` are rolled up once/sec from accumulators fed by entities.c (kills, hits) and game.c (wave-clear bonus) — no longer stuck at 0 |
| **audio.h** | Modular audio system header |
| **audio.c** | Background music (BGM) and sound effect (SFX) playback/init via wav64 and libdragon's mixer — no audio assets loaded yet |
| **save.h** | Serialized formats for custom maps (`CustomMapSave`), progress (`GameProgress` — best score, campaigns completed, total kills, fastest clear time per campaign), and remapped controls (`InputConfig`); `SaveStatus` enum |
| **save.c** | Controller Pak layer: scans all 4 controller ports for a pak, reading/writing "notes" (mempak's high-level API, never raw sector access), formatting only under explicit confirmation |
| **lang.h** | Language/StringId enums and the `T(StringId)` localization accessor |
| **lang.c** | English/Spanish string table (`STRINGS[STR_COUNT][LANG_COUNT]`), runtime language cycling |
| **controls.h** | `PhysicalButton`/`GameAction` enums, the 7 remappable in-game actions and their default bindings |
| **controls.c** | Binding storage, `action_pressed()` (checks whichever physical button is currently bound to an action), load from/save to the Controller Pak |

### `src/ui/` - User Interface (12 files)

| File | Purpose |
|---------|-----------|
| **menu.h** | Menu prototypes |
| **menu.c** | Main menu (6 options: play campaign / custom maps / controls / stats / language / credits), the Easy/Normal/Hard/Extreme difficulty select screen, faction select (also reused to start custom maps), pause, game over/victory |
| **ui.h** | HUD prototypes |
| **ui.c** | In-game HUD: health bar, gold, build panel, tooltips |
| **map_editor.h** | Map editor prototypes |
| **map_editor.c** | Editor: paint terrain (including free-form path tiles) on the grid, pick path shape / enemy faction / difficulty / starting gold / starting lives (cycled via Z, adjusted via C-up/C-down), name the map via an on-screen keyboard, clear the grid (hold L+R+B), save to the Controller Pak or play without saving |
| **custom_map_menu.h** | Custom map menu prototypes |
| **custom_map_menu.c** | Lists the 8 Controller Pak slots with their saved name (play/edit/delete), handles pak formatting with confirmation |
| **controls_menu.h** | Controls remap screen prototypes |
| **controls_menu.c** | Lists the 7 remappable actions; A captures the next button press as a new binding, Z resets to defaults, Start saves to the Controller Pak |
| **stats_menu.h** | Stats screen prototypes |
| **stats_menu.c** | Two pages (Z switches): lifetime Progress (total kills, best score/fastest clear per campaign, from the Controller Pak) and the current run's Highlights (`ScoreSystem.log`, D-pad scrolls) |

### `src/resources/` - Asset Loading (2 files)

| File | Purpose |
|---------|-----------|
| **resources.h** | Sprite loading prototypes |
| **resources.c** | Loads the 28 unit/projectile sprites (8-frame animated sheets) + 5 terrain textures from ROM |

---

## Assets

### `assets/sprites/` - Sprites and Textures (generated by `tools/`)

**24 animated unit sheets** (4 factions x 6 types), 256x32px each (8 frames of 32x32: idle + 4 walk + 3 attack):
```
dawnguard_scout.png      ironbone_scout.png
dawnguard_warrior.png    ironbone_warrior.png
dawnguard_archer.png     ironbone_archer.png
dawnguard_mage.png       ironbone_mage.png
dawnguard_tank.png       ironbone_tank.png
dawnguard_hero.png       ironbone_hero.png

ashclaw_scout.png        veilstorm_scout.png
ashclaw_warrior.png      veilstorm_warrior.png
ashclaw_archer.png       veilstorm_archer.png
ashclaw_mage.png         veilstorm_mage.png
ashclaw_tank.png         veilstorm_tank.png
ashclaw_hero.png         veilstorm_hero.png
```

**4 projectiles** (16x16px, not animated):
```
dawnguard_projectile.png
ironbone_projectile.png
ashclaw_projectile.png
veilstorm_projectile.png
```

**7 terrain textures** (16x16px, used at runtime by `terrain_compose()`):
```
tile_grass.png    tile_water.png    tile_mountain.png
tile_desert.png   tile_snow.png     tile_lava.png     tile_path.png
```
(`tile_lava` maps to `TERRAIN_LAVA` — a hazard that heavily slows enemies,
used for Volcanic Pass's lava rivers. `tile_path` maps to `TERRAIN_PATH`,
the road tiles a custom map's free-form path is painted with in the map
editor; towers can't be placed on it.)

**4 reference maps** (320x240px, generated but NOT used at runtime — see README):
```
map_greenfield.png   map_desert.png   map_frozen.png   map_volcanic.png
```

**1 reference sprite sheet** (overview of all 24 units in idle pose + projectiles):
```
sprite_sheet.png
```

### `assets/sprites/previews/` - 4x Previews

A `*_preview.png` copy of each file above, scaled 4x with *nearest* filtering
so it looks better in a regular image viewer. Not used by the game or the build.

---

## Tools

### `tools/gen_sprites.py`

Generates the 24 animated unit sheets + 4 projectiles + the reference sheet.
Each `draw_<type>(palette, faction, frame)` function draws the base
silhouette and shifts the legs/weapon based on the frame (`anim_offsets()`)
to simulate the walk/attack cycle without needing 8 hand-drawn poses per unit.

```bash
cd tools
python gen_sprites.py
```

### `tools/gen_maps.py`

Generates the 7 terrain textures (16x16) and the 4 full-map reference images
(320x240, with the path already drawn — not used at runtime, see README).

```bash
cd tools
python gen_maps.py
```

### `tools/downsample_images.py`

Utility for bringing in external art (e.g. AI-generated) while respecting
N64's constraints: strips the black background, resizes to the expected
dimensions (`unit`/`sheet`/`projectile`/`background`), and quantizes to
RGBA16. Not used for any active game asset today — all art in use is 100%
procedural.

```bash
python tools/downsample_images.py <image> <unit|sheet|projectile|background> [output_name]
```

---

## Build Artifacts (auto-generated, not version-controlled)

### `filesystem/` - Converted Assets

`.sprite` files converted from each PNG in `assets/sprites/` by `mksprite`
(a generic makefile rule — no need to list each one).

### `build/` - Compiled Binaries

```
build/
├── TD64.z64            # Final ROM for emulator/hardware
├── TD64.elf            # ELF for debugging
├── TD64.dfs            # Packed filesystem
└── *.o                 # Intermediate object files
```

---

## Documentation

| File | Content |
|---------|-----------|
| **README.md** | Main documentation: features, controls, custom maps, Controller Pak, procedural sprites/terrain, score, economy |
| **STRUCTURE.md** | This file - complete structure guide |
| **FILE_LIST.txt** | Full repo file tree with a one-line purpose per file/group |
| **TODO.md** | Roadmap grouped by area, each item grounded in a real gap found while building/reviewing the codebase |
| **prompts_graficos.md** | Prompt templates for AI image generators (optional — the game's active art doesn't depend on this) |

---

## Configuration Files

| File | Purpose |
|---------|-----------|
| **makefile** | Build system with targets: `make`, `make clean`, `make gen-sprites`, `make help` |
| **.gitignore** | Ignores `build/`, `filesystem/`, `*.sprite`, `*.z64`, `__pycache__`, etc — `libdragon/` is no longer listed here, it's a tracked git submodule now (see `.gitmodules`) |
| **.gitmodules** | Declares `libdragon` as a real git submodule pinned to a specific upstream commit, instead of a plain gitignored checkout re-cloned by CI on every run |

---

## Project Statistics

```
Total source files:       53 files (.c + .h)
  - Headers (.h):          28
  - Implementation (.c):   25

Total sprites:             35 generated PNGs (28 units/projectiles + 7 terrain tiles)
                            + 4 reference maps + 1 reference sheet
Factions:                  4
Units per faction:         6 (all 24 with a unique active ability)
Fixed maps:                4 (each WORLD_WIDTH x WORLD_HEIGHT = 4x one screen)
Custom map slots:          8 (Controller Pak)
Campaigns:                 4
Remappable actions:        7 (Controller Pak)
Debug overlays/cheats:     18, in one menu
Persisted progress:        best score, campaigns completed, total kills,
                            fastest clear time — per campaign (Controller Pak)
Run event log:             up to 48 curated events/run, RAM-only (Stats screen)
```

---

## Development Workflow

### 1. Edit Configuration
```
Change balance -> src/config/game_config.h
Tune stats     -> src/config/units_data.c
```

### 2. Modify Sprites or Terrain
```
Option A: Edit the PNG in assets/sprites/ directly
Option B: Regenerate with tools/gen_sprites.py (units) or tools/gen_maps.py (terrain)
Option C: Bring in external art with tools/downsample_images.py
```

### 3. Build
```bash
make              # Converts sprites + compiles + packs the ROM
```

### 4. Test
```bash
Ares (recommended) or another N64 emulator, or real hardware with a flash cart
```

### 5. Debug
```
In game: C-up opens/closes the debug menu
Inside it: D-pad selects an overlay or cheat, A toggles it, B closes
```

### 6. Custom Maps
```
Main menu -> CUSTOM MAPS -> pick a slot -> Start to edit
In the editor: A paints, L/R cycles terrain, Z cycles setting, C-up/down adjusts it, Start saves
```

### 7. Remap Controls
```
Main menu -> CONTROLS -> D-pad to select an action -> A to capture the next button press
Z resets everything to defaults, Start saves to the Controller Pak, B exits
```

### 8. View Stats
```
Main menu -> STATS -> Z switches between Progress and Highlights pages
D-pad scrolls the Highlights list, B exits
```

---

## Files Critical to Balancing

If you only want to tune the game without touching code:

1. **`src/config/game_config.h`** - Changes waves, economy, score, difficulty
2. **`src/config/units_data.c`** - Changes HP, damage, cost for each unit

These two files control 90% of the game's balance.

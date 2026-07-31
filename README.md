# Faction Wars Tower Defense - N64

Faction-based tower defense for the Nintendo 64, written in C with libdragon.

> **Status: Initial release.** This is the first published version of TD64.
> It builds cleanly and has been tested on the Ares emulator, but it hasn't
> gone through extensive playtesting or real-hardware testing yet (no
> flashcart available during development) — expect rough edges and possible
> bugs. Issues and pull requests are welcome.

## Table of Contents

- [Features](#features)
- [Controls](#controls)
- [Custom Maps and Map Editor](#custom-maps-and-map-editor)
- [Controller Pak Saving](#controller-pak-saving)
- [Animated Sprites and Textured Terrain](#animated-sprites-and-textured-terrain)
- [Language](#language)
- [Debug System](#debug-system)
- [Game Configuration](#game-configuration)
- [Score System](#score-system)
- [Economy System](#economy-system)
- [Audio System](#audio-system)
- [Special Abilities](#special-abilities)
- [Balancing](#balancing)
- [Asset Generation Tools](#asset-generation-tools)
- [Building](#building)

---

## Features

### 4 Playable Factions

| Faction | Style | Rival |
|---------|--------|-------|
| Dawnguard | Holy knights, balanced | Ironbone |
| Ironbone | Undead, draining tanks | Dawnguard |
| Ashclaw | Savages, maximum damage and speed | Veilstorm |
| Veilstorm | Arcane mages, extreme range | Ashclaw |

### 6 Unit Types per Faction

- Scout - Fast, cheap, fragile
- Warrior - Balanced, frontline
- Archer - Medium range, good DPS
- Mage - AoE, long range, slow cooldown
- Tank - Very high HP, slow, expensive
- Hero - Unique boss, appears every 5 waves

### 4 Maps with Tactical Terrain

- Greenfield - Tutorial (grass, easy)
- Desert Crossing - Medium (oasis and dunes)
- Frozen Highlands - Hard (snowy mountains)
- Volcanic Pass - Extreme (lava and rock)

### Integrated Systems

- Faction selection with a stat preview
- Multi-factor score (speed, combo, perfect wave), wired end-to-end:
  bonuses for wave, map complete, and campaign complete
- Balanced economy (cost / gold reward)
- Progressive wave system with random events (double gold, mini-boss,
  speed/HP boost) and staggered spawning (units appear one at a time, not
  all at once — no hitches on large waves)
- Tactical terrain (buffs/debuffs by type), rendered with real textures
  (not flat colors) composed once per map
- In-game map editor + Controller Pak persistence for custom maps and
  run progress (see dedicated sections below)
- Animated sprites (8 frames: idle + walk + attack) generated 100%
  procedurally, no external art
- Leveling (towers gain XP from kills, up to level 10)
- Upgrades (3 tiers per tower)
- Visual effects (particles, screen shake, dynamic floating text with real numbers)
- Modular audio system (background music per map and sound effects)
- Unique active special ability for each of the 24 units
- Modern input (libdragon's `joypad_*`) and a full debug system (10+ toggles)
- English/Spanish string-table localization, toggled from the main menu

---

## Controls

### In Game

| Button | Action |
|-------|--------|
| D-pad | Move placement cursor |
| A | Place unit / Confirm |
| B | Cancel selection |
| L / R | Cycle unit type |
| Z | Start next wave |
| Start | Pause |
| C-right | Upgrade the selected tower |

(The C buttons double as debug overlay toggles — see [Debug System](#debug-system).)

### Custom Map Menu

| Button | Action |
|-------|--------|
| D-pad up/down | Select slot (A..H) |
| A | Play the slot's map (if occupied) |
| Start | Edit the slot's map (creates a new one if empty) |
| C-left | Delete the slot's map (asks for confirmation: A=yes, B=no) |
| B | Return to the main menu |
| Hold L+R + A | Format the Controller Pak (only if unformatted — **erases the ENTIRE pak, not just TD64**) |

### Map Editor

| Button | Action |
|-------|--------|
| D-pad | Move the cursor over the grid (20x15 cells) |
| A | Paint the cell with the selected terrain |
| L / R | Cycle terrain type (grass/water/mountain/desert/snow) |
| Z | Cycle which setting C-up/C-down adjusts (path / enemy faction / difficulty / gold / lives) |
| C-up / C-down | Adjust the setting currently selected by Z |
| Start | Save to the Controller Pak and return to the map menu |
| C-right | Play the in-memory map without saving (goes to faction select) |
| B | Exit without saving |

---

## Custom Maps and Map Editor

From the main menu, "CUSTOM MAPS" opens a list of 8 slots (A-H) stored on the
Controller Pak. Each slot can be edited, played, or deleted.

The editor lets you configure a complete standalone map:
- Paint any of the 5 terrain types across the full 20x15 grid.
- Pick one of the 4 already-implemented path types (curve/zigzag/spiral/straight —
  the editor doesn't support drawing free-form waypoints; that's a known
  limitation of this version).
- Choose the attacking faction (the enemy side).
- Set a difficulty label (1-5, informational).
- Adjust starting gold (100-500) and starting lives (5-40).
- Play the map immediately without saving (useful for testing, or if there's
  no Controller Pak).

A custom map is played as a standalone run, independent of the 4 faction
campaigns: it has no "next map" — waves are played indefinitely until all
lives are lost.

## Controller Pak Saving

TD64 uses player 1's Controller Pak (memory pak) to persist:
- Up to 8 custom maps (one per slot).
- The best score for each of the 4 campaigns (updated automatically on
  completing a campaign).

**Safety rules** (a Controller Pak is storage shared across every game on
the console, not just TD64):
- The game **never formats the pak automatically**. If it detects an
  unformatted pak, it requires holding L+R and pressing A (the same kind of
  unusual combo already used by the debug cheats) before erasing its contents.
- All reads/writes use libdragon's "notes" API (`mempak.h`), never raw
  sector access — so TD64's notes coexist with any other game's notes on
  the same pak.
- With no Controller Pak inserted, the editor and "custom map" mode are
  still playable for that session (via "play without saving") — they just
  don't persist between runs.

## Animated Sprites and Textured Terrain

All of the game's art is **100% code-generated** (Python + Pillow), with no
AI image generator involved in the final result — see
[Asset Generation Tools](#asset-generation-tools).

- Each of the 24 units is an 8-frame sprite sheet (256x32px: 1 idle frame +
  4 walk + 3 attack) generated by shifting the legs/weapon a few pixels over
  the same base silhouette — no need to hand-draw 8 poses per unit.
- Each map's terrain is composed **once** on map load (`terrain_compose()`,
  `src/world/terrain.c`) from real 16x16px textures per terrain type, instead
  of flat colors. The result is stored in an offscreen surface and blitted
  whole in a single draw call per frame — the composition cost isn't repeated
  every frame, only on map load or edit.

## Language

TD64 ships with English and Spanish UI text built into the ROM as a
string table (`src/systems/lang.h` / `lang.c`) — every player-facing string
routes through `T(STR_ID)` instead of being hardcoded. The current language
can be switched at runtime from the main menu's "LANGUAGE" option, which
cycles English -> Spanish -> English without restarting. The build-time
default is English (`LANG_DEFAULT` in `lang.h`). Adding another language
means adding one more column to the `STRINGS[]` table, with no UI code changes.

### Debug Mode

| Combination | Action |
|-------------|--------|
| C-up | Toggle FPS + entity count + memory |
| C-down | Toggle grid + collision + ranges |
| C-left | Toggle AI + pathfinding + wave preview |
| C-right | Toggle performance + economy stats |

### Cheats (hold L+R + C-button)

| Combination | Cheat |
|-------------|-------|
| L+R+C-up | Godmode (infinite lives) |
| L+R+C-down | Infinite gold |
| L+R+C-left | One-hit kills |
| L+R+C-right | Fast forward (2x speed) |

---

## Debug System

### 10+ Visual Debug Options

Toggled with the C buttons:

1. FPS Counter - Frames per second
2. Entity Count - Active towers / enemies / projectiles
3. Memory Stats - Pool usage (128 entities max, 256 particles)
4. Collision Boxes - Visual hitboxes
5. Range Circles - Tower attack range circles
6. Pathfinding Overlay - Enemy path waypoints
7. Wave Preview - Next wave composition
8. Economy Debug - Gold/sec, total DPS
9. AI Targeting Lines - Tower -> enemy lines
10. Performance Timers - Update time, render time (ms)
11. Grid Overlay - Visible terrain grid
12. Damage Numbers - Floating text with real values (on by default)

### Active Cheats Indicator

While a cheat is active, a pulsing red "CHEATS ACTIVE" banner appears in the
bottom-right corner.

### Using Debug Mode

In game_config.h, change the defaults:
```c
#define DEBUG_SHOW_FPS_DEFAULT          1   // 1 = on at startup
#define DEBUG_SHOW_COLLISION_BOXES      1
```

Or toggle at runtime with the C buttons (see table above).

---

## Game Configuration

### `src/config/game_config.h` - The Master File

All of the game's balance is centralized in this file. Changing values here
doesn't require touching code, just recompiling.

### Config Sections

#### 1. Global Economy

```c
#define ECONOMY_STARTING_GOLD_BASE      200     // Starting gold
#define ECONOMY_UPGRADE_COST_MULT       1.5f    // Multiplier per tier
#define ECONOMY_WAVE_CLEAR_BASE_GOLD    50      // Bonus per wave
#define ECONOMY_WAVE_CLEAR_PER_WAVE     20      // +20 per wave number
```

#### 2. Wave System

```c
// Composition: BASE + (wave - START_WAVE) * SCALE
#define WAVE_SCOUT_BASE                 3
#define WAVE_SCOUT_START_WAVE           1
#define WAVE_SCOUT_SCALE                2       // Wave 10 = 3+(10-1)*2 = 21

#define WAVE_HERO_INTERVAL              5       // Boss every 5 waves
```

Progressive scaling:

```c
#define WAVE_ENEMY_HP_SCALE_PER_5       0.25f   // +25% HP every 5 waves
#define WAVE_ENEMY_DMG_SCALE_PER_5      0.15f   // +15% damage
#define WAVE_ENEMY_SPEED_SCALE_PER_5    0.10f   // +10% speed
```

Random events:

```c
#define WAVE_RANDOM_DOUBLE_GOLD         10      // 10% chance x2 gold
#define WAVE_RANDOM_MINI_BOSS           15      // 15% +1 extra hero
#define WAVE_RANDOM_SPEED_BOOST         20      // 20% enemies +30% speed
#define WAVE_RANDOM_HP_BOOST            20      // 20% enemies +50% HP
```

#### 3. Map Configuration

Each map has independent values:

```c
// MAP 0: Greenfield
#define MAP0_STARTING_GOLD              200
#define MAP0_STARTING_LIVES             20
#define MAP0_DIFFICULTY                 1       // 1-5
#define MAP0_PATH_TYPE                  0       // 0=curve, 1=zigzag, 2=spiral, 3=straight
#define MAP0_WAVES_TO_COMPLETE          10
```

#### 4. Score System (multi-factor)

See [Score System](#score-system) below.

#### 5. Difficulty (global multipliers)

```c
#define DIFFICULTY_ENEMY_HP_MULT        1.0f    // x1.2 = +20% HP
#define DIFFICULTY_TOWER_COST_MULT      1.0f    // x1.5 = towers +50% more expensive
```

#### 6. Debug Options

```c
#define DEBUG_SHOW_FPS_DEFAULT          0       // 0=off, 1=on
#define DEBUG_GODMODE                   0
```

---

## Score System

### Multi-Factor Scoring

Score is calculated from multiple factors to reward both skill and efficiency:

#### 1. Base Score per Kill

```c
#define SCORE_KILL_SCOUT                50
#define SCORE_KILL_WARRIOR              100
#define SCORE_KILL_ARCHER               80
#define SCORE_KILL_MAGE                 120
#define SCORE_KILL_TANK                 200
#define SCORE_KILL_HERO                 500
```

#### 2. Combo System

- Timeout: 3 seconds without a kill loses the combo
- Max multiplier: x10
- Each consecutive kill adds +1 to the multiplier

```
Kill 1: 50 x 1  = 50
Kill 2: 50 x 2  = 100
Kill 3: 50 x 3  = 150
...
Kill 10+: 50 x 10 = 500 (max)
```

#### 3. Wave Cleared Bonus

- Base: 400 points
- Scaling: +100 per wave number

```
Wave 1:  400 + 100*1  = 500
Wave 5:  400 + 100*5  = 900
Wave 10: 400 + 100*10 = 1400
```

#### 4. Perfect Wave Bonus

No lives lost during the wave:

```c
#define SCORE_PERFECT_WAVE_BASE         800
#define SCORE_PERFECT_WAVE_PER_WAVE     200

Wave 1 perfect:  800 + 200*1  = 1000
Wave 5 perfect:  800 + 200*5  = 1800
```

#### 5. Speed Bonus

Finishing a wave quickly grants a bonus:

```c
#define SCORE_SPEED_BONUS_THRESHOLD     30.0f   // Target seconds
#define SCORE_SPEED_BONUS_PER_SECOND    20      // Points per second saved
#define SCORE_SPEED_BONUS_MAX           1000    // Cap

// Example:
Finish in 20s, save 10s -> 10 * 20 = 200 bonus
Finish in 10s, save 20s -> 20 * 20 = 400 bonus
```

#### 6. Time Penalty

Taking too long is penalized:

```c
#define SCORE_TIME_PENALTY_THRESHOLD    120.0f  // 2 minutes
#define SCORE_TIME_PENALTY_PER_SECOND   10
#define SCORE_TIME_PENALTY_MAX          500

// Example:
Take 150s -> 30s extra -> -30 * 10 = -300
```

#### 7. Map Complete Bonus

```
Lives remaining x 100
5 lives left -> +500

Gold remaining (if > 500)
700 gold -> +700

Difficulty multiplier
Easy:    x 1.0
Normal:  x 1.5
Hard:    x 2.0
Extreme: x 3.0
```

#### 8. Campaign Complete Bonus

```c
#define SCORE_CAMPAIGN_COMPLETE_BONUS   10000
```

### Final Formula

```
FINAL SCORE = base_score * difficulty_mult
            + perfect_waves * 5000
            + total_kills * 10
            + life_bonus + gold_bonus
            + [campaign_bonus if complete]
```

---

## Economy System

### Cost / Reward Balance

The system is designed so a unit's summon cost is proportional to the gold
it drops on death as an enemy.

#### Balance Rule

```
unit_cost = (gold_reward * 4) + base_overhead

Scout cost = 40  ->  reward = 8   (ratio 5:1)
Tank  cost = 130 ->  reward = 25  (ratio 5.2:1)
```

This guarantees that:
- Killing about 5 enemies of the same type pays for its own tower
- There are no free towers that break the economy
- Upgrades are expensive but worth it

#### Wave Clear Bonus

```c
#define ECONOMY_WAVE_CLEAR_BASE_GOLD    50
#define ECONOMY_WAVE_CLEAR_PER_WAVE     20

Wave 1:  50 + 20 = 70 gold
Wave 5:  50 + 100 = 150 gold
Wave 10: 50 + 200 = 250 gold
```

This bonus offsets the growing cost of defense.

#### Upgrade Economy

```
Upgrade Tier 1: cost * 0.6 * 1.5^0 = 60% of the base price
Upgrade Tier 2: cost * 0.6 * 1.5^1 = 90%
Upgrade Tier 3: cost * 0.6 * 1.5^2 = 135%

// Example: Scout ($40)
Tier 1: $24
Tier 2: $36
Tier 3: $54
Total:  $154 (almost 4x the base price)
```

### Every Cost Is Configurable

In `src/config/units_data.c`, each unit has:

```c
[UNIT_WARRIOR] = {
    .cost         = 65,     // Cost to place
    .gold_reward  = 12,     // Gold on death (as an enemy)
}
```

---

## Audio System

The game has a modular audio system implemented in `src/systems/audio.c`
that uses libdragon's mixer for sound effects (SFX) and background music
(BGM) in WAV64 format.

### Background Music (BGM)
Music loops on mixer channel 0 and changes dynamically based on the current map:
- Greenfield: `grass.wav64`
- Desert Crossing: `desert.wav64`
- Frozen Highlands: `snow.wav64`
- Volcanic Pass: `volcano.wav64`

### Sound Effects (SFX)
Sound effects load from the filesystem and play on dedicated channels:
- Hit (damage): `hit.wav64` on channel 1
- Coin (gold collected): `coin.wav64` on channel 2
- Level Up (level-up or ability activation): `levelup.wav64` on channel 3

---

## Special Abilities

The 24 units (4 factions x 6 roles) each have a unique active ability that
triggers in combat once its specific cooldown elapses, defined in
`src/entities/entities.c`, with their own visual effects (particles and
floating text) and sound.

| Role | Dawnguard | Ironbone | Ashclaw | Veilstorm |
|---|---|---|---|---|
| Scout | Dash — instant double hit | Phase — x3 surprise strike | Berserk — x3 frenzied strike | Blink — x2 hit + slow |
| Warrior | Holy Shield — x2 hit + 1 life | Life Drain — x1.5 damage + gold | Slam — slows + x2 damage | Chain Strike — chains to nearby enemies |
| Archer | Volley — hits the target and nearby enemies | Poison Arrow — poisons and slows | Hurl — x3 heavy throw + stun | Rune Burst — small arcane explosion |
| Mage | Smite — x3 in a single hit | Bone Burst — AoE in a 50px radius | Firestorm — area fire rain | Arcane Surge — x4 nuke on a single target |
| Tank | Fortify — hit + wide-area slow | Undying — drains and steals gold in an area | Rage — x3 hit + cleave | Barrier — hit + containment zone |
| Hero | Divine Wrath — slows + x4 damage | Death Coil — x3 damage + 1 life | War Cry — x3 damage + area slow | Tempest — x3 damage + x2 AoE + slow |

Each ability reuses the same combat primitives (direct damage, area effect,
slow, extra gold/lives) combined differently per role and faction — there's
no separate ability system per unit, it all lives in a single `if/else` on
the ability name inside `entities_update()`.

---

## Balancing

### To make the game EASIER:

In `game_config.h`:
```c
#define DIFFICULTY_ENEMY_HP_MULT        0.8f    // Enemies -20% HP
#define DIFFICULTY_STARTING_GOLD_MULT   1.5f    // +50% starting gold
#define ECONOMY_STARTING_GOLD_BASE      300     // More base gold
```

### To make the game HARDER:

```c
#define DIFFICULTY_ENEMY_HP_MULT        1.3f    // Enemies +30% HP
#define DIFFICULTY_TOWER_COST_MULT      1.5f    // Towers +50% more expensive
#define WAVE_ENEMY_SPEED_SCALE_PER_5    0.20f   // +20% speed/5 waves
```

---

## Asset Generation Tools

All of TD64's art is generated by code (Python + Pillow), with no AI image
generator involved in the final result. The scripts live in `tools/`:

| Script | Generates | Output |
|---|---|---|
| `gen_sprites.py` | The 24 animated unit sheets (8 frames each) + 4 projectiles + the reference sheet | `assets/sprites/<faction>_<type>.png`, `sprite_sheet.png` |
| `gen_maps.py` | The 7 terrain textures (`tile_*.png`, 16x16) and 4 full-map reference images (320x240, not used at runtime — see note below) | `assets/sprites/tile_*.png`, `map_*.png` |
| `downsample_images.py` | Converts an external image (e.g. AI-generated) to the dimensions and format N64 expects (`unit` 32x32, `sheet` 256x32, `projectile` 16x16, `background` 320x240), removing the black background and quantizing to RGBA16 | The file you point it at, in `assets/sprites/` |

```bash
cd tools
python gen_sprites.py        # regenerates the 24 units + projectiles
python gen_maps.py           # regenerates the terrain textures
```

**Note:** `map_*.png` (the 4 full-map images with the path already drawn)
are a byproduct of `gen_maps.py`, but the game **doesn't use them** — terrain
is composed at runtime from individual tiles (`terrain_compose()`, see
[Animated Sprites and Textured Terrain](#animated-sprites-and-textured-terrain)),
because that same data grid is what the [map editor](#custom-maps-and-map-editor)
edits. They remain as a visual reference for what each full map looks like,
not as an active asset.

`downsample_images.py` exists for the case where you want to bring in
external art (hand-illustrated or AI-generated) while respecting N64's
constraints — today no active game asset uses that path, all active art is
100% procedural.

---

## Building and Environment Setup

The game is designed to build against the official Libdragon release found
on GitHub and Docker Hub, without altering any internal library file.
Compiler tweaks are isolated to the project's local `makefile`.

### Prerequisites

- Docker Desktop installed and running.
- Python 3.x installed locally (for the sprite scripts).
- The Pillow library for Python (`pip install Pillow`).

### Building with the libdragon CLI (Recommended)

The `libdragon` CLI automatically manages the Docker container and the SDK's
Makefile calls.

1. Install the libdragon CLI via npm:
   ```bash
   npm install -g libdragon
   ```
2. Generate local sprites:
   ```bash
   python tools/gen_sprites.py
   ```
3. Initialize the container (first time only):
   ```bash
   libdragon init
   ```
4. Build the ROM:
   ```bash
   libdragon make
   ```
   This produces `TD64.z64` in the project root.
5. Clean intermediate files:
   ```bash
   libdragon make clean
   ```

### Building Directly with Docker (No CLI)

1. Generate local sprites:
   ```bash
   python tools/gen_sprites.py
   ```
2. Build the ROM:
   ```bash
   docker run --rm -v "%cd%:/src" -w /src ghcr.io/dragonminded/libdragon:latest make
   ```
   *(Replace `%cd%` with `$(pwd)` on Linux or macOS)*
3. Clean the build:
   ```bash
   docker run --rm -v "%cd%:/src" -w /src ghcr.io/dragonminded/libdragon:latest make clean
   ```

### Running the Game

Once the ROM (`TD64.z64`) builds successfully, run it with:
- **Ares:** Recommended high-accuracy emulator.
- **Project64:** Configure memory to 8MB (Expansion Pak enabled) to avoid failures.
- **Real hardware (Nintendo 64):** Use flash carts like EverDrive64 or SummerCart64 on a console with the Expansion Pak module installed.

---

## Credits

- **Engine:** libdragon N64 SDK
- **Sprites:** Generative pixel art with Python + PIL
- **Design:** Inspired by Dragonshard, Warcraft 3, and classic TDs

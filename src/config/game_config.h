#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

// =============================================================================
// game_config.h — MASTER GAME CONFIGURATION
//
// This file contains ALL of the game's balance values.
// Changing values here does NOT require touching code — just recompiling.
//
// Sections:
//  1. GLOBAL ECONOMY
//  2. WAVES
//  3. MAPS
//  4. SCORE SYSTEM
//  5. DIFFICULTY
//  6. DEBUG OPTIONS
// =============================================================================

// =============================================================================
// 1. GLOBAL ECONOMY
// =============================================================================

// Base starting gold (can be overridden per map)
#define ECONOMY_STARTING_GOLD_BASE      200

// Cost multiplier per upgrade tier
#define ECONOMY_UPGRADE_COST_MULT       1.5f
#define ECONOMY_UPGRADE_BASE_MULT       0.6f    // upgrade costs 60% of the base price

// Interest on accumulated gold (% per second)
#define ECONOMY_INTEREST_RATE           0.0f    // 0 = disabled, 0.01 = 1% per second

// Gold bonus per wave cleared
#define ECONOMY_WAVE_CLEAR_BASE_GOLD    50
#define ECONOMY_WAVE_CLEAR_PER_WAVE     20      // +20 per wave number

// Penalty per life lost (gold)
#define ECONOMY_LIFE_LOST_PENALTY       0       // 0 = no penalty

// =============================================================================
// 2. WAVES (WAVE SYSTEM)
// =============================================================================

// Wave composition — how many of each type spawn
// Formula: BASE + (wave_number - START_WAVE) * SCALE
//
// Example: SCOUT with base=3, start=1, scale=2
//   Wave 1:  3 + (1-1)*2 = 3 scouts
//   Wave 2:  3 + (2-1)*2 = 5 scouts
//   Wave 10: 3 + (10-1)*2 = 21 scouts

#define WAVE_SCOUT_BASE                 3       // Base scouts per wave
#define WAVE_SCOUT_START_WAVE           1       // Appear starting wave 1
#define WAVE_SCOUT_SCALE                2       // +2 per wave

#define WAVE_WARRIOR_BASE               1
#define WAVE_WARRIOR_START_WAVE         2       // Appear starting wave 2
#define WAVE_WARRIOR_SCALE              1

#define WAVE_ARCHER_BASE                1
#define WAVE_ARCHER_START_WAVE          4       // Appear starting wave 4
#define WAVE_ARCHER_SCALE               1       // +1 every 2 waves (see code)

#define WAVE_MAGE_BASE                  1
#define WAVE_MAGE_START_WAVE            6
#define WAVE_MAGE_SCALE                 1       // +1 every 3 waves

#define WAVE_TANK_BASE                  1
#define WAVE_TANK_START_WAVE            8
#define WAVE_TANK_SCALE                 1       // +1 every 4 waves

// Enemy-only elite variants (UNIT_ARMORED/WARDED/FLYER, factions.h) — same
// BASE+scale-every-N-waves shape as archer/mage/tank above. Start waves are
// deliberately low enough that even an Easy map (WAVES_PER_MAP_EASY=10
// below) sees at least the Flyer, so the resistance mechanic isn't locked
// behind Hard/Extreme only.
#define WAVE_FLYER_BASE                 1
#define WAVE_FLYER_START_WAVE           4       // needs an Archer or a Mage
#define WAVE_FLYER_SCALE                1       // +1 every 3 waves

#define WAVE_ARMORED_BASE               1
#define WAVE_ARMORED_START_WAVE         6       // needs a Mage
#define WAVE_ARMORED_SCALE              1       // +1 every 4 waves

#define WAVE_WARDED_BASE                1
#define WAVE_WARDED_START_WAVE          8       // needs a Scout/Warrior/Archer/Tank/Hero
#define WAVE_WARDED_SCALE               1       // +1 every 4 waves

// Hero (boss) — how often it appears
#define WAVE_HERO_INTERVAL              5       // Waves 5, 10, 15...
#define WAVE_HERO_COUNT                 1       // How many heroes per boss wave

// Enemy stat scaling per wave
#define WAVE_ENEMY_HP_SCALE_PER_5       0.25f   // +25% HP every 5 waves
#define WAVE_ENEMY_DMG_SCALE_PER_5      0.15f   // +15% damage every 5 waves
#define WAVE_ENEMY_SPEED_SCALE_PER_5    0.10f   // +10% speed every 5 waves
#define WAVE_ENEMY_GOLD_SCALE_PER_5     0.20f   // +20% gold reward every 5 waves

// Spawn timing
#define WAVE_SPAWN_INTERVAL             0.8f    // Seconds between each enemy spawn
#define WAVE_SPAWN_INTERVAL_BOSS        1.5f    // Slower for boss waves

// Random wave events (% chance)
#define WAVE_RANDOM_DOUBLE_GOLD         10      // 10% chance: x2 gold this wave
#define WAVE_RANDOM_MINI_BOSS           15      // 15% chance: +1 random hero
#define WAVE_RANDOM_SPEED_BOOST         20      // 20% chance: enemies +30% speed
#define WAVE_RANDOM_HP_BOOST            20      // 20% chance: enemies +50% HP

// Waves required to complete a map
#define WAVES_PER_MAP_EASY              10
#define WAVES_PER_MAP_NORMAL            15
#define WAVES_PER_MAP_HARD              20
#define WAVES_PER_MAP_EXTREME           25

// =============================================================================
// 3. MAPS
// =============================================================================

// Number of available maps
#define MAP_COUNT                       4

// Map IDs
#define MAP_GREENFIELD                  0
#define MAP_DESERT_CROSSING             1
#define MAP_FROZEN_HIGHLANDS            2
#define MAP_VOLCANIC_PASS               3

// Map configuration
// Each map has: starting gold, lives, difficulty, path type, dominant terrain

// MAP 0: Greenfield (easy, tutorial)
#define MAP0_STARTING_GOLD              200
#define MAP0_STARTING_LIVES             20
#define MAP0_DIFFICULTY                 1       // 1-5
#define MAP0_PATH_TYPE                  0       // 0=curve, 1=zigzag, 2=spiral, 3=straight
#define MAP0_TERRAIN_PRIMARY            0       // TERRAIN_GRASS
#define MAP0_WAVES_TO_COMPLETE          10

// MAP 1: Desert Crossing (medium)
#define MAP1_STARTING_GOLD              175
#define MAP1_STARTING_LIVES             18
#define MAP1_DIFFICULTY                 2
#define MAP1_PATH_TYPE                  1       // zigzag
#define MAP1_TERRAIN_PRIMARY            3       // TERRAIN_DESERT
#define MAP1_WAVES_TO_COMPLETE          15

// MAP 2: Frozen Highlands (hard)
#define MAP2_STARTING_GOLD              160
#define MAP2_STARTING_LIVES             15
#define MAP2_DIFFICULTY                 3
#define MAP2_PATH_TYPE                  2       // spiral
#define MAP2_TERRAIN_PRIMARY            4       // TERRAIN_SNOW
#define MAP2_WAVES_TO_COMPLETE          18

// MAP 3: Volcanic Pass (extreme)
#define MAP3_STARTING_GOLD              150
#define MAP3_STARTING_LIVES             12
#define MAP3_DIFFICULTY                 4
#define MAP3_PATH_TYPE                  3       // straight (less time to attack)
#define MAP3_TERRAIN_PRIMARY            2       // TERRAIN_MOUNTAIN
#define MAP3_WAVES_TO_COMPLETE          20

// Buffs/debuffs per terrain type (already implemented in terrain.c, this is reference only)
//
// GRASS:    1.0x everything (neutral)
// WATER:    towers +25% range, everything -25% speed
// MOUNTAIN: towers +20% dmg, +50% range; enemies -45% speed
// DESERT:   towers -10% dmg/range; enemies +30% speed
// SNOW:     towers -35% speed; enemies -50% speed

// =============================================================================
// 4. SCORE SYSTEM
// =============================================================================

// Base points per kill by enemy type
#define SCORE_KILL_SCOUT                50
#define SCORE_KILL_WARRIOR              100
#define SCORE_KILL_ARCHER               80
#define SCORE_KILL_MAGE                 120
#define SCORE_KILL_TANK                 200
#define SCORE_KILL_HERO                 500

// Combo system
#define SCORE_COMBO_TIMEOUT             3.0f    // Seconds without a kill before losing the combo
#define SCORE_COMBO_MAX_MULT            10      // Max multiplier (x10)
#define SCORE_COMBO_INCREMENT           1       // +1 mult per consecutive kill

// Bonus per wave cleared
#define SCORE_WAVE_CLEAR_BASE           400
#define SCORE_WAVE_CLEAR_PER_WAVE       100     // wave 1=500, wave 2=600, etc

// Bonus for a perfect wave (no lives lost)
#define SCORE_PERFECT_WAVE_BASE         800
#define SCORE_PERFECT_WAVE_PER_WAVE     200

// Speed bonus (clearing a wave quickly)
#define SCORE_SPEED_BONUS_THRESHOLD     30.0f   // Seconds — bonus applies if you finish before this
#define SCORE_SPEED_BONUS_PER_SECOND    20      // +20 points per second saved
#define SCORE_SPEED_BONUS_MAX           1000    // Max cap on the speed bonus

// Penalty for taking too long
#define SCORE_TIME_PENALTY_THRESHOLD    120.0f  // Seconds — penalty applies if you take longer than this
#define SCORE_TIME_PENALTY_PER_SECOND   10      // -10 points per extra second
#define SCORE_TIME_PENALTY_MAX          500     // Cap on the penalty

// Bonus for lives remaining at the end of a map
#define SCORE_LIFE_REMAINING_MULT       100     // Each life = 100 points

// Bonus for gold remaining at the end
#define SCORE_GOLD_REMAINING_MULT       1       // Each 1 gold = 1 point
#define SCORE_GOLD_REMAINING_THRESHOLD  500     // Only if you have >500 gold

// Multiplier by map difficulty
#define SCORE_DIFFICULTY_MULT_EASY      1.0f
#define SCORE_DIFFICULTY_MULT_NORMAL    1.5f
#define SCORE_DIFFICULTY_MULT_HARD      2.0f
#define SCORE_DIFFICULTY_MULT_EXTREME   3.0f

// Final bonus for completing the campaign
#define SCORE_CAMPAIGN_COMPLETE_BONUS   10000

// =============================================================================
// 5. DIFFICULTY (global tuning)
// =============================================================================

// Multipliers that affect the ENTIRE game
// Use 1.0 = normal, <1.0 = easier, >1.0 = harder

#define DIFFICULTY_ENEMY_HP_MULT        1.0f    // x1.2 = enemies +20% HP
#define DIFFICULTY_ENEMY_DMG_MULT       1.0f
#define DIFFICULTY_ENEMY_SPEED_MULT     1.0f
#define DIFFICULTY_ENEMY_GOLD_MULT      1.0f    // x0.8 = enemies give -20% gold

#define DIFFICULTY_TOWER_DMG_MULT       1.0f
#define DIFFICULTY_TOWER_RANGE_MULT     1.0f
#define DIFFICULTY_TOWER_SPEED_MULT     1.0f    // Higher = lower cooldown
#define DIFFICULTY_TOWER_COST_MULT      1.0f    // x1.5 = towers +50% more expensive

#define DIFFICULTY_STARTING_GOLD_MULT   1.0f
#define DIFFICULTY_STARTING_LIVES_MULT  1.0f

// =============================================================================
// 6. DEBUG OPTIONS
// =============================================================================

// What to show in the debug overlay (0 = off, 1 = on)
// Toggled with C-up (FPS), C-down (grid), C-left (AI), C-right (perf)

#define DEBUG_SHOW_FPS_DEFAULT          0       // FPS counter
#define DEBUG_SHOW_ENTITY_COUNT         0       // Number of active entities
#define DEBUG_SHOW_MEMORY_STATS         0       // Pool usage (X/128 entities, etc)
#define DEBUG_SHOW_COLLISION_BOXES      0       // Hitboxes for all entities
#define DEBUG_SHOW_RANGE_CIRCLES        0       // Tower range circles
#define DEBUG_SHOW_PATHFINDING          0       // Waypoints and path lines
#define DEBUG_SHOW_WAVE_PREVIEW         0       // Next wave composition
#define DEBUG_SHOW_ECONOMY_INFO         0       // Gold/sec, DPS, value
#define DEBUG_SHOW_AI_TARGETING         0       // Tower -> enemy targeting lines
#define DEBUG_SHOW_PERFORMANCE_TIMERS   0       // Update/render time (µs)
#define DEBUG_SHOW_GRID_OVERLAY         0       // Visible terrain grid
#define DEBUG_SHOW_DAMAGE_NUMBERS       1       // Floating damage numbers (always useful)

// Gameplay debug options
#define DEBUG_GODMODE                   0       // Infinite lives
#define DEBUG_INFINITE_GOLD             0       // Infinite gold
#define DEBUG_INSTANT_WAVES             0       // Waves spawn instantly
#define DEBUG_SLOW_MOTION               0       // Game at 50% speed
#define DEBUG_FAST_FORWARD              0       // Game at 200% speed
#define DEBUG_ONE_HIT_KILLS             0       // Towers kill in 1 hit

// Debug overlay colors
#define DEBUG_COLOR_FPS                 RGBA32(255, 255, 100, 255)
#define DEBUG_COLOR_HITBOX              RGBA32(255, 50, 50, 120)
#define DEBUG_COLOR_RANGE               RGBA32(100, 255, 100, 80)
#define DEBUG_COLOR_PATH                RGBA32(255, 100, 255, 200)
#define DEBUG_COLOR_GRID                RGBA32(80, 80, 120, 60)
#define DEBUG_COLOR_TARGET_LINE         RGBA32(255, 200, 0, 150)

// =============================================================================
// 7. ADVANCED — FINE TWEAKS
// =============================================================================

// Projectiles
#define PROJECTILE_BASE_SPEED           160.0f  // Pixels per second
#define PROJECTILE_HOMING_STRENGTH      1.0f    // 1.0 = perfect homing, 0.0 = straight line

// Camera
#define CAMERA_SHAKE_MULTIPLIER         1.0f    // x2.0 = twice as intense shake

// Particles
#define PARTICLE_SPAWN_MULTIPLIER       1.0f    // x0.5 = half the particles (performance)

// UI
#define UI_COMBO_DISPLAY_MIN            2       // Only show combo if >=2
#define UI_TOOLTIP_DELAY                0.3f    // Seconds before showing a tooltip

// Animations
#define ANIMATION_SPEED_MULT            1.0f    // x2.0 = animations play 2x faster

#endif // GAME_CONFIG_H

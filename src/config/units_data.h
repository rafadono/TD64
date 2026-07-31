#ifndef UNITS_DATA_H
#define UNITS_DATA_H

#include "factions.h"
#include <libdragon.h>

// =============================================================================
// UnitStats
//
// Every value that defines a unit.
// Lives in units_data.c organized as a [FACTION][UNIT_TYPE] table.
// Edit that file to balance the game.
//
// =============================================================================

typedef struct UnitStats {
    // --- Identity ---
    const char* name;           // Name shown in the UI

    // --- Combat ---
    int   hp_max;               // Max HP
    int   damage;               // Damage per attack
    float attack_range;         // Attack radius in pixels
    float attack_cooldown;      // Seconds between attacks

    // --- Movement (only applies in MODE_RUNNER) ---
    float move_speed;           // Pixels per second

    // --- Economy ---
    int   cost;                 // Gold to place/summon
    int   gold_reward;          // Gold given on death (as an enemy)
    int   score_reward;         // Score points on kill
    int   xp_reward;            // XP given on death (for leveling up)

    // --- Visual ---
    color_t color_primary;      // Main tint color
    color_t color_secondary;    // Detail/accent color
    int     size;               // Size in pixels (square)

    // --- Level up (percentage improvement per level) ---
    float level_hp_pct;         // e.g. 0.20 = +20% HP per level
    float level_damage_pct;     // e.g. 0.15 = +15% damage per level
    float level_range_pct;      // e.g. 0.10 = +10% range per level
    float level_speed_pct;      // e.g. 0.05 = +5% speed per level

    // --- Special ability (for heroes, etc.) ---
    const char* ability_name;   // Ability name
    float ability_cooldown;     // 0 = has none

} UnitStats;

// =============================================================================
// Main table — defined in units_data.c
// Access: ALL_UNITS[faction][unit_type]
// =============================================================================

extern const UnitStats ALL_UNITS[FACTION_COUNT][UNIT_TYPE_COUNT];

// Helpers
const UnitStats* unit_get_stats(FactionId faction, UnitType type);
const char*      unit_get_name (FactionId faction, UnitType type);

#endif // UNITS_DATA_H

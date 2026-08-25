#ifndef UNITS_DATA_H
#define UNITS_DATA_H

#include "factions.h"
#include <libdragon.h>
#include <stdbool.h>

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

    // --- Resistances (Crystal Defenders-style) ---
    // damage_type: what type of damage THIS unit deals when attacking as a
    // tower (irrelevant/unused for the enemy-only types below, which never
    // attack). immune_to_mask: bitmask of DamageType this unit takes ZERO
    // damage from when it's the target (0 = no immunities — every one of
    // the original 6 playable roles). Appended at the end of the struct so
    // every existing positional initializer in units_data.c keeps working;
    // only entries that explicitly set them differ.
    DamageType damage_type;
    uint8_t    immune_to_mask;

} UnitStats;

// Convenience masks for immune_to_mask.
#define IMMUNE_TO_PHYSICAL   ((1 << DMG_PHYSICAL_MELEE) | (1 << DMG_PHYSICAL_RANGED))
#define IMMUNE_TO_MAGIC      (1 << DMG_MAGIC)
#define IMMUNE_TO_MELEE_ONLY (1 << DMG_PHYSICAL_MELEE)

// True if a hit of `atk` type is fully negated by `target`'s immunities.
static inline bool unit_stats_blocks_damage(const UnitStats* target, DamageType atk) {
    if (!target) return false;
    return (target->immune_to_mask & (1 << atk)) != 0;
}

// =============================================================================
// Main table — defined in units_data.c
// Access: ALL_UNITS[faction][unit_type]
// =============================================================================

extern const UnitStats ALL_UNITS[FACTION_COUNT][UNIT_TYPE_COUNT];

// Helpers
const UnitStats* unit_get_stats(FactionId faction, UnitType type);
const char*      unit_get_name (FactionId faction, UnitType type);

#endif // UNITS_DATA_H

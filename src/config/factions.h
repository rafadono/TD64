#ifndef FACTIONS_H
#define FACTIONS_H

// =============================================================================
// FACTIONS
// =============================================================================
//
// 4 factions in a D&D Dragonshard style.
// Each faction can be playable or the enemy depending on the campaign.
//
//  DAWNGUARD  - Holy knights. Balanced, disciplined.
//  IRONBONE   - Undead. High HP, energy drain.
//  ASHCLAW    - Savages / Orcs. Fast, aggressive, fragile.
//  VEILSTORM  - Arcane mages. High damage, low HP, long range.
//
// =============================================================================

typedef enum {
    FACTION_DAWNGUARD = 0,   // Knights / Holy
    FACTION_IRONBONE  = 1,   // Undead  / Dark
    FACTION_ASHCLAW   = 2,   // Orcs    / Savage
    FACTION_VEILSTORM = 3,   // Mages   / Arcane
    FACTION_COUNT     = 4
} FactionId;

// =============================================================================
// UNIT TYPES
// (same roles across every faction, different names and sprites)
// =============================================================================

typedef enum {
    UNIT_SCOUT   = 0,  // Fast, cheap, low HP
    UNIT_WARRIOR = 1,  // Balanced, frontline
    UNIT_ARCHER  = 2,  // Medium range, good DPS
    UNIT_MAGE    = 3,  // AoE, long range, slow cooldown
    UNIT_TANK    = 4,  // Very high HP, slow, expensive
    UNIT_HERO    = 5,  // Unique per faction, very powerful

    // --- Enemy-only elite variants (never placeable as towers — see
    // PLAYABLE_UNIT_TYPE_COUNT below) introduced in later waves, inspired by
    // Final Fantasy: Crystal Defenders' resistance mechanic: each is fully
    // immune to one or two DamageType(s) (see units_data.h), so beating one
    // requires bringing the right tower type, not just more towers. ---
    UNIT_ARMORED = 6,  // Immune to ALL physical damage (melee + ranged) — needs a Mage
    UNIT_WARDED  = 7,  // Immune to magic damage — needs Scout/Warrior/Archer/Tank/Hero
    UNIT_FLYER   = 8,  // Immune to melee only — needs an Archer or a Mage

    UNIT_TYPE_COUNT = 9
} UnitType;

// The first PLAYABLE_UNIT_TYPE_COUNT values of UnitType are placeable as
// towers (cycled with L/R in the build menu, ui.c); UNIT_ARMORED/WARDED/
// FLYER only ever appear as enemies and must stay excluded from that cycle.
#define PLAYABLE_UNIT_TYPE_COUNT 6

// =============================================================================
// DAMAGE TYPES — what kind of damage a tower's attack deals. Matched against
// a target's immune_to_mask (units_data.h) in entity_deal_damage() to decide
// whether a hit lands at all.
// =============================================================================
typedef enum {
    DMG_PHYSICAL_MELEE  = 0,  // Scout / Warrior / Tank (and most Heroes' base attack)
    DMG_PHYSICAL_RANGED = 1,  // Archer (arrows/thrown weapons)
    DMG_MAGIC           = 2,  // Mage (and some Heroes)
    DAMAGE_TYPE_COUNT   = 3
} DamageType;

// =============================================================================
// TEAM (in any match, a unit belongs to the player or the enemy)
// =============================================================================

typedef enum {
    TEAM_PLAYER  = 0,
    TEAM_ENEMY   = 1
} Team;

// =============================================================================
// UNIT MODE (how it behaves on the map)
// =============================================================================

typedef enum {
    MODE_TOWER   = 0,  // Static, attacks enemies passing by
    MODE_RUNNER  = 1,  // Follows the path toward the enemy base
} UnitMode;

// =============================================================================
// FACTION INFO (faction and unit names)
// =============================================================================

typedef struct {
    const char* faction_name;
    const char* unit_names[UNIT_TYPE_COUNT];
} FactionInfo;

// Declared in units_data.c
extern const FactionInfo FACTION_INFO[FACTION_COUNT];

// Localized description (English/Spanish, see src/systems/lang.h) — not a
// plain struct field like the names above, because it must follow the
// current language at render time. Faction/unit names are proper nouns and
// stay the same in every language.
const char* faction_get_description(FactionId f);

// =============================================================================
// HELPER MACROS
// =============================================================================

#define FACTION_NAME(f)        FACTION_INFO[f].faction_name
#define UNIT_NAME(f, u)        FACTION_INFO[f].unit_names[u]

#endif // FACTIONS_H

// =============================================================================
// units_data.c  —  MASTER UNIT TABLE
//
// This is the only file you need to edit to balance the game.
//
// Structure of each entry:
//  { name, hp, damage, range, cooldown, speed,
//    cost, gold_reward, score_reward, xp_reward,
//    color_primary, color_secondary, size,
//    %hp/lvl, %dmg/lvl, %rng/lvl, %spd/lvl,
//    "ability", ability_cooldown }
//
// Factions:
//   DAWNGUARD  — Knights, balanced, holy
//   IRONBONE   — Undead, high HP, drain
//   ASHCLAW    — Savages, speed and aggression
//   VEILSTORM  — Mages, maximum range, glass cannon
// =============================================================================

#include "../config/units_data.h"
#include "../systems/lang.h"

// Shorthand for colors
#define C(r,g,b) RGBA32(r, g, b, 255)

const UnitStats ALL_UNITS[FACTION_COUNT][UNIT_TYPE_COUNT] = {

// =============================================================================
// FACTION 0 — DAWNGUARD (Holy Knights)
// Palette: Blue / Gold / White
// Style: Balanced, reliable, no extremes
// =============================================================================
[FACTION_DAWNGUARD] = {

    [UNIT_SCOUT] = {
    //  name              hp  dmg  range  cooldown  speed   cost  gold  score  xp
        "Dawnscout",      60,  8,  55.0f,  0.7f,   50.0f,   40,   8,   80,   20,
        C(80,140,220), C(200,180,60), 12,   // color primary, secondary, size
        0.20f, 0.15f, 0.08f, 0.10f,         // level %: hp, dmg, range, speed
        "Dash", 8.0f
    },

    [UNIT_WARRIOR] = {
    //  name              hp  dmg  range  cooldown  speed   cost  gold  score  xp
        "Crusader",      120, 15,  70.0f,  1.0f,   28.0f,   65,  12,  120,   40,
        C(60,100,200), C(200,180,60), 14,
        0.25f, 0.18f, 0.08f, 0.05f,
        "Holy Shield", 12.0f
    },

    [UNIT_ARCHER] = {
    //  name              hp  dmg  range  cooldown  speed   cost  gold  score  xp
        "Lightarrow",     80, 12,  110.0f, 0.9f,   35.0f,   75,  15,  100,   35,
        C(100,160,240), C(240,210,80), 13,
        0.20f, 0.20f, 0.12f, 0.06f,
        "Volley", 10.0f
    },

    [UNIT_MAGE] = {
    //  name              hp  dmg  range  cooldown  speed   cost  gold  score  xp
        "Chaplain",       70, 25,  130.0f, 2.5f,   25.0f,  100,  15,  150,   50,
        C(150,180,255), C(255,240,120), 14,
        0.15f, 0.25f, 0.15f, 0.05f,
        "Smite", 15.0f
    },

    [UNIT_TANK] = {
    //  name              hp  dmg  range  cooldown  speed   cost  gold  score  xp
        "Ironshield",    280, 20,  60.0f,  1.5f,   15.0f,  130,  25,  200,   80,
        C(40, 80,180), C(180,160,50), 18,
        0.30f, 0.12f, 0.05f, 0.03f,
        "Fortify", 20.0f
    },

    [UNIT_HERO] = {
    //  name              hp  dmg  range  cooldown  speed   cost  gold  score  xp
        "Archangel",     350, 45,  150.0f, 1.8f,   40.0f,  250,  60,  500,  200,
        C(200,220,255), C(255,230,80), 20,
        0.35f, 0.30f, 0.15f, 0.10f,
        "Divine Wrath", 20.0f
    },
}, // END DAWNGUARD


// =============================================================================
// FACTION 1 — IRONBONE (Undead)
// Palette: Purple / Black / Toxic green
// Style: Draining tanks, slow but unstoppable
// =============================================================================
[FACTION_IRONBONE] = {

    [UNIT_SCOUT] = {
        "Shade",          50,  7,  60.0f,  0.6f,   55.0f,   35,   8,   70,   18,
        C(120,40,160), C(80,200,80), 12,
        0.18f, 0.15f, 0.08f, 0.12f,
        "Phase", 9.0f
    },

    [UNIT_WARRIOR] = {
        "Boneknight",    150, 12,  65.0f,  1.1f,   22.0f,   60,  14,  130,   45,
        C(100,20,140), C(60,180,60), 14,
        0.30f, 0.15f, 0.07f, 0.04f,
        "Life Drain", 14.0f
    },

    [UNIT_ARCHER] = {
        "Plagueshot",     75, 14,  100.0f, 1.0f,   30.0f,   70,  14,  110,   38,
        C(130,50,170), C(90,210,90), 13,
        0.20f, 0.22f, 0.10f, 0.06f,
        "Poison Arrow", 12.0f
    },

    [UNIT_MAGE] = {
        "Lich",           60, 30,  140.0f, 2.8f,   20.0f,   95,  18,  160,   55,
        C(160,60,200), C(120,255,120), 14,
        0.12f, 0.28f, 0.18f, 0.04f,
        "Bone Burst", 18.0f
    },

    [UNIT_TANK] = {
        "Deathwall",     350, 18,  55.0f,  1.6f,   10.0f,  125,  30,  220,   90,
        C(80, 0,120), C(40,160,40), 18,
        0.35f, 0.10f, 0.04f, 0.02f,
        "Undying", 25.0f
    },

    [UNIT_HERO] = {
        "Dreadlord",     420, 40,  140.0f, 2.0f,   35.0f,  250,  65,  520,  210,
        C(180,20,220), C(160,255,160), 20,
        0.40f, 0.28f, 0.14f, 0.08f,
        "Death Coil", 22.0f
    },
}, // END IRONBONE


// =============================================================================
// FACTION 2 — ASHCLAW (Savages / Orcs)
// Palette: Red / Orange / Dark brown
// Style: Maximum damage and speed, low HP, chaotic
// =============================================================================
[FACTION_ASHCLAW] = {

    [UNIT_SCOUT] = {
        "Feral",          45, 10,  50.0f,  0.6f,   65.0f,   35,   7,   75,   20,
        C(220,80,40),  C(240,160,40), 12,
        0.15f, 0.20f, 0.07f, 0.15f,
        "Berserk", 7.0f
    },

    [UNIT_WARRIOR] = {
        "Brute",         100, 20,  65.0f,  0.9f,   32.0f,   60,  13,  130,   42,
        C(200,60,30),  C(220,140,30), 14,
        0.20f, 0.22f, 0.07f, 0.08f,
        "Slam", 10.0f
    },

    [UNIT_ARCHER] = {
        "Spearblood",     70, 18,  95.0f,  0.85f,  38.0f,   70,  14,  115,   40,
        C(210,70,35),  C(230,150,35), 13,
        0.18f, 0.22f, 0.10f, 0.08f,
        "Hurl", 9.0f
    },

    [UNIT_MAGE] = {
        "Shaman",         55, 35,  120.0f, 2.2f,   28.0f,   90,  16,  155,   52,
        C(180,50,25),  C(255,180,50), 14,
        0.12f, 0.30f, 0.12f, 0.06f,
        "Firestorm", 16.0f
    },

    [UNIT_TANK] = {
        "Crusher",       240, 28,  58.0f,  1.3f,   18.0f,  120,  28,  210,   85,
        C(160,40,20),  C(200,120,20), 18,
        0.25f, 0.18f, 0.05f, 0.05f,
        "Rage", 18.0f
    },

    [UNIT_HERO] = {
        "Warchief",      320, 55,  130.0f, 1.5f,   45.0f,  250,  60,  510,  205,
        C(240,60,20),  C(255,200,40), 20,
        0.30f, 0.35f, 0.12f, 0.12f,
        "War Cry", 18.0f
    },
}, // END ASHCLAW


// =============================================================================
// FACTION 3 — VEILSTORM (Arcane Mages)
// Palette: Cyan / Violet / Electric white
// Style: Maximum range and AoE, extremely fragile
// =============================================================================
[FACTION_VEILSTORM] = {

    [UNIT_SCOUT] = {
        "Wisp",           35,  9,  65.0f,  0.7f,   58.0f,   38,   7,   72,   18,
        C(80,200,220), C(180,100,255), 11,
        0.15f, 0.18f, 0.12f, 0.12f,
        "Blink", 7.0f
    },

    [UNIT_WARRIOR] = {
        "Spellblade",     85, 17,  80.0f,  0.95f,  30.0f,   65,  12,  125,   40,
        C(60,180,210), C(160,80,240), 13,
        0.18f, 0.20f, 0.12f, 0.06f,
        "Chain Strike", 11.0f
    },

    [UNIT_ARCHER] = {
        "Runeshot",       65, 16,  130.0f, 0.95f,  33.0f,   75,  15,  108,   36,
        C(90,210,230), C(190,110,255), 12,
        0.16f, 0.22f, 0.15f, 0.06f,
        "Rune Burst", 11.0f
    },

    [UNIT_MAGE] = {
        "Arcanist",       50, 40,  160.0f, 3.0f,   22.0f,  100,  20,  170,   60,
        C(120,230,250), C(210,140,255), 13,
        0.10f, 0.32f, 0.20f, 0.04f,
        "Arcane Surge", 20.0f
    },

    [UNIT_TANK] = {
        "Golem",         200, 22,  70.0f,  1.4f,   14.0f,  120,  26,  200,   80,
        C(40,160,200), C(140,60,220), 17,
        0.22f, 0.14f, 0.08f, 0.03f,
        "Barrier", 22.0f
    },

    [UNIT_HERO] = {
        "Stormlord",     280, 60,  180.0f, 2.2f,   38.0f,  250,  62,  530,  215,
        C(160,240,255), C(220,160,255), 20,
        0.28f, 0.38f, 0.20f, 0.10f,
        "Tempest", 25.0f
    },
}, // END VEILSTORM

}; // END ALL_UNITS


// =============================================================================
// NOMBRES DE FACCIONES Y SUS UNIDADES
// =============================================================================

const FactionInfo FACTION_INFO[FACTION_COUNT] = {
    [FACTION_DAWNGUARD] = {
        .faction_name = "Dawnguard",
        .unit_names = { "Dawnscout", "Crusader", "Lightarrow",
                        "Chaplain", "Ironshield", "Archangel" },
    },
    [FACTION_IRONBONE] = {
        .faction_name = "Ironbone",
        .unit_names = { "Shade", "Boneknight", "Plagueshot",
                        "Lich", "Deathwall", "Dreadlord" },
    },
    [FACTION_ASHCLAW] = {
        .faction_name = "Ashclaw",
        .unit_names = { "Feral", "Brute", "Spearblood",
                        "Shaman", "Crusher", "Warchief" },
    },
    [FACTION_VEILSTORM] = {
        .faction_name = "Veilstorm",
        .unit_names = { "Wisp", "Spellblade", "Runeshot",
                        "Arcanist", "Golem", "Stormlord" },
    }
};


// =============================================================================
// HELPERS
// =============================================================================

const UnitStats* unit_get_stats(FactionId faction, UnitType type) {
    if (faction < 0 || faction >= FACTION_COUNT) return NULL;
    if (type   < 0 || type   >= UNIT_TYPE_COUNT) return NULL;
    return &ALL_UNITS[faction][type];
}

const char* unit_get_name(FactionId faction, UnitType type) {
    const UnitStats* s = unit_get_stats(faction, type);
    return s ? s->name : "Unknown";
}

const char* faction_get_description(FactionId f) {
    switch (f) {
        case FACTION_DAWNGUARD: return T(STR_FACTION_DESC_DAWNGUARD);
        case FACTION_IRONBONE:  return T(STR_FACTION_DESC_IRONBONE);
        case FACTION_ASHCLAW:   return T(STR_FACTION_DESC_ASHCLAW);
        case FACTION_VEILSTORM: return T(STR_FACTION_DESC_VEILSTORM);
        default: return "";
    }
}

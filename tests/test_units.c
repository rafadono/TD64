#include "test_harness.h"
#include "../src/config/units_data.h"

// =============================================================================
// Tests for the Crystal Defenders-style resistance system added this
// session: unit_stats_blocks_damage() (units_data.h) plus the actual data
// in ALL_UNITS (units_data.c) — locking in the design invariants that make
// the mechanic work: only the 3 enemy-only elite variants have any
// immunity, every faction's Archer/Mage deal the damage type their
// resistances are meant to counter, etc.
// =============================================================================

static void test_blocks_damage_pure_logic(void) {
    SECTION("unit_stats_blocks_damage: bitmask logic");
    CHECK(unit_stats_blocks_damage(NULL, DMG_MAGIC) == false);

    UnitStats no_immunity = {0};
    CHECK(unit_stats_blocks_damage(&no_immunity, DMG_PHYSICAL_MELEE) == false);
    CHECK(unit_stats_blocks_damage(&no_immunity, DMG_PHYSICAL_RANGED) == false);
    CHECK(unit_stats_blocks_damage(&no_immunity, DMG_MAGIC) == false);

    UnitStats armored = {0};
    armored.immune_to_mask = IMMUNE_TO_PHYSICAL;
    CHECK(unit_stats_blocks_damage(&armored, DMG_PHYSICAL_MELEE) == true);
    CHECK(unit_stats_blocks_damage(&armored, DMG_PHYSICAL_RANGED) == true);
    CHECK(unit_stats_blocks_damage(&armored, DMG_MAGIC) == false);

    UnitStats warded = {0};
    warded.immune_to_mask = IMMUNE_TO_MAGIC;
    CHECK(unit_stats_blocks_damage(&warded, DMG_PHYSICAL_MELEE) == false);
    CHECK(unit_stats_blocks_damage(&warded, DMG_PHYSICAL_RANGED) == false);
    CHECK(unit_stats_blocks_damage(&warded, DMG_MAGIC) == true);

    UnitStats flyer = {0};
    flyer.immune_to_mask = IMMUNE_TO_MELEE_ONLY;
    CHECK(unit_stats_blocks_damage(&flyer, DMG_PHYSICAL_MELEE) == true);
    CHECK(unit_stats_blocks_damage(&flyer, DMG_PHYSICAL_RANGED) == false);
    CHECK(unit_stats_blocks_damage(&flyer, DMG_MAGIC) == false);
}

static void test_only_enemy_only_variants_have_immunities(void) {
    SECTION("ALL_UNITS: only UNIT_ARMORED/WARDED/FLYER carry any immunity");
    for (int f = 0; f < FACTION_COUNT; f++) {
        for (int t = 0; t < PLAYABLE_UNIT_TYPE_COUNT; t++) {
            const UnitStats* s = unit_get_stats((FactionId)f, (UnitType)t);
            CHECK(s != NULL);
            if (s) CHECK(s->immune_to_mask == 0);
        }

        const UnitStats* armored = unit_get_stats((FactionId)f, UNIT_ARMORED);
        const UnitStats* warded  = unit_get_stats((FactionId)f, UNIT_WARDED);
        const UnitStats* flyer   = unit_get_stats((FactionId)f, UNIT_FLYER);
        CHECK(armored && armored->immune_to_mask == IMMUNE_TO_PHYSICAL);
        CHECK(warded  && warded->immune_to_mask  == IMMUNE_TO_MAGIC);
        CHECK(flyer   && flyer->immune_to_mask   == IMMUNE_TO_MELEE_ONLY);
    }
}

static void test_damage_types_are_consistent_across_factions(void) {
    SECTION("ALL_UNITS: every faction's Archer/Mage deal the type their counters expect");
    for (int f = 0; f < FACTION_COUNT; f++) {
        const UnitStats* archer = unit_get_stats((FactionId)f, UNIT_ARCHER);
        const UnitStats* mage   = unit_get_stats((FactionId)f, UNIT_MAGE);
        CHECK(archer && archer->damage_type == DMG_PHYSICAL_RANGED);
        CHECK(mage   && mage->damage_type   == DMG_MAGIC);

        // Every faction must be able to deal magic (else UNIT_WARDED would
        // be completely unkillable for that faction) and physical damage
        // (else UNIT_ARMORED would be completely unkillable) — a basic
        // "no faction is fully locked out of an enemy type" sanity check.
        bool has_magic = false, has_physical = false;
        for (int t = 0; t < PLAYABLE_UNIT_TYPE_COUNT; t++) {
            const UnitStats* s = unit_get_stats((FactionId)f, (UnitType)t);
            if (!s) continue;
            if (s->damage_type == DMG_MAGIC) has_magic = true;
            else has_physical = true;
        }
        CHECK(has_magic == true);
        CHECK(has_physical == true);
    }
}

static void test_enemy_only_variants_are_excluded_from_playable_range(void) {
    SECTION("PLAYABLE_UNIT_TYPE_COUNT excludes UNIT_ARMORED/WARDED/FLYER");
    CHECK(PLAYABLE_UNIT_TYPE_COUNT <= (int)UNIT_ARMORED);
    CHECK(UNIT_ARMORED < UNIT_TYPE_COUNT);
    CHECK(UNIT_WARDED  < UNIT_TYPE_COUNT);
    CHECK(UNIT_FLYER   < UNIT_TYPE_COUNT);
}

int main(void) {
    test_blocks_damage_pure_logic();
    test_only_enemy_only_variants_have_immunities();
    test_damage_types_are_consistent_across_factions();
    test_enemy_only_variants_are_excluded_from_playable_range();
    SUMMARY_AND_RETURN();
}

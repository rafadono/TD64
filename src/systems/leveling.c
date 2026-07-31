#include "leveling.h"
#include "../core/engine.h"
#include <math.h>

void level_init(UnitLevel* lvl) {
    lvl->level = 1;
    lvl->xp = 0;
    lvl->xp_for_next = 100;
}

bool level_add_xp(UnitLevel* lvl, int xp, const UnitStats* s, Entity* e) {
    // Stats are computed dynamically based on level inside entities_update()
    // to avoid double-compounding base stats and properly handle terrain modifiers.
    (void)s;
    (void)e;

    if (lvl->level >= LEVEL_MAX) return false;

    lvl->xp += xp;
    bool leveled_up = false;

    while (lvl->level < LEVEL_MAX && lvl->xp >= lvl->xp_for_next) {
        lvl->xp -= lvl->xp_for_next;
        lvl->level++;
        lvl->xp_for_next = (int)(100 * powf(1.5f, lvl->level - 1));
        leveled_up = true;
        debugf("LEVEL UP: %d\n", lvl->level);
    }

    if (lvl->level >= LEVEL_MAX) lvl->xp = 0;

    return leveled_up;
}

float level_get_range_bonus(int level)  { return 1.0f + (level - 1) * 0.08f; }
float level_get_speed_bonus(int level)  { return 1.0f - (level - 1) * 0.04f; }

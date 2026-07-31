#ifndef LEVELING_H
#define LEVELING_H
#include <stdbool.h>
#define LEVEL_MAX 10
typedef struct { int level, xp, xp_for_next; } UnitLevel;
struct UnitStats; struct Entity; // forward
void level_init(UnitLevel* lvl);
bool level_add_xp(UnitLevel* lvl, int xp, const struct UnitStats* s, struct Entity* e);
float level_get_range_bonus(int level);
float level_get_speed_bonus(int level);
#endif

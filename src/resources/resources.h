#ifndef RESOURCES_H
#define RESOURCES_H
#include <libdragon.h>
#include "../config/factions.h"
#include "../world/terrain.h"
void resources_init(void);
void resources_free(void);
sprite_t* resources_get_unit_sprite(FactionId f, UnitType t);
sprite_t* resources_get_projectile_sprite(FactionId f);
sprite_t* resources_get_terrain_sprite(TerrainType t);
#endif

#ifndef MAPS_H
#define MAPS_H
#include "terrain.h"
#include "pathfinding.h"
#include "../systems/save.h"
typedef struct {
    char name[32];
    int difficulty, starting_gold, starting_lives;
    // Actual play-area size in pixels for THIS map: WORLD_WIDTH/HEIGHT for
    // the 4 fixed campaign maps, SCREEN_WIDTH/HEIGHT for custom maps. Used
    // to clamp the camera's scroll range and to compose/cull only the
    // valid portion of `terrain` (which is always allocated WORLD-sized).
    int width, height;
    Path runner_path;
    TerrainMap terrain;
} MapData;
int map_get_count(void);
const char* map_get_name(int id);
void map_load(MapData* map, int id);
// Builds a MapData from a saved/edited custom map (see save.h).
void map_load_custom(MapData* map, const CustomMapSave* saved);
#endif

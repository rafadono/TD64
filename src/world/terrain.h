#ifndef TERRAIN_H
#define TERRAIN_H
#include <libdragon.h>
#define TERRAIN_GRID_SIZE 16
typedef enum {
    TERRAIN_GRASS=0, TERRAIN_WATER=1, TERRAIN_MOUNTAIN=2,
    TERRAIN_DESERT=3, TERRAIN_SNOW=4, TERRAIN_TYPE_COUNT
} TerrainType;
typedef struct {
    float tower_damage_mult, tower_range_mult, tower_speed_mult, enemy_speed_mult;
    color_t render_color;
} TerrainModifier;
typedef struct {
    TerrainType grid[320/TERRAIN_GRID_SIZE][240/TERRAIN_GRID_SIZE];
} TerrainMap;
struct Entity; // forward
void terrain_init(TerrainMap* map, TerrainType fill);
void terrain_set(TerrainMap* map, int gx, int gy, TerrainType t);
TerrainType terrain_get(const TerrainMap* map, float wx, float wy);
TerrainModifier terrain_get_modifier(TerrainType t);
// Composes the full map (every cell in map->grid) once into an offscreen
// surface using the real terrain textures (tile_grass.png, etc.) — called
// whenever the grid changes (map load, or an edit from the map editor).
// terrain_render() just blits that already-composed surface, one draw call
// per frame regardless of map complexity.
void terrain_compose(const TerrainMap* map);
void terrain_render(const TerrainMap* map);
void terrain_apply_to_entity(struct Entity* e, const TerrainMap* map);
#endif

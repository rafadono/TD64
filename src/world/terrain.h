#ifndef TERRAIN_H
#define TERRAIN_H
#include <libdragon.h>
#include "../core/screen.h"
#define TERRAIN_GRID_SIZE 16
typedef enum {
    TERRAIN_GRASS=0, TERRAIN_WATER=1, TERRAIN_MOUNTAIN=2,
    TERRAIN_DESERT=3, TERRAIN_SNOW=4, TERRAIN_LAVA=5, TERRAIN_PATH=6,
    TERRAIN_TYPE_COUNT
} TerrainType;
typedef struct {
    float tower_damage_mult, tower_range_mult, tower_speed_mult, enemy_speed_mult;
    color_t render_color;
} TerrainModifier;
typedef struct {
    // Sized from WORLD_WIDTH/HEIGHT (screen.h) — big enough for the largest
    // scrollable map (the 4 fixed campaign maps). Custom maps (always
    // exactly SCREEN_WIDTH x SCREEN_HEIGHT) just use the top-left
    // SCREEN-sized corner of this array and leave the rest unused; the
    // MapData that owns a TerrainMap knows how much of it is actually valid
    // (see MapData.width/height in world/maps.h).
    TerrainType grid[WORLD_WIDTH/TERRAIN_GRID_SIZE][WORLD_HEIGHT/TERRAIN_GRID_SIZE];
} TerrainMap;
struct Entity; // forward
// Fills the WHOLE grid (always WORLD-sized) with `fill` — harmless even for
// a screen-sized map, which just never reads past its own valid area.
void terrain_init(TerrainMap* map, TerrainType fill);
void terrain_set(TerrainMap* map, int gx, int gy, TerrainType t);
TerrainType terrain_get(const TerrainMap* map, float wx, float wy);
TerrainModifier terrain_get_modifier(TerrainType t);
// Composes the map into an offscreen surface (sized WORLD_WIDTH x
// WORLD_HEIGHT) using the real terrain textures (tile_grass.png, etc.) —
// called whenever the grid changes (map load, or an edit from the map
// editor). `width`/`height` is how much of the grid is actually valid for
// this particular map (MapData.width/height) — cells beyond that are never
// read, so a screen-sized custom map only composes its own corner.
void terrain_compose(const TerrainMap* map, int width, int height);
// Blits a SCREEN_WIDTH x SCREEN_HEIGHT window of the composed surface,
// starting at world position (cam_x, cam_y) — this is what makes the
// camera's scroll position actually show a different part of a bigger map.
// A screen-sized map's camera always clamps to (0,0), so passing (0,0)
// here (as the editor does) is exactly the old single-screen behavior.
void terrain_render(int cam_x, int cam_y);
void terrain_apply_to_entity(struct Entity* e, const TerrainMap* map);
#endif

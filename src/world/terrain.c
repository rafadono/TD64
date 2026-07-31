#include "terrain.h"
#include "../core/engine.h"
#include <string.h>

static const TerrainModifier MODIFIERS[TERRAIN_TYPE_COUNT] = {
    // GRASS — neutral
    [TERRAIN_GRASS]    = {1.0f, 1.0f, 1.0f, 1.0f, RGBA32(50,  140, 55,  255)},
    // WATER — tower range up, everything slowed
    [TERRAIN_WATER]    = {0.85f, 1.25f, 0.75f, 0.75f, RGBA32(50,  100, 200, 255)},
    // MOUNTAIN — high ground bonus, enemy slow
    [TERRAIN_MOUNTAIN] = {1.20f, 1.50f, 1.0f, 0.55f, RGBA32(100, 100, 100, 255)},
    // DESERT — tower range down, enemy speed up
    [TERRAIN_DESERT]   = {0.90f, 0.80f, 1.10f, 1.30f, RGBA32(200, 175, 95,  255)},
    // SNOW — everything slowed
    [TERRAIN_SNOW]     = {1.0f, 1.0f, 0.65f, 0.50f, RGBA32(220, 220, 250, 255)},
    // LAVA — hazardous ground: enemies wade through it very slowly, towers
    // get a small damage boost from the heat but lose some range to haze
    [TERRAIN_LAVA]     = {1.15f, 0.90f, 1.0f, 0.35f, RGBA32(230, 90,  20,  255)},
    // PATH — the road tiles a custom map's free-form path is painted on
    // (see path_init_from_terrain in pathfinding.c); a mild speed boost for
    // enemies since it's paved. Towers cannot be placed on PATH cells at all
    // (see main.c), so its tower_* multipliers are never actually used.
    [TERRAIN_PATH]     = {1.0f, 1.0f, 1.0f, 1.10f, RGBA32(150, 130, 100, 255)},
};

// Offscreen surface holding the map already composed from the real terrain
// textures. Reused across map loads (its contents are recomposed, not
// reallocated each time).
static surface_t terrain_surface;
static bool      terrain_surface_ready = false;

void terrain_init(TerrainMap* map, TerrainType fill) {
    for (int y = 0; y < SCREEN_HEIGHT/TERRAIN_GRID_SIZE; y++)
        for (int x = 0; x < SCREEN_WIDTH/TERRAIN_GRID_SIZE; x++)
            map->grid[x][y] = fill;
}

void terrain_set(TerrainMap* map, int gx, int gy, TerrainType t) {
    if (gx < 0 || gx >= SCREEN_WIDTH/TERRAIN_GRID_SIZE) return;
    if (gy < 0 || gy >= SCREEN_HEIGHT/TERRAIN_GRID_SIZE) return;
    map->grid[gx][gy] = t;
}

TerrainType terrain_get(const TerrainMap* map, float wx, float wy) {
    // TERRAIN_GRID_SIZE is a power of 2 (16); out-of-range cells fall back to
    // the same GRASS default regardless of the shift rounding differently
    // than division for negative values, so it's safe to use here.
    int gx = ((int)wx) >> 4;
    int gy = ((int)wy) >> 4;
    if (gx < 0 || gx >= SCREEN_WIDTH/TERRAIN_GRID_SIZE) return TERRAIN_GRASS;
    if (gy < 0 || gy >= SCREEN_HEIGHT/TERRAIN_GRID_SIZE) return TERRAIN_GRASS;
    return map->grid[gx][gy];
}

TerrainModifier terrain_get_modifier(TerrainType t) {
    if (t >= 0 && t < TERRAIN_TYPE_COUNT) return MODIFIERS[t];
    return MODIFIERS[TERRAIN_GRASS];
}

void terrain_compose(const TerrainMap* map) {
    if (!terrain_surface_ready) {
        terrain_surface = surface_alloc(FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT);
        terrain_surface_ready = true;
    }

    int gw = SCREEN_WIDTH  / TERRAIN_GRID_SIZE;
    int gh = SCREEN_HEIGHT / TERRAIN_GRID_SIZE;

    // This blit of up to 300 cells runs ONCE (on map load, or when the
    // editor saves an edit), never per frame — so there's no need to batch
    // by color/run the way the rest of the renderer does.
    rdpq_attach(&terrain_surface, NULL);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    for (int y = 0; y < gh; y++) {
        for (int x = 0; x < gw; x++) {
            sprite_t* tile = resources_get_terrain_sprite(map->grid[x][y]);
            if (tile) {
                rdpq_sprite_blit(tile, x * TERRAIN_GRID_SIZE, y * TERRAIN_GRID_SIZE, NULL);
            } else {
                // Fallback if the texture is missing: flat color (shouldn't happen at normal runtime)
                rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
                rdpq_set_prim_color(terrain_get_modifier(map->grid[x][y]).render_color);
                rdpq_fill_rectangle(x*TERRAIN_GRID_SIZE, y*TERRAIN_GRID_SIZE,
                                    (x+1)*TERRAIN_GRID_SIZE, (y+1)*TERRAIN_GRID_SIZE);
                rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            }
        }
    }
    rdpq_detach();
}

void terrain_render(const TerrainMap* map) {
    (void)map; // the visible content is already composed into terrain_surface
    if (!terrain_surface_ready) return;
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_tex_blit(&terrain_surface, 0, 0, NULL);
}

void terrain_apply_to_entity(Entity* e, const TerrainMap* map) {
    TerrainModifier m = terrain_get_modifier(terrain_get(map, e->x, e->y));
    if (e->mode == MODE_TOWER) {
        e->damage        = (int)(e->base_damage  * m.tower_damage_mult);
        e->attack_range  = e->base_range          * m.tower_range_mult;
        e->attack_cooldown = e->base_cooldown     * m.tower_speed_mult;
    } else if (e->mode == MODE_RUNNER) {
        e->speed = e->base_speed * m.enemy_speed_mult;
    }
}

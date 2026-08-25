#include "resources.h"
#include "../core/engine.h"
#include <string.h>
#include <stdio.h>

// Sprite table: [faction][unit_type]
static sprite_t* unit_sprites[FACTION_COUNT][UNIT_TYPE_COUNT];
static sprite_t* proj_sprites[FACTION_COUNT];
static sprite_t* terrain_sprites[TERRAIN_TYPE_COUNT];

static const char* FACTION_DIRS[FACTION_COUNT] = {
    "dawnguard",
    "ironbone",
    "ashclaw",
    "veilstorm"
};

static const char* TYPE_NAMES[UNIT_TYPE_COUNT] = {
    "scout",
    "warrior",
    "archer",
    "mage",
    "tank",
    "hero",
    "armored", // enemy-only elite variants (UNIT_ARMORED/WARDED/FLYER, factions.h)
    "warded",
    "flyer"
};

// Must match the TerrainType order in terrain.h
static const char* TERRAIN_TILE_NAMES[TERRAIN_TYPE_COUNT] = {
    "tile_grass",
    "tile_water",
    "tile_mountain",
    "tile_desert",
    "tile_snow",
    "tile_lava",
    "tile_path",
};

void resources_init(void) {
    dfs_init(DFS_DEFAULT_LOCATION);

    int loaded = 0, failed = 0;
    char path[64];

    for (int f = 0; f < FACTION_COUNT; f++) {
        for (int t = 0; t < UNIT_TYPE_COUNT; t++) {
            snprintf(path, sizeof(path), "rom:/%s_%s.sprite",
                     FACTION_DIRS[f], TYPE_NAMES[t]);
            unit_sprites[f][t] = sprite_load(path);
            if (unit_sprites[f][t]) loaded++;
            else {
                failed++;
                debugf("MISSING sprite: %s\n", path);
            }
        }

        // Projectile
        snprintf(path, sizeof(path), "rom:/%s_projectile.sprite",
                 FACTION_DIRS[f]);
        proj_sprites[f] = sprite_load(path);
        if (proj_sprites[f]) loaded++;
        else { failed++; debugf("MISSING projectile: %s\n", path); }
    }

    // Terrain tiles (used by terrain_compose() to build the map background)
    for (int t = 0; t < TERRAIN_TYPE_COUNT; t++) {
        snprintf(path, sizeof(path), "rom:/%s.sprite", TERRAIN_TILE_NAMES[t]);
        terrain_sprites[t] = sprite_load(path);
        if (terrain_sprites[t]) loaded++;
        else { failed++; debugf("MISSING terrain tile: %s\n", path); }
    }

    debugf("Resources: %d loaded, %d missing\n", loaded, failed);
}

void resources_free(void) {
    for (int f = 0; f < FACTION_COUNT; f++) {
        for (int t = 0; t < UNIT_TYPE_COUNT; t++) {
            if (unit_sprites[f][t]) sprite_free(unit_sprites[f][t]);
        }
        if (proj_sprites[f]) sprite_free(proj_sprites[f]);
    }
    for (int t = 0; t < TERRAIN_TYPE_COUNT; t++) {
        if (terrain_sprites[t]) sprite_free(terrain_sprites[t]);
    }
}

sprite_t* resources_get_unit_sprite(FactionId f, UnitType t) {
    if (f < 0 || f >= FACTION_COUNT) return NULL;
    if (t < 0 || t >= UNIT_TYPE_COUNT) return NULL;
    return unit_sprites[f][t];
}

sprite_t* resources_get_projectile_sprite(FactionId f) {
    if (f < 0 || f >= FACTION_COUNT) return NULL;
    return proj_sprites[f];
}

sprite_t* resources_get_terrain_sprite(TerrainType t) {
    if (t < 0 || t >= TERRAIN_TYPE_COUNT) return NULL;
    return terrain_sprites[t];
}

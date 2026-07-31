#include "maps.h"
#include "../core/engine.h"
#include <string.h>

// Wave composition per map: how many of each unit type appear
// Format: [UNIT_SCOUT, UNIT_WARRIOR, UNIT_ARCHER, UNIT_MAGE, UNIT_TANK]
// (UNIT_HERO is spawned separately as a boss)
typedef struct {
    int base_counts[UNIT_TYPE_COUNT - 1]; // No hero in waves
    int wave_scale;   // +N units total per extra wave
} WaveTemplate;

static void fill_grass(TerrainMap* m) {
    terrain_init(m, TERRAIN_GRASS);
}
static void fill_desert(TerrainMap* m) {
    terrain_init(m, TERRAIN_DESERT);
    // Oasis patches
    for (int y=4;y<7;y++) for(int x=8;x<12;x++) terrain_set(m,x,y,TERRAIN_WATER);
    for (int y=9;y<12;y++) for(int x=14;x<18;x++) terrain_set(m,x,y,TERRAIN_WATER);
}
static void fill_frozen(TerrainMap* m) {
    for (int y=0;y<SCREEN_HEIGHT/TERRAIN_GRID_SIZE;y++)
        for(int x=0;x<SCREEN_WIDTH/TERRAIN_GRID_SIZE;x++)
            terrain_set(m,x,y, (x+y)%2==0 ? TERRAIN_SNOW : TERRAIN_MOUNTAIN);
}
static void fill_volcanic(TerrainMap* m) {
    terrain_init(m, TERRAIN_MOUNTAIN);
    // Lava rivers as water (slowing)
    for (int x=5;x<15;x++) terrain_set(m,x,4,TERRAIN_WATER);
    for (int x=8;x<18;x++) terrain_set(m,x,10,TERRAIN_WATER);
    for (int x=3;x<12;x++) terrain_set(m,x,13,TERRAIN_WATER);
}

void map_load(MapData* map, int id) {
    switch (id) {

        case 0:  // Greenfield
            strcpy(map->name, "Greenfield");
            map->difficulty    = 1;
            map->starting_gold = 200;
            map->starting_lives= 20;
            path_init_curve(&map->runner_path);
            fill_grass(&map->terrain);
            terrain_compose(&map->terrain);
            audio_play_bgm("grass");
            break;

        case 1:  // Desert Crossing
            strcpy(map->name, "Desert Crossing");
            map->difficulty    = 2;
            map->starting_gold = 175;
            map->starting_lives= 18;
            path_init_zigzag(&map->runner_path);
            fill_desert(&map->terrain);
            terrain_compose(&map->terrain);
            audio_play_bgm("desert");
            break;

        case 2:  // Frozen Highlands
            strcpy(map->name, "Frozen Highlands");
            map->difficulty    = 3;
            map->starting_gold = 160;
            map->starting_lives= 15;
            path_init_spiral(&map->runner_path);
            fill_frozen(&map->terrain);
            terrain_compose(&map->terrain);
            audio_play_bgm("snow");
            break;

        case 3:  // Volcanic Pass
            strcpy(map->name, "Volcanic Pass");
            map->difficulty    = 4;
            map->starting_gold = 150;
            map->starting_lives= 12;
            path_init_straight(&map->runner_path);
            fill_volcanic(&map->terrain);
            terrain_compose(&map->terrain);
            audio_play_bgm("volcano");
            break;

        default:
            map_load(map, 0);
            break;
    }
}

void map_load_custom(MapData* map, const CustomMapSave* saved) {
    strcpy(map->name, "Custom Map");
    map->difficulty     = saved->difficulty > 0 ? saved->difficulty : 1;
    map->starting_gold  = saved->starting_gold;
    map->starting_lives = saved->starting_lives;

    switch (saved->path_type) {
        case 1:  path_init_zigzag(&map->runner_path);   break;
        case 2:  path_init_spiral(&map->runner_path);   break;
        case 3:  path_init_straight(&map->runner_path); break;
        default: path_init_curve(&map->runner_path);    break;
    }

    for (int y = 0; y < SAVE_GRID_H; y++) {
        for (int x = 0; x < SAVE_GRID_W; x++) {
            terrain_set(&map->terrain, x, y, (TerrainType)saved->grid[y * SAVE_GRID_W + x]);
        }
    }
    terrain_compose(&map->terrain);

    // No dedicated BGM for custom maps yet — whatever was already playing keeps going.
}

int map_get_count(void) { return 4; }

const char* map_get_name(int id) {
    switch(id) {
        case 0: return "Greenfield";
        case 1: return "Desert Crossing";
        case 2: return "Frozen Highlands";
        case 3: return "Volcanic Pass";
        default: return "Unknown";
    }
}

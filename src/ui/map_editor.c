#include "map_editor.h"
#include "../core/engine.h"
#include <string.h>
#include <stdio.h>

// =============================================================================
// MAP EDITOR
//
// Reuses the same cursor pattern as tower placement in-game (D-pad moves, A
// confirms on the cell) and the same L/R cycling pattern already used for
// unit types, applied here to terrain types. terrain_compose()/
// terrain_render() (world/terrain.c) already do all the visual preview work
// — the editor only paints the logical grid.
//
// Besides terrain, the editor lets you configure everything a custom map
// needs to play a full match: path shape, attacking faction, a difficulty
// label, starting gold and starting lives. Since that is more settings than
// there are spare buttons, Z cycles which setting C-up/C-down currently
// adjusts (see EditorTab below) instead of giving every setting its own
// button.
// =============================================================================

typedef enum {
    TAB_PATH,
    TAB_ENEMY,
    TAB_DIFFICULTY,
    TAB_GOLD,
    TAB_LIVES,
    TAB_COUNT
} EditorTab;

static TerrainMap editing_terrain;
static int        editing_path_type;   // 0=curve 1=zigzag 2=spiral 3=straight
static int        editing_enemy;       // FactionId attacking
static int        editing_difficulty;  // 1-5, informational label
static int        editing_gold;
static int        editing_lives;
static int        target_slot;         // -1 = new map, no slot assigned yet
static int        cursor_gx, cursor_gy;
static int        brush_type;          // current TerrainType
static EditorTab  current_tab;

static const StringId TERRAIN_STR[TERRAIN_TYPE_COUNT] = {
    STR_TERRAIN_GRASS, STR_TERRAIN_WATER, STR_TERRAIN_MOUNTAIN, STR_TERRAIN_DESERT, STR_TERRAIN_SNOW
};
static const StringId PATH_STR[4] = { STR_PATH_CURVE, STR_PATH_ZIGZAG, STR_PATH_SPIRAL, STR_PATH_STRAIGHT };

static void me_fill(int x, int y, int w, int h, color_t c) {
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(c);
    rdpq_fill_rectangle(x, y, x+w, y+h);
}

void map_editor_enter(int slot) {
    target_slot = slot;
    cursor_gx = SAVE_GRID_W / 2;
    cursor_gy = SAVE_GRID_H / 2;
    brush_type = TERRAIN_GRASS;
    current_tab = TAB_PATH;

    CustomMapSave loaded;
    if (slot >= 0 && save_read_custom_map(slot, &loaded) && loaded.used) {
        editing_path_type  = loaded.path_type;
        editing_enemy       = loaded.enemy_faction;
        editing_difficulty  = loaded.difficulty > 0 ? loaded.difficulty : 1;
        editing_gold        = loaded.starting_gold;
        editing_lives       = loaded.starting_lives;
        for (int y = 0; y < SAVE_GRID_H; y++)
            for (int x = 0; x < SAVE_GRID_W; x++)
                terrain_set(&editing_terrain, x, y, (TerrainType)loaded.grid[y*SAVE_GRID_W+x]);
    } else {
        editing_path_type  = 0;
        editing_enemy       = FACTION_IRONBONE; // any default works; player picks their own faction later
        editing_difficulty  = 1;
        editing_gold        = 200;
        editing_lives       = 20;
        terrain_init(&editing_terrain, TERRAIN_GRASS);
    }
    terrain_compose(&editing_terrain);
}

static void editing_to_save(CustomMapSave* out) {
    memset(out, 0, sizeof(*out));
    for (int y = 0; y < SAVE_GRID_H; y++) {
        for (int x = 0; x < SAVE_GRID_W; x++) {
            out->grid[y*SAVE_GRID_W+x] =
                (uint8_t)terrain_get(&editing_terrain, x*TERRAIN_GRID_SIZE, y*TERRAIN_GRID_SIZE);
        }
    }
    out->path_type      = (uint8_t)editing_path_type;
    out->enemy_faction  = (uint8_t)editing_enemy;
    out->difficulty     = (uint8_t)editing_difficulty;
    out->starting_gold  = (uint16_t)editing_gold;
    out->starting_lives = (uint8_t)editing_lives;
    out->used = 1;
}

// Applies a C-up (+1) or C-down (-1) step to whichever setting `current_tab`
// points at, wrapping/clamping as appropriate for that setting.
static void adjust_current_tab(int dir) {
    switch (current_tab) {
        case TAB_PATH:
            editing_path_type = (editing_path_type + dir + 4) % 4;
            break;
        case TAB_ENEMY:
            editing_enemy = (editing_enemy + dir + FACTION_COUNT) % FACTION_COUNT;
            break;
        case TAB_DIFFICULTY:
            editing_difficulty += dir;
            if (editing_difficulty < 1) editing_difficulty = 1;
            if (editing_difficulty > 5) editing_difficulty = 5;
            break;
        case TAB_GOLD:
            editing_gold += dir * 50;
            if (editing_gold < 100) editing_gold = 500;
            if (editing_gold > 500) editing_gold = 100;
            break;
        case TAB_LIVES:
            editing_lives += dir * 5;
            if (editing_lives < 5)  editing_lives = 40;
            if (editing_lives > 40) editing_lives = 5;
            break;
        default: break;
    }
}

void map_editor_handle_input(GameState* game) {
    joypad_buttons_t kd = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if (kd.d_up)    { cursor_gy--; if (cursor_gy < 0) cursor_gy = 0; }
    if (kd.d_down)  { cursor_gy++; if (cursor_gy > SAVE_GRID_H-1) cursor_gy = SAVE_GRID_H-1; }
    if (kd.d_left)  { cursor_gx--; if (cursor_gx < 0) cursor_gx = 0; }
    if (kd.d_right) { cursor_gx++; if (cursor_gx > SAVE_GRID_W-1) cursor_gx = SAVE_GRID_W-1; }

    if (kd.l) { brush_type--; if (brush_type < 0) brush_type = TERRAIN_TYPE_COUNT-1; }
    if (kd.r) { brush_type++; if (brush_type >= TERRAIN_TYPE_COUNT) brush_type = 0; }

    if (kd.z) { current_tab = (EditorTab)((current_tab + 1) % TAB_COUNT); }
    if (kd.c_up)   adjust_current_tab(+1);
    if (kd.c_down) adjust_current_tab(-1);

    if (kd.a) {
        terrain_set(&editing_terrain, cursor_gx, cursor_gy, (TerrainType)brush_type);
        terrain_compose(&editing_terrain);
    }

    if (kd.start) {
        CustomMapSave data;
        editing_to_save(&data);
        int slot = (target_slot >= 0) ? target_slot : 0;
        if (save_write_custom_map(slot, &data)) {
            game->flow = STATE_CUSTOM_MAP_MENU;
        }
        // On failure (no Controller Pak ready, no space, etc.) we stay in
        // the editor — the HUD already shows the pak status.
    }

    if (kd.c_right) {
        // Play without saving: reuses the existing faction select screen
        // via the pending_custom_map bridge.
        editing_to_save(&game->pending_custom_map);
        game->has_pending_custom_map = true;
        game->flow = STATE_FACTION_SELECT;
    }

    if (kd.b) {
        game->flow = STATE_CUSTOM_MAP_MENU;
    }
}

void map_editor_render(const GameState* game) {
    (void)game;

    terrain_render(&editing_terrain);

    // Cursor: highlighted cell
    me_fill(cursor_gx*TERRAIN_GRID_SIZE, cursor_gy*TERRAIN_GRID_SIZE,
            TERRAIN_GRID_SIZE, TERRAIN_GRID_SIZE, RGBA32(255, 255, 100, 110));

    // Top bar: current brush + whichever setting is active on the tab
    me_fill(0, 0, SCREEN_WIDTH, 16, RGBA32(10, 10, 25, 220));
    rdpq_text_printf(NULL, 1, 4, 11, T(STR_EDITOR_TERRAIN_LABEL), T(TERRAIN_STR[brush_type]));

    char tab_buf[32];
    switch (current_tab) {
        case TAB_PATH:       snprintf(tab_buf, sizeof(tab_buf), T(STR_EDITOR_PATH_LABEL), T(PATH_STR[editing_path_type])); break;
        case TAB_ENEMY:      snprintf(tab_buf, sizeof(tab_buf), T(STR_EDITOR_ENEMY_LABEL), FACTION_NAME(editing_enemy)); break;
        case TAB_DIFFICULTY: snprintf(tab_buf, sizeof(tab_buf), T(STR_EDITOR_DIFFICULTY_LABEL), editing_difficulty); break;
        case TAB_GOLD:       snprintf(tab_buf, sizeof(tab_buf), T(STR_EDITOR_GOLD_LABEL), editing_gold); break;
        case TAB_LIVES:      snprintf(tab_buf, sizeof(tab_buf), "HP:%d (Z)", editing_lives); break;
        default: tab_buf[0] = '\0'; break;
    }
    rdpq_text_printf(NULL, 1, 130, 11, "%s", tab_buf);

    if (target_slot >= 0) {
        rdpq_text_printf(NULL, 1, 250, 11, T(STR_EDITOR_SLOT_LABEL), 'A' + target_slot);
    } else {
        rdpq_text_printf(NULL, 1, 250, 11, "%s", T(STR_EDITOR_NEW));
    }

    // Bottom bar: pak status + controls
    me_fill(0, SCREEN_HEIGHT-24, SCREEN_WIDTH, 24, RGBA32(10, 10, 25, 220));
    rdpq_text_printf(NULL, 1, 4, SCREEN_HEIGHT-14, "%s", T(STR_EDITOR_CONTROLS_HINT));

    SaveStatus st = save_system_status();
    const char* st_txt = (st == SAVE_STATUS_READY) ? T(STR_PAK_READY)
                        : (st == SAVE_STATUS_UNFORMATTED) ? T(STR_PAK_UNFORMATTED)
                        : T(STR_PAK_MISSING);
    color_t st_col = (st == SAVE_STATUS_READY) ? RGBA32(100, 255, 100, 255) : RGBA32(255, 150, 100, 255);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(st_col);
    rdpq_text_printf(NULL, 1, 4, SCREEN_HEIGHT-4, "%s", st_txt);
}

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
//
// The path can also be free-form: painting TERRAIN_PATH tiles (cycle the
// brush to it with L/R) and setting the PATH tab to CUSTOM makes
// path_init_from_terrain() (pathfinding.c) trace that painted corridor into
// real waypoints at load time, instead of using one of the 4 fixed shapes.
// =============================================================================

typedef enum {
    TAB_PATH,
    TAB_ENEMY,
    TAB_DIFFICULTY,
    TAB_GOLD,
    TAB_LIVES,
    TAB_COUNT
} EditorTab;

#define PATH_TYPE_COUNT 5 // curve, zigzag, spiral, straight, custom/painted

static TerrainMap editing_terrain;
static int        editing_path_type;   // 0=curve 1=zigzag 2=spiral 3=straight 4=custom
static int        editing_enemy;       // FactionId attacking
static int        editing_difficulty;  // 1-5, informational label
static int        editing_gold;
static int        editing_lives;
static char       editing_name[SAVE_NAME_LEN];
static int        target_slot;         // -1 = new map, no slot assigned yet
static int        cursor_gx, cursor_gy;
static int        brush_type;          // current TerrainType
static EditorTab  current_tab;

// On-screen keyboard for naming the map (C-left to open, see
// handle_naming_input). " " renders as "_" for visibility but still types
// a real space; A-Z/0-9 type themselves. Laid out in a fixed-width grid.
static bool naming_mode;
static int  kb_cursor;
static const char KEYBOARD_CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
#define KEYBOARD_LEN (sizeof(KEYBOARD_CHARS) - 1)
#define KEYBOARD_COLS 8

static const StringId TERRAIN_STR[TERRAIN_TYPE_COUNT] = {
    STR_TERRAIN_GRASS, STR_TERRAIN_WATER, STR_TERRAIN_MOUNTAIN,
    STR_TERRAIN_DESERT, STR_TERRAIN_SNOW, STR_TERRAIN_LAVA, STR_TERRAIN_PATH
};
static const StringId PATH_STR[PATH_TYPE_COUNT] = {
    STR_PATH_CURVE, STR_PATH_ZIGZAG, STR_PATH_SPIRAL, STR_PATH_STRAIGHT, STR_PATH_CUSTOM
};

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
    naming_mode = false;
    kb_cursor = 0;

    CustomMapSave loaded;
    if (slot >= 0 && save_read_custom_map(slot, &loaded) && loaded.used) {
        editing_path_type  = loaded.path_type;
        editing_enemy       = loaded.enemy_faction;
        editing_difficulty  = loaded.difficulty > 0 ? loaded.difficulty : 1;
        editing_gold        = loaded.starting_gold;
        editing_lives       = loaded.starting_lives;
        snprintf(editing_name, sizeof(editing_name), "%s", loaded.name);
        for (int y = 0; y < SAVE_GRID_H; y++)
            for (int x = 0; x < SAVE_GRID_W; x++)
                terrain_set(&editing_terrain, x, y, (TerrainType)loaded.grid[y*SAVE_GRID_W+x]);
    } else {
        editing_path_type  = 0;
        editing_enemy       = FACTION_IRONBONE; // any default works; player picks their own faction later
        editing_difficulty  = 1;
        editing_gold        = 200;
        editing_lives       = 20;
        editing_name[0]     = '\0';
        terrain_init(&editing_terrain, TERRAIN_GRASS);
    }
    terrain_compose(&editing_terrain, SCREEN_WIDTH, SCREEN_HEIGHT);
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
    snprintf(out->name, sizeof(out->name), "%s", editing_name);
    out->used = 1;
}

// Applies a C-up (+1) or C-down (-1) step to whichever setting `current_tab`
// points at, wrapping/clamping as appropriate for that setting.
static void adjust_current_tab(int dir) {
    switch (current_tab) {
        case TAB_PATH:
            editing_path_type = (editing_path_type + dir + PATH_TYPE_COUNT) % PATH_TYPE_COUNT;
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

// While naming_mode is active, every button means something different (see
// the on-screen keyboard in map_editor_render), so it's handled in a fully
// separate branch instead of interleaving checks with the terrain editor.
static void handle_naming_input(joypad_buttons_t kd) {
    if (kd.d_left)  { kb_cursor--; if (kb_cursor < 0) kb_cursor = KEYBOARD_LEN - 1; }
    if (kd.d_right) { kb_cursor++; if (kb_cursor >= (int)KEYBOARD_LEN) kb_cursor = 0; }
    if (kd.d_up)    { kb_cursor -= KEYBOARD_COLS; if (kb_cursor < 0) kb_cursor += KEYBOARD_LEN; }
    if (kd.d_down)  { kb_cursor += KEYBOARD_COLS; if (kb_cursor >= (int)KEYBOARD_LEN) kb_cursor -= KEYBOARD_LEN; }

    if (kd.a) {
        size_t len = strlen(editing_name);
        if (len < sizeof(editing_name) - 1) {
            editing_name[len]     = KEYBOARD_CHARS[kb_cursor];
            editing_name[len + 1] = '\0';
        }
    }
    if (kd.z) {
        size_t len = strlen(editing_name);
        if (len > 0) editing_name[len - 1] = '\0';
    }
    if (kd.b) naming_mode = false;
}

void map_editor_handle_input(GameState* game) {
    joypad_buttons_t kd = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if (naming_mode) { handle_naming_input(kd); return; }

    // Hold L+R and tap B to reset the whole grid to grass - a quick way to
    // start over without repainting every cell by hand. Must be checked
    // before the plain "B exits the editor" handler below so a single B tap
    // doesn't also exit the editor on the same frame this combo fires.
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
    if (held.l && held.r && kd.b) {
        terrain_init(&editing_terrain, TERRAIN_GRASS);
        terrain_compose(&editing_terrain, SCREEN_WIDTH, SCREEN_HEIGHT);
        return;
    }

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
        terrain_compose(&editing_terrain, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    if (kd.c_left) { naming_mode = true; kb_cursor = 0; }

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

static void render_naming(void) {
    me_fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA32(10, 10, 25, 240));

    // Header: current name buffer with a trailing cursor
    me_fill(20, 10, SCREEN_WIDTH-40, 24, RGBA32(30, 30, 70, 255));
    char name_buf[SAVE_NAME_LEN + 2];
    snprintf(name_buf, sizeof(name_buf), "%s_", editing_name);
    rdpq_text_printf(NULL, 1, 30, 27, T(STR_EDITOR_NAME_LABEL), name_buf);

    // On-screen keyboard grid
    int cell_w = SCREEN_WIDTH / KEYBOARD_COLS;
    int cell_h = 24;
    int grid_y = 50;
    for (int i = 0; i < (int)KEYBOARD_LEN; i++) {
        int col = i % KEYBOARD_COLS;
        int row = i / KEYBOARD_COLS;
        int cx = col * cell_w;
        int cy = grid_y + row * cell_h;
        bool sel = (i == kb_cursor);
        me_fill(cx+1, cy+1, cell_w-2, cell_h-2,
                sel ? RGBA32(90, 140, 220, 255) : RGBA32(25, 25, 55, 220));
        char c = KEYBOARD_CHARS[i];
        char label[2] = { (c == ' ') ? '_' : c, '\0' };
        rdpq_text_printf(NULL, 1, cx + cell_w/2 - 3, cy + cell_h/2 + 4, "%s", label);
    }

    me_fill(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, RGBA32(10, 10, 25, 240));
    rdpq_text_printf(NULL, 1, 10, SCREEN_HEIGHT-4, "%s", T(STR_EDITOR_NAMING_HINT));
}

void map_editor_render(const GameState* game) {
    (void)game;

    if (naming_mode) { render_naming(); return; }

    terrain_render(0, 0); // the editor never scrolls — always shows the map from its origin

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

    char slot_buf[24];
    const char* display_name = editing_name[0] ? editing_name : T(STR_EDITOR_NEW);
    if (target_slot >= 0) {
        snprintf(slot_buf, sizeof(slot_buf), "%c:%s", 'A' + target_slot, display_name);
    } else {
        snprintf(slot_buf, sizeof(slot_buf), "*:%s", display_name);
    }
    // Right-anchored: 125px margin from the right edge at design resolution
    rdpq_text_printf(NULL, 1, SCREEN_WIDTH - 125, 11, "%s", slot_buf);

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

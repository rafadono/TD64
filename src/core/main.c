#include <libdragon.h>
#include "engine.h"
#include "../ui/custom_map_menu.h"
#include "../ui/map_editor.h"
#include "../ui/controls_menu.h"

// =============================================================================
// GAME STATE (single global)
// =============================================================================
static GameState game;

// Timing
static uint64_t last_ticks = 0;
static float    fps_accum  = 0;
static int      fps_frames = 0;
static float    current_fps = 60.0f;

// Tower placement grid position (cursor). WORLD grid coordinates (not
// screen-relative) — on a map bigger than one screen, the camera
// auto-scrolls to follow the cursor (see move_cursor_and_scroll_camera()),
// so the cursor can reach anywhere in the map, not just the visible corner.
static int cursor_gx = 5, cursor_gy = 3;

// Same top/bottom HUD clearance the cursor has always kept (1 grid cell at
// the top, 2 at the bottom) — now expressed in pixels so it can be reused
// both as a hard limit at the map's own edges (where the camera can't
// scroll any further to compensate) and as the auto-scroll trigger margin
// everywhere else in a bigger map. No left/right clearance is needed.
#define CURSOR_MARGIN_TOP_PX    (TERRAIN_GRID_SIZE * 1)
#define CURSOR_MARGIN_BOTTOM_PX (TERRAIN_GRID_SIZE * 2)

// =============================================================================
// INPUT — IN GAME
// =============================================================================

// Moves the cursor by (dgx, dgy) grid cells, clamped to the map's own edges
// (with the HUD clearance reserved there too, since the camera can't scroll
// past a map edge to make room for it), then scrolls the camera just enough
// to keep the cursor visible with that same clearance everywhere else.
//
// This replaces manual camera panning entirely: the D-pad is the only input
// needed to explore a map bigger than one screen, so the player's hands
// never have to leave the D-pad+A "build" grip to reach the stick. See the
// controller ergonomics note in Camera (systems/effects.h) for why that
// matters on the N64's 3-handle shape.
static void move_cursor_and_scroll_camera(int dgx, int dgy) {
    int map_gw = game.map.width  / TERRAIN_GRID_SIZE;
    int map_gh = game.map.height / TERRAIN_GRID_SIZE;
    int min_gy = CURSOR_MARGIN_TOP_PX    / TERRAIN_GRID_SIZE;
    int max_gy = map_gh - 1 - (CURSOR_MARGIN_BOTTOM_PX / TERRAIN_GRID_SIZE);

    cursor_gx += dgx;
    cursor_gy += dgy;
    if (cursor_gx < 0) cursor_gx = 0;
    if (cursor_gx > map_gw - 1) cursor_gx = map_gw - 1;
    if (cursor_gy < min_gy) cursor_gy = min_gy;
    if (cursor_gy > max_gy) cursor_gy = max_gy;

    camera_ensure_visible(&game.camera,
        cursor_gx * TERRAIN_GRID_SIZE, cursor_gy * TERRAIN_GRID_SIZE,
        0, 0, CURSOR_MARGIN_TOP_PX, CURSOR_MARGIN_BOTTOM_PX,
        game.map.width, game.map.height);
}

static void handle_play_input(float dt) {
    joypad_buttons_t kd = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Pause — remappable (see systems/controls.h), default Start
    if (action_pressed(kd, ACTION_PAUSE)) {
        game.paused = true;
        game.flow   = STATE_PAUSED;
        return;
    }

    // Cycle unit type to place — remappable, default L = prev, R = next
    if (action_pressed(kd, ACTION_PREV_UNIT)) {
        game.selected_unit_type--;
        if (game.selected_unit_type < 0) game.selected_unit_type = UNIT_TYPE_COUNT - 1;
    }
    if (action_pressed(kd, ACTION_NEXT_UNIT)) {
        game.selected_unit_type++;
        if (game.selected_unit_type >= UNIT_TYPE_COUNT) game.selected_unit_type = 0;
    }

    // Move cursor (and auto-scroll the camera to follow it) — always the
    // D-pad, not remappable (see systems/controls.h for why).
    if (kd.d_up)    move_cursor_and_scroll_camera(0, -1);
    if (kd.d_down)  move_cursor_and_scroll_camera(0, 1);
    if (kd.d_left)  move_cursor_and_scroll_camera(-1, 0);
    if (kd.d_right) move_cursor_and_scroll_camera(1, 0);

    // Place unit — remappable, default A
    if (action_pressed(kd, ACTION_PLACE) && game.selected_unit_type >= 0) {
        UnitType ut = (UnitType)game.selected_unit_type;
        // Cursor is already in WORLD grid coordinates (see
        // move_cursor_and_scroll_camera) — no camera offset needed here.
        float px = cursor_gx * TERRAIN_GRID_SIZE + 4;
        float py = cursor_gy * TERRAIN_GRID_SIZE + 4;
        // PATH tiles are the enemy's route (custom maps with a free-form
        // painted path, see pathfinding.c) - never buildable, same as you
        // wouldn't be able to build in the middle of the road.
        bool blocked = terrain_get(&game.map.terrain, px, py) == TERRAIN_PATH;
        if (!blocked && game_can_place(&game, ut)) {
            Entity* e = unit_spawn_tower(game.player_faction, ut, px, py);
            if (e) {
                game.gold -= game_unit_cost(game.player_faction, ut);
                particles_emit_explosion(px + 8, py + 8,
                                         e->tint, 8);
                debugf("Placed %s at (%d, %d)\n",
                       UNIT_NAME(game.player_faction, ut),
                       cursor_gx, cursor_gy);
            }
        }
    }

    // Cancel selection — remappable, default B
    if (action_pressed(kd, ACTION_CANCEL)) game.selected_unit_type = -1;

    // Upgrade tower under cursor — remappable, default C-right
    if (action_pressed(kd, ACTION_UPGRADE) && game.selected_tower) {
        unit_upgrade(game.selected_tower, &game);
    }

    // Start wave — remappable, default Z
    if (action_pressed(kd, ACTION_SPAWN_WAVE)) {
        game_spawn_wave(&game);
    }
    (void)dt;
}

// =============================================================================
// RENDER — IN GAME
// =============================================================================
static void render_play(void) {
    // 1. Terrain — blits the SCREEN-sized window of the (possibly bigger)
    // composed map starting at the camera's current world position.
    terrain_render((int)game.camera.scroll_x, (int)game.camera.scroll_y);

    // 2. Path overlay (debug) — path points are world-space, so they need
    // the same camera transform as any other world-space render.
    if (debug.show_pathfinding) {
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        const Path* path = &game.map.runner_path;
        for (int i = 0; i < path->count; i++) {
            rdpq_set_prim_color(RGBA32(255, 50, 50, 180));
            float px = path->points[i].x - 3;
            float py = path->points[i].y - 3;
            camera_apply(&game.camera, &px, &py);
            rdpq_fill_rectangle(px, py, px+6, py+6);
        }
    }

    // 3. Cursor (where tower will be placed) — cursor_gx/gy are WORLD grid
    // coordinates, so its draw position needs the camera transform too.
    if (game.selected_unit_type >= 0) {
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        const UnitStats* s = unit_get_stats(game.player_faction,
                                            (UnitType)game.selected_unit_type);
        bool can = s && game.gold >= s->cost;
        rdpq_set_prim_color(can
            ? RGBA32(255, 255, 100, 120)
            : RGBA32(255, 50,  50,  120));
        float cx = cursor_gx * TERRAIN_GRID_SIZE, cy = cursor_gy * TERRAIN_GRID_SIZE;
        camera_apply(&game.camera, &cx, &cy);
        rdpq_fill_rectangle(cx, cy, cx + TERRAIN_GRID_SIZE, cy + TERRAIN_GRID_SIZE);
    }

    // 4. Entities (towers + runners + projectiles)
    entities_render(&game.camera, debug.show_ranges);

    // 5. Effects
    particles_render(&game.camera);
    floating_text_render();

    // 6. HUD
    ui_draw_hud(&game);
    ui_draw_build_panel(&game);
    if (game.selected_unit_type >= 0) ui_draw_unit_tooltip(&game);
}

// A custom resolution_t built directly from SCREEN_WIDTH/SCREEN_HEIGHT
// (screen.h) instead of an independent RESOLUTION_320x240 preset — the two
// used to only match by convention, with nothing enforcing it if either one
// changed on its own. This is the actual video mode; everything else in the
// game (terrain grid, UI layout anchors, cursor bounds) derives from the
// same two macros. `false` = progressive; flip to `true` for one of the
// interlaced 480-line modes if SCREEN_HEIGHT is ever raised to 480.
static const resolution_t GAME_RESOLUTION = { SCREEN_WIDTH, SCREEN_HEIGHT, false };

// =============================================================================
// MAIN LOOP
// =============================================================================
int main(void) {
    // Init hardware
    display_init(GAME_RESOLUTION, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    rdpq_init();
    joypad_init();
    timer_init();

    // Init systems
    resources_init();
    entities_init();
    particles_init();
    floating_text_init();
    audio_system_init();
    sys_debug_init();
    controls_init();

    // Init font
    rdpq_font_t *font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_VAR);
    rdpq_text_register_font(1, font);

    // Start at main menu
    game.flow = STATE_MAIN_MENU;
    game.selected_unit_type = 0;
    game.selected_tower = NULL;
    cursor_gx = 5; cursor_gy = 3;

    last_ticks = get_ticks();

    debugf("Tower Defense — Faction Wars started\n");
    debugf("Controls: L/R = unit type | D-pad = cursor (auto-scrolls the camera\n");
    debugf("          on maps bigger than one screen) | A = place | Z = next wave\n");
    debugf("          Start = pause | B = cancel | C-right = upgrade\n");
    debugf("          C-up = open/close the debug menu\n");

    while (1) {
        audio_update_mixer();

        // Delta time
        uint64_t now = get_ticks();
        float dt = (float)(now - last_ticks) / (float)TICKS_PER_SECOND;
        last_ticks = now;
        if (dt > 0.1f)  dt = 0.1f;
        if (dt < 0.001f) dt = 0.001f;

        // FPS counter
        fps_accum  += dt;
        fps_frames++;
        if (fps_accum >= 1.0f) {
            current_fps = fps_frames / fps_accum;
            fps_accum   = 0; fps_frames = 0;
            debug.current_fps = current_fps;
            debug_update_perf(dt, entities_count_active(), 0);
            if (debug.show_fps) debugf("FPS: %.1f  Entities: %d\n",
                                   current_fps, entities_count_active());
        }

        // Reads controller state once per frame; the rest of the code only
        // queries joypad_get_buttons_* (never polls again).
        joypad_poll();

        // === UPDATE ===
        switch (game.flow) {
            case STATE_PLAYING: {
                debug_handle_input();

                float game_dt = dt;
                if (debug.fast_forward)      game_dt *= 2.0f;
                else if (debug.slow_motion)  game_dt *= 0.5f;

                // Skip normal gameplay input while the debug menu is open —
                // D-pad/A are redirected to it (see debug_handle_input()),
                // so reading them again here would fight over the same
                // press (e.g. moving the cursor AND the menu selection).
                // The simulation keeps running underneath either way.
                if (!debug_menu_is_open()) handle_play_input(game_dt);
                game_update(&game, game_dt);
                break;
            }

            case STATE_MAIN_MENU:
            case STATE_DIFFICULTY_SELECT:
            case STATE_FACTION_SELECT:
            case STATE_PAUSED:
            case STATE_GAME_OVER:
            case STATE_VICTORY:
                menu_handle_input(&game);
                break;

            case STATE_CUSTOM_MAP_MENU:
                custom_map_menu_handle_input(&game);
                break;

            case STATE_MAP_EDITOR:
                map_editor_handle_input(&game);
                break;

            case STATE_CONTROLS_MENU:
                controls_menu_handle_input(&game);
                break;

            default:
                break;
        }

        // === RENDER ===
        surface_t* disp = display_get();
        rdpq_attach(disp, NULL);

        // Clear
        rdpq_set_mode_fill(RGBA32(25, 25, 45, 255));
        rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        if (game.flow == STATE_PLAYING) {
            render_play();
            sys_debug_render(&game);
        } else if (game.flow == STATE_PAUSED) {
            // Redraws the last game frame underneath the pause dialog
            // (previously the world stopped rendering entirely while
            // paused, since render_play() only ran under STATE_PLAYING).
            render_play();
            sys_debug_render(&game);
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            menu_render(&game);
        } else if (game.flow == STATE_MAP_EDITOR) {
            rdpq_set_mode_standard();
            map_editor_render(&game);
        } else if (game.flow == STATE_CUSTOM_MAP_MENU) {
            rdpq_set_mode_standard();
            custom_map_menu_render(&game);
        } else if (game.flow == STATE_CONTROLS_MENU) {
            rdpq_set_mode_standard();
            controls_menu_render(&game);
        } else {
            // All other menu states (main menu, faction select, game over, victory)
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            menu_render(&game);
        }

        // FPS display (debug)
        if (debug.show_fps) {
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            rdpq_set_prim_color(RGBA32(255, 255, 100, 200));
            rdpq_fill_rectangle(SCREEN_WIDTH-42, 0, SCREEN_WIDTH, 12);
        }

        rdpq_detach_show();
    }

    resources_free();
    audio_system_close();
    rdpq_close();
    display_close();
    return 0;
}

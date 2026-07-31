#include <libdragon.h>
#include "engine.h"
#include "../ui/custom_map_menu.h"
#include "../ui/map_editor.h"

// =============================================================================
// GAME STATE (single global)
// =============================================================================
static GameState game;

// Timing
static uint64_t last_ticks = 0;
static float    fps_accum  = 0;
static int      fps_frames = 0;
static float    current_fps = 60.0f;

// Debug
static bool debug_mode = false;

// Tower placement grid position (cursor)
static int cursor_gx = 5, cursor_gy = 3;

#define CURSOR_GY_MIN 1
#define CURSOR_GY_MAX ((SCREEN_HEIGHT / TERRAIN_GRID_SIZE) - 3)  // leaves room for the top/bottom HUD
#define CURSOR_GX_MAX ((SCREEN_WIDTH  / TERRAIN_GRID_SIZE) - 1)

// =============================================================================
// INPUT — IN GAME
// =============================================================================
static void handle_play_input(float dt) {
    joypad_buttons_t kd = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Pause
    if (kd.start) {
        game.paused = true;
        game.flow   = STATE_PAUSED;
        return;
    }

    // Toggle debug
    if (kd.c_up) debug_mode = !debug_mode;

    // Cycle unit type to place  (L = prev, R = next)
    if (kd.l) {
        game.selected_unit_type--;
        if (game.selected_unit_type < 0) game.selected_unit_type = UNIT_TYPE_COUNT - 1;
    }
    if (kd.r) {
        game.selected_unit_type++;
        if (game.selected_unit_type >= UNIT_TYPE_COUNT) game.selected_unit_type = 0;
    }

    // Move cursor
    if (kd.d_up)    { cursor_gy--; if (cursor_gy < CURSOR_GY_MIN) cursor_gy = CURSOR_GY_MIN; }
    if (kd.d_down)  { cursor_gy++; if (cursor_gy > CURSOR_GY_MAX) cursor_gy = CURSOR_GY_MAX; }
    if (kd.d_left)  { cursor_gx--; if (cursor_gx < 0)  cursor_gx = 0; }
    if (kd.d_right) { cursor_gx++; if (cursor_gx > CURSOR_GX_MAX) cursor_gx = CURSOR_GX_MAX; }

    // Place unit
    if (kd.a && game.selected_unit_type >= 0) {
        UnitType ut = (UnitType)game.selected_unit_type;
        if (game_can_place(&game, ut)) {
            float px = cursor_gx * TERRAIN_GRID_SIZE + 4;
            float py = cursor_gy * TERRAIN_GRID_SIZE + 4;
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

    // Cancel selection
    if (kd.b) game.selected_unit_type = -1;

    // Upgrade tower under cursor (C-right)
    if (kd.c_right && game.selected_tower) {
        unit_upgrade(game.selected_tower, &game);
    }

    // Start wave
    if (kd.z) {
        game_spawn_wave(&game);
    }
    (void)dt;
}

// =============================================================================
// RENDER — IN GAME
// =============================================================================
static void render_play(void) {
    // 1. Terrain
    terrain_render(&game.map.terrain);

    // 2. Path overlay (debug)
    if (debug_mode) {
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        const Path* path = &game.map.runner_path;
        for (int i = 0; i < path->count; i++) {
            rdpq_set_prim_color(RGBA32(255, 50, 50, 180));
            float px = path->points[i].x - 3;
            float py = path->points[i].y - 3;
            rdpq_fill_rectangle(px, py, px+6, py+6);
        }
    }

    // 3. Cursor (where tower will be placed)
    if (game.selected_unit_type >= 0) {
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        const UnitStats* s = unit_get_stats(game.player_faction,
                                            (UnitType)game.selected_unit_type);
        bool can = s && game.gold >= s->cost;
        rdpq_set_prim_color(can
            ? RGBA32(255, 255, 100, 120)
            : RGBA32(255, 50,  50,  120));
        rdpq_fill_rectangle(cursor_gx * TERRAIN_GRID_SIZE,
                            cursor_gy * TERRAIN_GRID_SIZE,
                            (cursor_gx+1) * TERRAIN_GRID_SIZE,
                            (cursor_gy+1) * TERRAIN_GRID_SIZE);
    }

    // 4. Entities (towers + runners + projectiles)
    entities_render(&game.camera, debug_mode);

    // 5. Effects
    particles_render();
    floating_text_render();

    // 6. HUD
    ui_draw_hud(&game);
    ui_draw_build_panel(&game);
    if (game.selected_unit_type >= 0) ui_draw_unit_tooltip(&game);
}

// =============================================================================
// MAIN LOOP
// =============================================================================
int main(void) {
    // Init hardware
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);
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
    debugf("Controls: L/R = unit type | D-pad = cursor | A = place | Z = next wave\n");
    debugf("          Start = pause | C-up = debug | B = cancel | C-right = upgrade\n");

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
            if (debug_mode) debugf("FPS: %.1f  Entities: %d\n",
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

                handle_play_input(game_dt);
                game_update(&game, game_dt);
                break;
            }

            case STATE_MAIN_MENU:
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
        } else {
            // All other menu states (main menu, faction select, game over, victory)
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            menu_render(&game);
        }

        // FPS display (debug)
        if (debug_mode) {
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

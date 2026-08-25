#include "../systems/debug.h"
#include "../config/game_config.h"
#include "../core/engine.h"
#include <stdio.h>
#include <stdarg.h>

DebugState debug;

void sys_debug_init(void) {
    // Load defaults from game_config.h
    debug.show_fps             = DEBUG_SHOW_FPS_DEFAULT;
    debug.show_entity_count    = DEBUG_SHOW_ENTITY_COUNT;
    debug.show_memory          = DEBUG_SHOW_MEMORY_STATS;
    debug.show_collision       = DEBUG_SHOW_COLLISION_BOXES;
    debug.show_ranges          = DEBUG_SHOW_RANGE_CIRCLES;
    debug.show_pathfinding     = DEBUG_SHOW_PATHFINDING;
    debug.show_wave_preview    = DEBUG_SHOW_WAVE_PREVIEW;
    debug.show_economy         = DEBUG_SHOW_ECONOMY_INFO;
    debug.show_ai_lines        = DEBUG_SHOW_AI_TARGETING;
    debug.show_performance     = DEBUG_SHOW_PERFORMANCE_TIMERS;
    debug.show_grid            = DEBUG_SHOW_GRID_OVERLAY;
    debug.show_damage_numbers  = DEBUG_SHOW_DAMAGE_NUMBERS;

    debug.godmode          = DEBUG_GODMODE;
    debug.infinite_gold    = DEBUG_INFINITE_GOLD;
    debug.instant_waves    = DEBUG_INSTANT_WAVES;
    debug.slow_motion      = DEBUG_SLOW_MOTION;
    debug.fast_forward     = DEBUG_FAST_FORWARD;
    debug.one_hit_kills    = DEBUG_ONE_HIT_KILLS;

    debug.current_fps      = 60.0f;
    debug.entity_count     = 0;
    debug.particle_count   = 0;
    debug.update_time_ms   = 0.0f;
    debug.render_time_ms   = 0.0f;
    debug.gold_per_second  = 0;
    debug.dps_total        = 0;
}

// =============================================================================
// DEBUG MENU
//
// Every visual overlay AND every cheat lives in one navigable list, opened
// with a single button (C-up) instead of the old scheme of 4 separate
// C-button toggles plus 4 hidden L+R+C-* cheat combos. That old scheme had
// a real bug (C-right doubled as both "toggle performance overlay" and
// "upgrade the selected tower" — both fired on the same press) and it used
// up every C-button + the whole L+R+C-* combo space, leaving nothing free
// for anything else (see the ergonomics discussion that led to this).
// =============================================================================
typedef struct { const char* label; bool* flag; } DebugMenuEntry;

static bool debug_menu_open = false;
static int  debug_menu_sel  = 0;

static DebugMenuEntry DEBUG_MENU_ENTRIES[] = {
    // Visual overlays
    { "FPS Counter",        &debug.show_fps },
    { "Entity Count",       &debug.show_entity_count },
    { "Memory Stats",       &debug.show_memory },
    { "Collision Boxes",    &debug.show_collision },
    { "Range Circles",      &debug.show_ranges },
    { "Pathfinding",        &debug.show_pathfinding },
    { "Wave Preview",       &debug.show_wave_preview },
    { "Economy Info",       &debug.show_economy },
    { "AI Targeting Lines", &debug.show_ai_lines },
    { "Performance Timers", &debug.show_performance },
    { "Grid Overlay",       &debug.show_grid },
    { "Damage Numbers",     &debug.show_damage_numbers },
    // Cheats
    { "Godmode",            &debug.godmode },
    { "Infinite Gold",      &debug.infinite_gold },
    { "Instant Waves",      &debug.instant_waves },
    { "Slow Motion",        &debug.slow_motion },
    { "Fast Forward",       &debug.fast_forward },
    { "One-Hit Kills",      &debug.one_hit_kills },
};
#define DEBUG_MENU_COUNT ((int)(sizeof(DEBUG_MENU_ENTRIES) / sizeof(DEBUG_MENU_ENTRIES[0])))

bool debug_menu_is_open(void) { return debug_menu_open; }

void debug_handle_input(void) {
    joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if (keys.c_up) {
        debug_menu_open = !debug_menu_open;
        debug_menu_sel  = 0;
        return; // don't also read D-pad/A/B below on the same press that opened/closed it
    }
    if (!debug_menu_open) return;

    if (keys.d_up)   { debug_menu_sel--; if (debug_menu_sel < 0) debug_menu_sel = DEBUG_MENU_COUNT - 1; }
    if (keys.d_down) { debug_menu_sel++; if (debug_menu_sel >= DEBUG_MENU_COUNT) debug_menu_sel = 0; }

    if (keys.a) {
        bool* flag = DEBUG_MENU_ENTRIES[debug_menu_sel].flag;
        *flag = !*flag;
        // Fast forward and slow motion are mutually exclusive (same as the
        // old L+R+C-right cheat used to enforce).
        if (flag == &debug.fast_forward && debug.fast_forward) debug.slow_motion = false;
        if (flag == &debug.slow_motion  && debug.slow_motion)  debug.fast_forward = false;
    }

    if (keys.b) debug_menu_open = false;
}

void debug_update_perf(float dt, int entities, int particles) {
    debug.entity_count = entities;
    debug.particle_count = particles;
    
    // FPS is calculated externally in main loop
    // update_time_ms and render_time_ms set externally with timer
}

// Render debug overlay
void sys_debug_render(const GameState* game) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    int y = 2;  // Current Y position for text

    // ── FPS ────────────────────────────────────────────────────────────────
    if (debug.show_fps) {
        char fps_str[32];
        snprintf(fps_str, sizeof(fps_str), "FPS: %.1f", debug.current_fps);
        
        // Background box
        rdpq_set_prim_color(RGBA32(0, 0, 0, 180));
        rdpq_fill_rectangle(2, y, 70, y+12);
        
        // FPS text (as colored blocks — no font system yet)
        rdpq_set_prim_color(DEBUG_COLOR_FPS);
        rdpq_fill_rectangle(6, y+3, 66, y+9);
        
        y += 14;
    }

    // ── Entity Count ───────────────────────────────────────────────────────
    if (debug.show_entity_count) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 180));
        rdpq_fill_rectangle(2, y, 90, y+12);
        rdpq_set_prim_color(RGBA32(100, 200, 255, 255));
        rdpq_fill_rectangle(6, y+3, 86, y+9);
        y += 14;
    }

    // ── Memory Stats ───────────────────────────────────────────────────────
    if (debug.show_memory) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 180));
        rdpq_fill_rectangle(2, y, 110, y+12);
        
        // Entity pool bar
        float entity_pct = (float)debug.entity_count / MAX_ENTITIES;
        color_t bar_col = entity_pct > 0.9f ? RGBA32(255,50,50,255)
                        : entity_pct > 0.7f ? RGBA32(255,200,50,255)
                                            : RGBA32(50,255,100,255);
        int bar_w = (int)(100 * entity_pct);
        rdpq_set_prim_color(RGBA32(30, 30, 50, 255));
        rdpq_fill_rectangle(6, y+3, 106, y+9);
        rdpq_set_prim_color(bar_col);
        rdpq_fill_rectangle(6, y+3, 6+bar_w, y+9);
        
        y += 14;
    }

    // ── Economy Info ───────────────────────────────────────────────────────
    if (debug.show_economy) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 180));
        rdpq_fill_rectangle(2, y, 120, y+26);
        
        // Gold
        rdpq_set_prim_color(RGBA32(255, 215, 0, 255));
        rdpq_fill_rectangle(6, y+3, 50, y+9);
        
        // Gold/sec rate (placeholder visualization)
        int gps_bar = debug.gold_per_second / 2;  // Scale down for display
        if (gps_bar > 100) gps_bar = 100;
        rdpq_set_prim_color(RGBA32(200, 255, 100, 255));
        rdpq_fill_rectangle(55, y+3, 55+gps_bar, y+9);
        
        // DPS total
        rdpq_set_prim_color(RGBA32(255, 100, 50, 255));
        rdpq_fill_rectangle(6, y+14, 50, y+20);
        int dps_bar = debug.dps_total / 5;
        if (dps_bar > 100) dps_bar = 100;
        rdpq_set_prim_color(RGBA32(255, 150, 100, 255));
        rdpq_fill_rectangle(55, y+14, 55+dps_bar, y+20);
        
        y += 28;
    }

    // ── Performance Timers ─────────────────────────────────────────────────
    if (debug.show_performance) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 180));
        rdpq_fill_rectangle(2, y, 100, y+26);
        
        // Update time bar (target: <5ms)
        rdpq_set_prim_color(RGBA32(100, 100, 120, 255));
        rdpq_fill_rectangle(6, y+3, 96, y+9);
        int upd_bar = (int)(debug.update_time_ms * 10);
        if (upd_bar > 90) upd_bar = 90;
        color_t upd_col = debug.update_time_ms > 5.0f ? RGBA32(255,50,50,255)
                                                      : RGBA32(100,255,100,255);
        rdpq_set_prim_color(upd_col);
        rdpq_fill_rectangle(6, y+3, 6+upd_bar, y+9);
        
        // Render time bar (target: <11ms for 60fps)
        rdpq_set_prim_color(RGBA32(100, 100, 120, 255));
        rdpq_fill_rectangle(6, y+14, 96, y+20);
        int rnd_bar = (int)(debug.render_time_ms * 8);
        if (rnd_bar > 90) rnd_bar = 90;
        color_t rnd_col = debug.render_time_ms > 11.0f ? RGBA32(255,50,50,255)
                                                        : RGBA32(100,255,100,255);
        rdpq_set_prim_color(rnd_col);
        rdpq_fill_rectangle(6, y+14, 6+rnd_bar, y+20);
        
        y += 28;
    }

    // ── Wave Preview ───────────────────────────────────────────────────────
    if (debug.show_wave_preview && game) {
        // Show composition of next wave
        rdpq_set_prim_color(RGBA32(0, 0, 0, 180));
        rdpq_fill_rectangle(SCREEN_WIDTH - 82, 2, SCREEN_WIDTH - 2, 46);
        rdpq_set_prim_color(RGBA32(200, 200, 255, 220));
        rdpq_fill_rectangle(SCREEN_WIDTH - 78, 6, SCREEN_WIDTH - 6, 16);
        
        // 6 unit type bars (simplified visualization)
        static const color_t UNIT_COLS[6] = {
            RGBA32(100,200,255,255),  // Scout - blue
            RGBA32(220,100,50,255),   // Warrior - orange
            RGBA32(100,255,100,255),  // Archer - green
            RGBA32(200,100,255,255),  // Mage - purple
            RGBA32(150,150,180,255),  // Tank - gray
            RGBA32(255,215,0,255),    // Hero - gold
        };
        for (int i = 0; i < 6; i++) {
            int bx = SCREEN_WIDTH - 76 + i*12;
            rdpq_set_prim_color(UNIT_COLS[i]);
            rdpq_fill_rectangle(bx, 19, bx+10, 42);
            // Count as height (placeholder — would calculate actual wave comp)
            int count_height = 4 + i*3;  // Mock data
            rdpq_set_prim_color(RGBA32(255, 255, 255, 100));
            rdpq_fill_rectangle(bx+2, 42-count_height, bx+8, 42);
        }
    }

    // ── Active Cheats Indicator ────────────────────────────────────────────
    int cheat_count = 0;
    if (debug.godmode)        cheat_count++;
    if (debug.infinite_gold)  cheat_count++;
    if (debug.one_hit_kills)  cheat_count++;
    if (debug.fast_forward)   cheat_count++;
    if (debug.slow_motion)    cheat_count++;

    if (cheat_count > 0) {
        // Pulsing "CHEATS ACTIVE" indicator in top-right
        static float pulse = 0.0f;
        pulse += 0.05f;
        if (pulse > 6.28f) pulse = 0.0f;
        uint8_t alpha = (uint8_t)(180 + 75 * sinf(pulse));
        
        rdpq_set_prim_color(RGBA32(255, 50, 50, alpha));
        rdpq_fill_rectangle(SCREEN_WIDTH - 62, SCREEN_HEIGHT - 18,
                            SCREEN_WIDTH - 2,  SCREEN_HEIGHT - 2);
        rdpq_set_prim_color(RGBA32(255, 255, 100, 255));
        rdpq_fill_rectangle(SCREEN_WIDTH - 58, SCREEN_HEIGHT - 14,
                            SCREEN_WIDTH - 6,  SCREEN_HEIGHT - 6);
    }

    // ── Debug Menu ────────────────────────────────────────────────────────
    // Drawn last so it sits on top of every other overlay.
    if (debug_menu_open) {
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(10, 10, 20, 235));
        rdpq_fill_rectangle(SCREEN_WIDTH/2 - 90, 10, SCREEN_WIDTH/2 + 90, SCREEN_HEIGHT - 10);

        rdpq_set_prim_color(RGBA32(60, 60, 120, 255));
        rdpq_fill_rectangle(SCREEN_WIDTH/2 - 90, 10, SCREEN_WIDTH/2 + 90, 22);
        rdpq_text_printf(NULL, 1, SCREEN_WIDTH/2 - 82, 20, "DEBUG MENU");

        int row_h = (SCREEN_HEIGHT - 34) / DEBUG_MENU_COUNT;
        if (row_h > 11) row_h = 11; // don't let rows get taller than needed on a bigger screen
        int start_y = 24;
        for (int i = 0; i < DEBUG_MENU_COUNT; i++) {
            int ry = start_y + i * row_h;
            bool sel = (i == debug_menu_sel);
            bool on  = *DEBUG_MENU_ENTRIES[i].flag;

            if (sel) {
                rdpq_set_prim_color(RGBA32(50, 50, 100, 255));
                rdpq_fill_rectangle(SCREEN_WIDTH/2 - 88, ry, SCREEN_WIDTH/2 + 88, ry + row_h - 1);
            }
            rdpq_set_prim_color(on ? RGBA32(100, 255, 130, 255) : RGBA32(120, 120, 130, 255));
            rdpq_text_printf(NULL, 1, SCREEN_WIDTH/2 - 84, ry + row_h - 2, "[%s]", on ? "X" : " ");
            rdpq_set_prim_color(RGBA32(230, 230, 230, 255));
            rdpq_text_printf(NULL, 1, SCREEN_WIDTH/2 - 68, ry + row_h - 2, "%s",
                              DEBUG_MENU_ENTRIES[i].label);
        }

        rdpq_set_prim_color(RGBA32(180, 180, 200, 255));
        rdpq_text_printf(NULL, 1, SCREEN_WIDTH/2 - 82, SCREEN_HEIGHT - 14,
                          "D-PAD SELECT  A TOGGLE  B/C-UP CLOSE");
    }
}

void debug_log(const char* fmt, ...) {
    #ifndef NDEBUG
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    debugf("%s", buffer);
    #else
    (void)fmt;  // Suppress unused warning in release
    #endif
}

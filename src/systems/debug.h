#ifndef DEBUG_H
#define DEBUG_H

#include <libdragon.h>
#include <stdbool.h>

// =============================================================================
// DEBUG SYSTEM
// Controls to toggle debug features at runtime
// =============================================================================

typedef struct {
    // Visual overlays
    bool show_fps;              // FPS counter
    bool show_entity_count;     // Entity count
    bool show_memory;           // Memory pool usage
    bool show_collision;        // Hitboxes
    bool show_ranges;           // Tower range circles
    bool show_pathfinding;      // Enemy paths
    bool show_wave_preview;     // Next wave composition
    bool show_economy;          // Gold/sec, DPS
    bool show_ai_lines;         // Targeting lines
    bool show_performance;      // Frame timing
    bool show_grid;             // Terrain grid
    bool show_damage_numbers;   // Always useful

    // Gameplay cheats
    bool godmode;               // Infinite lives
    bool infinite_gold;         // Infinite money
    bool instant_waves;         // No spawn delay
    bool slow_motion;           // 0.5x speed
    bool fast_forward;          // 2.0x speed
    bool one_hit_kills;         // Towers insta-kill

    // Performance stats (updated each frame)
    float   current_fps;
    int     entity_count;
    int     particle_count;
    float   update_time_ms;
    float   render_time_ms;
    int     gold_per_second;
    int     dps_total;

} DebugState;

// Global debug state
extern DebugState debug;

struct GameState;

// Initialize debug system
void sys_debug_init(void);

// Handle debug input: C-up opens/closes the debug menu (D-pad selects,
// A toggles, B also closes) — this is the ONLY debug input, replacing the
// old scattered C-down/C-left/C-right toggles and the L+R+C-* cheat combos.
// While the menu is open, the caller (core/main.c) should skip normal
// gameplay input (cursor/placement/etc) so D-pad/A aren't double-purposed.
void debug_handle_input(void);

// True while the debug menu is open (see debug_handle_input).
bool debug_menu_is_open(void);

// Render debug overlay (the per-flag overlays, e.g. FPS counter/grid/etc,
// plus the menu itself when open)
void sys_debug_render(const struct GameState* game);

// Update performance counters
void debug_update_perf(float dt, int entities, int particles);

// Log message to debug console (if available)
void debug_log(const char* fmt, ...);

#endif // DEBUG_H

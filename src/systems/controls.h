#ifndef CONTROLS_H
#define CONTROLS_H
#include <libdragon.h>
#include <stdbool.h>

// =============================================================================
// CONTROLS — remappable button bindings for in-game actions
//
// Only non-movement actions are remappable (see GameAction below) — the
// D-pad stays fixed as the build cursor, same as virtually every game with
// rebindable controls keeps movement on a fixed stick/pad and only remaps
// action buttons. The menu screens themselves (including this one) are
// also not remappable, only what happens during STATE_PLAYING.
// =============================================================================

// Every physical button that can be assigned to an action.
typedef enum {
    BTN_A, BTN_B, BTN_Z, BTN_START, BTN_L, BTN_R,
    BTN_D_UP, BTN_D_DOWN, BTN_D_LEFT, BTN_D_RIGHT,
    BTN_C_UP, BTN_C_DOWN, BTN_C_LEFT, BTN_C_RIGHT,
    BTN_COUNT
} PhysicalButton;

// Every action that can be rebound, with its default binding noted.
typedef enum {
    ACTION_PLACE,       // default: A
    ACTION_CANCEL,      // default: B
    ACTION_PREV_UNIT,   // default: L
    ACTION_NEXT_UNIT,   // default: R
    ACTION_UPGRADE,     // default: C-right
    ACTION_SPAWN_WAVE,  // default: Z
    ACTION_PAUSE,       // default: Start
    ACTION_COUNT
} GameAction;

// Plain button labels — these are universal Nintendo controller labels, not
// translated strings (same as a faction's proper-noun name in factions.h).
extern const char* const PHYSICAL_BUTTON_NAMES[BTN_COUNT];

// Loads the default bindings, then overrides them from the Controller Pak
// if a valid saved config is found there. Call once at boot.
void controls_init(void);

// Restores every action to its default binding (in memory only — the
// player still has to save from the Controls screen to persist it).
void controls_reset_defaults(void);

PhysicalButton controls_get_binding(GameAction action);
void controls_set_binding(GameAction action, PhysicalButton button);

// Explicit save (mirrors save_write_progress()'s "explicit, not automatic"
// convention) — call from the Controls screen, not on every change.
bool controls_save_to_pak(void);

// Extracts whether `btn` is set in a joypad_buttons_t — pass whatever the
// caller polled (joypad_get_buttons_pressed/held) to check that edge.
bool button_matches(joypad_buttons_t buttons, PhysicalButton btn);

// Convenience: was the button currently bound to `action` just pressed?
// (always checks the press-edge, i.e. pass joypad_get_buttons_pressed()'s
// result — none of the 7 remappable actions need "held" semantics.)
bool action_pressed(joypad_buttons_t buttons_pressed, GameAction action);

#endif // CONTROLS_H

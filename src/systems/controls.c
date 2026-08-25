#include "controls.h"
#include "save.h"
#include <string.h>

const char* const PHYSICAL_BUTTON_NAMES[BTN_COUNT] = {
    "A", "B", "Z", "START", "L", "R",
    "D-UP", "D-DOWN", "D-LEFT", "D-RIGHT",
    "C-UP", "C-DOWN", "C-LEFT", "C-RIGHT",
};

static const PhysicalButton DEFAULT_BINDINGS[ACTION_COUNT] = {
    [ACTION_PLACE]      = BTN_A,
    [ACTION_CANCEL]     = BTN_B,
    [ACTION_PREV_UNIT]  = BTN_L,
    [ACTION_NEXT_UNIT]  = BTN_R,
    [ACTION_UPGRADE]    = BTN_C_RIGHT,
    [ACTION_SPAWN_WAVE] = BTN_Z,
    [ACTION_PAUSE]      = BTN_START,
};

static PhysicalButton bindings[ACTION_COUNT];

void controls_reset_defaults(void) {
    memcpy(bindings, DEFAULT_BINDINGS, sizeof(bindings));
}

void controls_init(void) {
    controls_reset_defaults();

    // Controller Pak access does real I/O — only worth it once at boot,
    // and only if a pak happens to already be ready (we don't want to
    // force-detect/validate here; save_system_check() gets called anyway
    // the first time the player opens a pak-backed screen).
    if (save_system_check() != SAVE_STATUS_READY) return;

    InputConfig cfg;
    if (save_read_input_config(&cfg) && cfg.valid) {
        for (int i = 0; i < ACTION_COUNT; i++) {
            if (cfg.bindings[i] < BTN_COUNT) bindings[i] = (PhysicalButton)cfg.bindings[i];
        }
    }
}

PhysicalButton controls_get_binding(GameAction action) {
    if (action < 0 || action >= ACTION_COUNT) return BTN_A;
    return bindings[action];
}

void controls_set_binding(GameAction action, PhysicalButton button) {
    if (action < 0 || action >= ACTION_COUNT) return;
    if (button < 0 || button >= BTN_COUNT) return;
    bindings[action] = button;
}

bool controls_save_to_pak(void) {
    InputConfig cfg;
    cfg.valid = 1;
    for (int i = 0; i < ACTION_COUNT; i++) cfg.bindings[i] = (uint8_t)bindings[i];
    return save_write_input_config(&cfg);
}

bool button_matches(joypad_buttons_t buttons, PhysicalButton btn) {
    switch (btn) {
        case BTN_A:       return buttons.a;
        case BTN_B:       return buttons.b;
        case BTN_Z:       return buttons.z;
        case BTN_START:   return buttons.start;
        case BTN_L:       return buttons.l;
        case BTN_R:       return buttons.r;
        case BTN_D_UP:    return buttons.d_up;
        case BTN_D_DOWN:  return buttons.d_down;
        case BTN_D_LEFT:  return buttons.d_left;
        case BTN_D_RIGHT: return buttons.d_right;
        case BTN_C_UP:    return buttons.c_up;
        case BTN_C_DOWN:  return buttons.c_down;
        case BTN_C_LEFT:  return buttons.c_left;
        case BTN_C_RIGHT: return buttons.c_right;
        default:          return false;
    }
}

bool action_pressed(joypad_buttons_t buttons_pressed, GameAction action) {
    return button_matches(buttons_pressed, controls_get_binding(action));
}

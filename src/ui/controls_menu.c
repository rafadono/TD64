#include "controls_menu.h"
#include "../core/engine.h"
#include <stdio.h>

// =============================================================================
// CONTROLS MENU — rebind the 7 in-game actions (D-pad movement stays fixed,
// see systems/controls.h for why).
//
// D-pad up/down = select an action | A = capture the next button press as
// its new binding (B cancels the capture without changing anything) |
// Z = reset every binding to its default | Start = save to the Controller
// Pak (explicit, like every other pak write in this game) | B = back.
// =============================================================================

static int  sel = 0;
static bool waiting_for_button = false;
static float saved_flash_timer = 0.0f; // >0 while the "SAVED" confirmation is shown

static const StringId ACTION_STR[ACTION_COUNT] = {
    STR_ACTION_PLACE, STR_ACTION_CANCEL, STR_ACTION_PREV_UNIT, STR_ACTION_NEXT_UNIT,
    STR_ACTION_UPGRADE, STR_ACTION_SPAWN_WAVE, STR_ACTION_PAUSE,
};

void controls_menu_enter(void) {
    sel = 0;
    waiting_for_button = false;
    saved_flash_timer = 0.0f;
}

void controls_menu_handle_input(GameState* game) {
    joypad_buttons_t kd = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if (saved_flash_timer > 0.0f) saved_flash_timer -= 1.0f/60.0f;

    if (waiting_for_button) {
        if (kd.b) { waiting_for_button = false; return; } // cancel, keep the old binding
        for (int b = 0; b < BTN_COUNT; b++) {
            // C-up is hardwired to open/close the debug menu (see
            // debug_handle_input()) and isn't itself remappable — binding
            // an action to it would make that action unreachable whenever
            // the debug menu is open, so it's simply not offered here.
            if (b == BTN_C_UP) continue;
            if (button_matches(kd, (PhysicalButton)b)) {
                controls_set_binding((GameAction)sel, (PhysicalButton)b);
                waiting_for_button = false;
                return;
            }
        }
        return; // no button pressed yet this frame — keep waiting
    }

    if (kd.d_up)   { sel--; if (sel < 0) sel = ACTION_COUNT - 1; }
    if (kd.d_down) { sel++; if (sel >= ACTION_COUNT) sel = 0; }
    if (kd.a)      { waiting_for_button = true; }
    if (kd.z)      { controls_reset_defaults(); }
    if (kd.start)  { if (controls_save_to_pak()) saved_flash_timer = 1.5f; }
    if (kd.b)      { game->flow = STATE_MAIN_MENU; }
}

void controls_menu_render(const GameState* game) {
    (void)game;

    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(10, 10, 22, 255));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    rdpq_set_prim_color(RGBA32(30, 30, 70, 255));
    rdpq_fill_rectangle(20, 8, SCREEN_WIDTH-20, 26);
    rdpq_text_printf(NULL, 1, 30, 24, "%s", T(STR_CONTROLS_TITLE));

    int row_h = 22;
    int start_y = 40;
    for (int i = 0; i < ACTION_COUNT; i++) {
        int y = start_y + i * row_h;
        bool is_sel = (i == sel);

        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(is_sel ? RGBA32(60, 90, 160, 220) : RGBA32(22, 22, 55, 200));
        rdpq_fill_rectangle(20, y, SCREEN_WIDTH-20, y+18);

        rdpq_text_printf(NULL, 1, 26, y+13, "%s", T(ACTION_STR[i]));

        PhysicalButton btn = controls_get_binding((GameAction)i);
        char btn_buf[16];
        snprintf(btn_buf, sizeof(btn_buf), "[ %s ]", PHYSICAL_BUTTON_NAMES[btn]);
        rdpq_text_printf(NULL, 1, SCREEN_WIDTH-90, y+13, "%s", btn_buf);
    }

    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(10, 10, 25, 220));
    rdpq_fill_rectangle(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (waiting_for_button) {
        rdpq_set_prim_color(RGBA32(255, 220, 100, 255));
        rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, "%s", T(STR_CONTROLS_WAITING));
    } else if (saved_flash_timer > 0.0f) {
        rdpq_set_prim_color(RGBA32(120, 255, 120, 255));
        rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, "%s", T(STR_CONTROLS_SAVED));
    } else {
        rdpq_set_prim_color(RGBA32(200, 200, 220, 255));
        rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, "%s", T(STR_CONTROLS_HINT));
    }
}

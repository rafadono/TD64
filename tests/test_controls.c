#include "test_harness.h"
#include "../src/systems/controls.h"
#include "../src/systems/save.h"
#include <string.h>

// =============================================================================
// Tests for src/systems/controls.c, linked against tests/fakes/save_stub.c
// instead of the real save.c (real Controller Pak I/O isn't something worth
// faking realistically on a host build — these tests dictate the pak's
// behavior directly instead).
// =============================================================================

// Test-controlled state from tests/fakes/save_stub.c.
extern SaveStatus stub_save_status;
extern InputConfig stub_input_config;
extern bool stub_input_config_present;
extern InputConfig stub_last_written_config;
extern bool stub_write_was_called;

static void reset_stub(void) {
    stub_save_status = SAVE_STATUS_NO_PAK;
    stub_input_config_present = false;
    stub_write_was_called = false;
    memset(&stub_input_config, 0, sizeof(stub_input_config));
    memset(&stub_last_written_config, 0, sizeof(stub_last_written_config));
}

static joypad_buttons_t no_buttons(void) {
    joypad_buttons_t b;
    memset(&b, 0, sizeof(b));
    return b;
}

static void test_reset_defaults_and_c_up_invariant(void) {
    SECTION("controls_reset_defaults: restores defaults; C-up is never a default binding");
    reset_stub();
    controls_set_binding(ACTION_PLACE, BTN_START); // scramble it first
    controls_reset_defaults();

    CHECK(controls_get_binding(ACTION_PLACE) == BTN_A);
    CHECK(controls_get_binding(ACTION_CANCEL) == BTN_B);
    CHECK(controls_get_binding(ACTION_PREV_UNIT) == BTN_L);
    CHECK(controls_get_binding(ACTION_NEXT_UNIT) == BTN_R);
    CHECK(controls_get_binding(ACTION_UPGRADE) == BTN_C_RIGHT);
    CHECK(controls_get_binding(ACTION_SPAWN_WAVE) == BTN_Z);
    CHECK(controls_get_binding(ACTION_PAUSE) == BTN_START);

    // Design invariant from this session's remap work: C-up is hardwired to
    // the debug menu, so no default binding may point at it (an action
    // bound there would become unreachable whenever that menu is open).
    for (int a = 0; a < ACTION_COUNT; a++) {
        CHECK(controls_get_binding((GameAction)a) != BTN_C_UP);
    }
}

static void test_get_set_binding_bounds_checking(void) {
    SECTION("controls_get_binding / controls_set_binding: out-of-range inputs are safe no-ops");
    reset_stub();
    controls_reset_defaults();

    CHECK(controls_get_binding((GameAction)-1) == BTN_A);
    CHECK(controls_get_binding((GameAction)ACTION_COUNT) == BTN_A);

    PhysicalButton before = controls_get_binding(ACTION_PLACE);
    controls_set_binding((GameAction)-1, BTN_START);       // invalid action -> ignored
    controls_set_binding(ACTION_PLACE, (PhysicalButton)-1); // invalid button -> ignored
    controls_set_binding(ACTION_PLACE, (PhysicalButton)BTN_COUNT); // out of range -> ignored
    CHECK(controls_get_binding(ACTION_PLACE) == before);

    controls_set_binding(ACTION_PLACE, BTN_C_LEFT); // a valid rebind should still work
    CHECK(controls_get_binding(ACTION_PLACE) == BTN_C_LEFT);
}

static void test_button_matches_every_button(void) {
    SECTION("button_matches: each PhysicalButton maps to exactly its own joypad_buttons_t field");
    for (int btn = 0; btn < BTN_COUNT; btn++) {
        joypad_buttons_t buttons = no_buttons();
        switch ((PhysicalButton)btn) {
            case BTN_A:       buttons.a = true; break;
            case BTN_B:       buttons.b = true; break;
            case BTN_Z:       buttons.z = true; break;
            case BTN_START:   buttons.start = true; break;
            case BTN_L:       buttons.l = true; break;
            case BTN_R:       buttons.r = true; break;
            case BTN_D_UP:    buttons.d_up = true; break;
            case BTN_D_DOWN:  buttons.d_down = true; break;
            case BTN_D_LEFT:  buttons.d_left = true; break;
            case BTN_D_RIGHT: buttons.d_right = true; break;
            case BTN_C_UP:    buttons.c_up = true; break;
            case BTN_C_DOWN:  buttons.c_down = true; break;
            case BTN_C_LEFT:  buttons.c_left = true; break;
            case BTN_C_RIGHT: buttons.c_right = true; break;
            default: break;
        }
        CHECK(button_matches(buttons, (PhysicalButton)btn) == true);
        // No OTHER button should read as pressed from this single-bit state.
        for (int other = 0; other < BTN_COUNT; other++) {
            if (other == btn) continue;
            CHECK(button_matches(buttons, (PhysicalButton)other) == false);
        }
    }
}

static void test_action_pressed_follows_the_current_binding(void) {
    SECTION("action_pressed: reflects whichever physical button is currently bound");
    reset_stub();
    controls_reset_defaults();
    controls_set_binding(ACTION_UPGRADE, BTN_C_LEFT);

    joypad_buttons_t pressed = no_buttons();
    pressed.c_left = true;
    CHECK(action_pressed(pressed, ACTION_UPGRADE) == true);

    joypad_buttons_t not_pressed = no_buttons();
    not_pressed.c_right = true; // the OLD default, no longer bound to this action
    CHECK(action_pressed(not_pressed, ACTION_UPGRADE) == false);
}

static void test_save_to_pak_writes_current_bindings(void) {
    SECTION("controls_save_to_pak: writes a valid InputConfig with the current bindings");
    reset_stub();
    stub_save_status = SAVE_STATUS_READY;
    controls_reset_defaults();
    controls_set_binding(ACTION_PAUSE, BTN_D_DOWN);

    bool ok = controls_save_to_pak();
    CHECK(ok == true);
    CHECK(stub_write_was_called == true);
    CHECK(stub_last_written_config.valid == 1);
    CHECK((PhysicalButton)stub_last_written_config.bindings[ACTION_PAUSE] == BTN_D_DOWN);
    CHECK((PhysicalButton)stub_last_written_config.bindings[ACTION_PLACE] == BTN_A);
}

static void test_init_keeps_defaults_when_no_pak(void) {
    SECTION("controls_init: no Controller Pak -> keeps compiled-in defaults");
    reset_stub();
    stub_save_status = SAVE_STATUS_NO_PAK;
    controls_set_binding(ACTION_PLACE, BTN_START); // would be overwritten by init() either way; scramble first

    controls_init();
    CHECK(controls_get_binding(ACTION_PLACE) == BTN_A);
}

static void test_init_keeps_defaults_when_pak_has_no_saved_config(void) {
    SECTION("controls_init: pak ready but no saved config -> keeps defaults");
    reset_stub();
    stub_save_status = SAVE_STATUS_READY;
    stub_input_config_present = false;

    controls_init();
    CHECK(controls_get_binding(ACTION_PLACE) == BTN_A);
}

static void test_init_ignores_a_config_marked_invalid(void) {
    SECTION("controls_init: an InputConfig with valid=0 (e.g. a freshly formatted pak) is ignored");
    reset_stub();
    stub_save_status = SAVE_STATUS_READY;
    stub_input_config_present = true;
    stub_input_config.valid = 0;
    stub_input_config.bindings[ACTION_PLACE] = BTN_D_RIGHT; // present, but must NOT be trusted

    controls_init();
    CHECK(controls_get_binding(ACTION_PLACE) == BTN_A);
}

static void test_init_loads_a_valid_saved_config(void) {
    SECTION("controls_init: loads bindings from a valid saved config");
    reset_stub();
    stub_save_status = SAVE_STATUS_READY;
    stub_input_config_present = true;
    stub_input_config.valid = 1;
    for (int i = 0; i < ACTION_COUNT; i++) stub_input_config.bindings[i] = BTN_A; // arbitrary baseline
    stub_input_config.bindings[ACTION_PLACE] = BTN_D_LEFT;
    stub_input_config.bindings[ACTION_UPGRADE] = BTN_START;

    controls_init();
    CHECK(controls_get_binding(ACTION_PLACE) == BTN_D_LEFT);
    CHECK(controls_get_binding(ACTION_UPGRADE) == BTN_START);
}

int main(void) {
    test_reset_defaults_and_c_up_invariant();
    test_get_set_binding_bounds_checking();
    test_button_matches_every_button();
    test_action_pressed_follows_the_current_binding();
    test_save_to_pak_writes_current_bindings();
    test_init_keeps_defaults_when_no_pak();
    test_init_keeps_defaults_when_pak_has_no_saved_config();
    test_init_ignores_a_config_marked_invalid();
    test_init_loads_a_valid_saved_config();
    SUMMARY_AND_RETURN();
}

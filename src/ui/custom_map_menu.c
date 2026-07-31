#include "custom_map_menu.h"
#include "map_editor.h"
#include "../core/engine.h"
#include <stdio.h>

// =============================================================================
// CUSTOM MAP MENU — lists the 8 Controller Pak slots
//
// A = play (occupied slot) | Start = edit (creates a new one if empty)
// C-left = delete, with confirmation (avoids losing a map from a single tap)
// L+R held + A = format the pak (only if unformatted — see save.h: this
// erases the ENTIRE Controller Pak, not just TD64, so it requires the same
// unusual combo already used by the debug cheats).
// =============================================================================

static int  sel = 0;
static bool slot_used[SAVE_MAP_SLOTS];
static bool pending_delete = false;

void custom_map_menu_enter(void) {
    save_system_check();
    save_list_custom_maps(slot_used);
    sel = 0;
    pending_delete = false;
}

void custom_map_menu_handle_input(GameState* game) {
    joypad_buttons_t kd   = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);

    if (pending_delete) {
        if (kd.a) {
            save_delete_custom_map(sel);
            save_list_custom_maps(slot_used);
            pending_delete = false;
        } else if (kd.b) {
            pending_delete = false;
        }
        return;
    }

    if (kd.d_up)   { sel--; if (sel < 0) sel = SAVE_MAP_SLOTS - 1; }
    if (kd.d_down) { sel++; if (sel >= SAVE_MAP_SLOTS) sel = 0; }

    SaveStatus st = save_system_status();

    // Unusual combo (same as the debug cheats) to format an unformatted pak
    // — this is never done automatically.
    if (st == SAVE_STATUS_UNFORMATTED && held.l && held.r && kd.a) {
        if (save_format_pak_confirmed()) {
            save_list_custom_maps(slot_used);
        }
        return;
    }

    if (st == SAVE_STATUS_READY) {
        if (kd.a && slot_used[sel]) {
            CustomMapSave data;
            if (save_read_custom_map(sel, &data)) {
                game->pending_custom_map = data;
                game->has_pending_custom_map = true;
                game->flow = STATE_FACTION_SELECT;
                return;
            }
        }

        if (kd.start) {
            map_editor_enter(sel);
            game->flow = STATE_MAP_EDITOR;
            return;
        }

        if (kd.c_left && slot_used[sel]) {
            pending_delete = true;
            return;
        }
    }

    if (kd.b) {
        game->flow = STATE_MAIN_MENU;
    }
}

void custom_map_menu_render(const GameState* game) {
    (void)game;

    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(10, 10, 22, 255));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    rdpq_set_prim_color(RGBA32(30, 30, 70, 255));
    rdpq_fill_rectangle(20, 8, SCREEN_WIDTH-20, 26);
    rdpq_text_printf(NULL, 1, 30, 24, "%s", T(STR_CMM_TITLE));

    SaveStatus st = save_system_status();
    const char* st_txt = (st == SAVE_STATUS_READY) ? T(STR_CMM_PAK_READY_MSG)
                        : (st == SAVE_STATUS_UNFORMATTED) ? T(STR_CMM_PAK_UNFORMATTED_MSG)
                        : T(STR_CMM_PAK_MISSING_MSG);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(st == SAVE_STATUS_READY ? RGBA32(120,255,120,255) : RGBA32(255,160,100,255));
    rdpq_text_printf(NULL, 1, 20, 40, "%s", st_txt);

    for (int i = 0; i < SAVE_MAP_SLOTS; i++) {
        int y = 50 + i * 20;
        bool is_sel = (i == sel);

        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(is_sel ? RGBA32(60, 90, 160, 220) : RGBA32(22, 22, 55, 200));
        rdpq_fill_rectangle(20, y, SCREEN_WIDTH-20, y+18);

        char line[64];
        if (slot_used[i]) {
            // Peek at the saved map's enemy faction/difficulty for a useful
            // one-line summary instead of just "saved".
            CustomMapSave data;
            if (st == SAVE_STATUS_READY && save_read_custom_map(i, &data)) {
                snprintf(line, sizeof(line), "SLOT %c: %s vs %s (%d)", 'A'+i,
                         T(STR_CMM_SLOT_SAVED), FACTION_NAME((int)data.enemy_faction), data.difficulty);
            } else {
                snprintf(line, sizeof(line), "SLOT %c: %s", 'A'+i, T(STR_CMM_SLOT_SAVED));
            }
        } else {
            snprintf(line, sizeof(line), "SLOT %c: %s", 'A'+i, T(STR_CMM_SLOT_EMPTY));
        }
        rdpq_text_printf(NULL, 1, 26, y+13, "%s", line);
    }

    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(10, 10, 25, 220));
    rdpq_fill_rectangle(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (pending_delete) {
        rdpq_set_prim_color(RGBA32(255, 100, 100, 255));
        rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, T(STR_CMM_CONFIRM_DELETE), 'A'+sel);
    } else {
        rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, "%s", T(STR_CMM_CONTROLS_HINT));
    }
}

#include "ui.h"
#include "../core/engine.h"
#include <stdio.h>

// HUD layout constants
#define HUD_H 28
#define PANEL_H 38

// Drawing primitive: rdpq_set_mode_standard() leaves the combiner in its
// default texture mode (TEX0), so any flat-color fill needs to explicitly
// re-assert RDPQ_COMBINER_FLAT before every rectangle — especially since
// rdpq_text_printf (used throughout this HUD) reconfigures the combiner
// internally to draw text and leaves it in that state afterwards.
static void ui_fill(int x, int y, int w, int h, color_t c) {
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(c);
    rdpq_fill_rectangle(x, y, x+w, y+h);
}

static void draw_stat_bar(int x, int y, int w, int h,
                          float pct, color_t fill, color_t bg) {
    ui_fill(x, y, w, h, bg);
    if (pct > 0) {
        ui_fill(x, y, (int)(w*pct), h, fill);
    }
    // Border
    ui_fill(x,   y,     w, 1, RGBA32(0,0,0,200));
    ui_fill(x,   y+h-1, w, 1, RGBA32(0,0,0,200));
    ui_fill(x,   y,     1, h, RGBA32(0,0,0,200));
    ui_fill(x+w-1,y,    1, h, RGBA32(0,0,0,200));
}

void ui_draw_hud(const GameState* game) {
    rdpq_set_mode_standard();

    // Top bar background
    ui_fill(0, 0, SCREEN_WIDTH, HUD_H, RGBA32(10, 10, 25, 220));

    // Gold bar (left)
    int max_gold_display = 500;
    float gold_pct = (float)game->gold / max_gold_display;
    if (gold_pct > 1.0f) gold_pct = 1.0f;
    draw_stat_bar(8, 6, 90, 8, gold_pct,
                  RGBA32(255, 200, 30, 255),
                  RGBA32(60, 50, 10, 255));
    // Gold icon dot
    ui_fill(4, 7, 4, 6, RGBA32(255, 215, 0, 255));

    // Gold numeric value
    rdpq_text_printf(NULL, 1, 14, 17, "%d", game->gold);

    // Lives bar (center-left)
    float lives_pct = game->lives_wave_start > 0
        ? (float)game->lives / game->lives_wave_start : 0.0f;
    if (lives_pct > 1.0f) lives_pct = 1.0f;
    draw_stat_bar(110, 6, 80, 8, lives_pct,
                  RGBA32(220, 40, 40, 255),
                  RGBA32(60, 10, 10, 255));
    ui_fill(106, 7, 4, 6, RGBA32(255, 80, 80, 255));

    // Lives numeric value
    rdpq_text_printf(NULL, 1, 114, 17, "HP:%d", game->lives);

    // Wave indicator (right side)
    ui_fill(SCREEN_WIDTH-70, 2, 68, HUD_H-4, RGBA32(30, 50, 100, 220));
    rdpq_text_printf(NULL, 1, SCREEN_WIDTH - 66, 17, "WAVE:%d", game->wave);

    // Faction colors on HUD
    const Campaign* c = campaign_get(game->campaign_id);
    if (c) {
        // Player faction strip
        ui_fill(202, 2, 28, HUD_H-4, RGBA32(
            c->player_faction == FACTION_DAWNGUARD ? 60  : (c->player_faction == FACTION_IRONBONE ? 110 : (c->player_faction == FACTION_ASHCLAW ? 200 : 40)),
            c->player_faction == FACTION_DAWNGUARD ? 110 : (c->player_faction == FACTION_IRONBONE ? 30  : (c->player_faction == FACTION_ASHCLAW ? 55  : 190)),
            c->player_faction == FACTION_DAWNGUARD ? 210 : (c->player_faction == FACTION_IRONBONE ? 150 : (c->player_faction == FACTION_ASHCLAW ? 30  : 220)),
            200));
    }

    // Score
    ui_fill(SCREEN_WIDTH/2 - 40, 2, 80, HUD_H-4, RGBA32(200, 200, 255, 255));
    rdpq_text_printf(NULL, 1, SCREEN_WIDTH/2 - 36, 17, "SCORE:%05d", game->score.score);

    // Combo if active
    if (game->score.combo_mult > 1) {
        score_render_combo(&game->score);
    }
}

void ui_draw_build_panel(const GameState* game) {
    // Bottom bar
    ui_fill(0, SCREEN_HEIGHT - PANEL_H, SCREEN_WIDTH, PANEL_H, RGBA32(10, 10, 25, 220));

    // Draw one button per unit type (not hero, too expensive for early game)
    int n_types = UNIT_TYPE_COUNT - 1; // exclude hero from quick-build
    int btn_w  = (SCREEN_WIDTH - 20) / n_types - 4;
    int btn_h  = PANEL_H - 8;
    int btn_y  = SCREEN_HEIGHT - PANEL_H + 4;

    for (int t = 0; t < n_types; t++) {
        int bx = 10 + t * (btn_w + 4);
        const UnitStats* s = unit_get_stats(game->player_faction, (UnitType)t);
        if (!s) continue;

        bool selected  = (game->selected_unit_type == t);
        bool can_afford = (game->gold >= s->cost);

        // Button background
        color_t bg;
        if (!can_afford)  bg = RGBA32(40,  40,  40,  200);
        else if (selected) bg = RGBA32(200, 175, 40,  240);
        else               bg = s->color_primary;

        ui_fill(bx, btn_y, btn_w, btn_h, bg);

        // Border
        color_t bcol = selected ? RGBA32(255,255,100,255) : RGBA32(0,0,0,200);
        ui_fill(bx, btn_y, btn_w, 1, bcol);
        ui_fill(bx, btn_y+btn_h-1, btn_w, 1, bcol);
        ui_fill(bx, btn_y, 1, btn_h, bcol);
        ui_fill(bx+btn_w-1, btn_y, 1, btn_h, bcol);

        // Unit type indicator dots (type index)
        for (int d = 0; d <= t; d++) {
            ui_fill(bx+3+d*4, btn_y+btn_h-6, 3, 3,
                    can_afford ? s->color_secondary : RGBA32(80,80,80,255));
        }
    }

    // Hero button (separate, larger, at right)
    {
        const UnitStats* hs = unit_get_stats(game->player_faction, UNIT_HERO);
        if (hs) {
            bool sel = (game->selected_unit_type == UNIT_HERO);
            bool can = (game->gold >= hs->cost);
            int hbx = SCREEN_WIDTH - 50;
            color_t bg = !can ? RGBA32(40,40,40,200)
                        : sel  ? RGBA32(255,215,0,255)
                               : hs->color_primary;
            ui_fill(hbx, btn_y, 40, btn_h, bg);
            // Star icon
            ui_fill(hbx+16, btn_y+2, 8, 8, RGBA32(255,215,0,255));
        }
    }
}

void ui_draw_unit_tooltip(const GameState* game) {
    if (game->selected_unit_type < 0 ||
        game->selected_unit_type >= UNIT_TYPE_COUNT) return;

    const UnitStats* s = unit_get_stats(game->player_faction,
                                        (UnitType)game->selected_unit_type);
    if (!s) return;

    // Small tooltip card
    int tx = 10, ty = SCREEN_HEIGHT - PANEL_H - 53;
    int tw = 180, th = 48;

    ui_fill(tx, ty, tw, th, RGBA32(10, 10, 25, 230));
    ui_fill(tx, ty, tw, 3, s->color_primary);

    // Unit Name
    rdpq_text_printf(NULL, 1, tx+5, ty+10, "%s", s->name);

    // Stat bars inside tooltip
    // HP
    draw_stat_bar(tx+5, ty+15, 80, 5,
                  (float)s->hp_max / 400.0f,
                  RGBA32(40, 220, 60, 255),
                  RGBA32(20, 50, 20, 255));
    rdpq_text_printf(NULL, 1, tx+90, ty+19, "HP:%d", s->hp_max);

    // Damage
    draw_stat_bar(tx+5, ty+25, 80, 5,
                  (float)s->damage / 60.0f,
                  RGBA32(220, 60, 40, 255),
                  RGBA32(50, 20, 20, 255));
    rdpq_text_printf(NULL, 1, tx+90, ty+29, "DMG:%d", s->damage);

    // Range
    draw_stat_bar(tx+5, ty+35, 80, 5,
                  s->attack_range / 200.0f,
                  RGBA32(40, 120, 220, 255),
                  RGBA32(20, 30, 50, 255));
    rdpq_text_printf(NULL, 1, tx+90, ty+39, "RNG:%d", (int)s->attack_range);

    // Cost pill
    ui_fill(tx+tw-45, ty+15, 40, 28, RGBA32(255, 200, 30, 220));
    rdpq_text_printf(NULL, 1, tx+tw-38, ty+31, "%dG", s->cost);
}

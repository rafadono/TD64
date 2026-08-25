#include "menu.h"
#include "../core/engine.h"
#include "custom_map_menu.h"
#include "controls_menu.h"
#include "stats_menu.h"
#include <string.h>
#include <stdio.h>

// =============================================================================
// PER-FACTION VISUAL PALETTE
// Primary / Secondary / Light / Dark
// =============================================================================

typedef struct { color_t primary, secondary, light, dark; } FactionPalette;

static const FactionPalette PAL[FACTION_COUNT] = {
    [FACTION_DAWNGUARD] = { RGBA32(60,110,210,255), RGBA32(200,175,50,255),
                            RGBA32(140,180,255,255), RGBA32(20,40,100,255) },
    [FACTION_IRONBONE]  = { RGBA32(110,30,150,255), RGBA32(60,180,60,255),
                            RGBA32(170,80,210,255),  RGBA32(35,10,55,255)  },
    [FACTION_ASHCLAW]   = { RGBA32(200,55,30,255),  RGBA32(130,75,30,255),
                            RGBA32(255,130,80,255),  RGBA32(85,18,8,255)   },
    [FACTION_VEILSTORM] = { RGBA32(40,190,220,255), RGBA32(160,80,240,255),
                            RGBA32(140,230,250,255), RGBA32(12,70,100,255) },
};

// =============================================================================
// INTERNAL STATE
// =============================================================================

static int   fs_sel    = 0;   // highlighted faction in faction select
static int   pause_sel = 0;
static int   end_sel   = 0;
static int   main_sel  = 0;
static int   diff_sel  = GAME_DIFF_NORMAL; // highlighted option in difficulty select
static float pulse     = 0.0f; // 0..1 loop for animations

#define MAIN_MENU_OPTION_COUNT 6 // Play Campaign / Custom Maps / Controls / Stats / Language / Credits

// =============================================================================
// DRAWING PRIMITIVES
// =============================================================================

// rdpq_set_mode_standard() leaves the combiner in its default texture mode
// (TEX0); since rdpq_text_printf (used on every screen in this file)
// reconfigures the combiner internally to draw text, RDPQ_COMBINER_FLAT must
// be re-asserted before EVERY flat-color rectangle, not just once at the top
// of the screen.
static void fill(int x, int y, int w, int h, color_t c) {
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(c);
    rdpq_fill_rectangle(x, y, x+w, y+h);
}

// 1px border
static void border_rect(int x, int y, int w, int h, color_t c) {
    fill(x,       y,       w, 1, c);
    fill(x,       y+h-1,   w, 1, c);
    fill(x,       y,       1, h, c);
    fill(x+w-1,   y,       1, h, c);
}

// Filled + border
static void panel(int x, int y, int w, int h, color_t bg, color_t edge) {
    fill(x, y, w, h, bg);
    border_rect(x, y, w, h, edge);
}

// Stat bar: bg track + colored fill
static void stat_bar(int x, int y, int w, int h, float pct, color_t bar_c) {
    fill(x, y, w, h, RGBA32(20, 20, 40, 255));
    if (pct > 0.0f) {
        int fw = (int)(w * pct);
        if (fw < 2) fw = 2;
        if (fw > w) fw = w;
        fill(x, y, fw, h, bar_c);
    }
    border_rect(x, y, w, h, RGBA32(0, 0, 0, 180));
}

// Faction average of a stat (normalised 0..1). Only the 6 playable roles —
// the enemy-only elite variants (UNIT_ARMORED/WARDED/FLYER) would skew this
// preview with stats (0 damage/range, since they never attack as towers)
// that have nothing to do with what the player is actually choosing here.
static float faction_stat_pct(FactionId f, int stat, float max_val) {
    float sum = 0;
    for (int t = 0; t < PLAYABLE_UNIT_TYPE_COUNT; t++) {
        const UnitStats* s = unit_get_stats(f, (UnitType)t);
        if (!s) continue;
        switch (stat) {
            case 0: sum += s->hp_max;       break;
            case 1: sum += s->damage;       break;
            case 2: sum += s->attack_range; break;
            case 3: sum += s->move_speed;   break;
        }
    }
    float pct = (sum / PLAYABLE_UNIT_TYPE_COUNT) / max_val;
    return pct > 1.0f ? 1.0f : pct;
}

// =============================================================================
// FACTION SELECT — SMALL CARD (not selected)
// =============================================================================
// Layout: [accent| name ......  | rival badge ]

#define CARD_X    10
#define CARD_W   (SCREEN_WIDTH - 2*CARD_X)  // stretches to fill the width; every
                                             // offset inside draw_card_small/large
                                             // is expressed relative to CARD_W, so
                                             // they all scale automatically from this
#define CARD_SM_H  26

static void draw_card_small(int y, FactionId f) {
    const FactionPalette* p = &PAL[f];

    fill(CARD_X, y, CARD_W, CARD_SM_H, RGBA32(20, 20, 45, 220));
    border_rect(CARD_X, y, CARD_W, CARD_SM_H, RGBA32(40, 40, 80, 200));

    // Left color accent strip
    fill(CARD_X+1, y+1, 5, CARD_SM_H-2, p->primary);

    // Name text
    rdpq_text_printf(NULL, 1, CARD_X+12, y+16, "%s", FACTION_NAME(f));

    // Right: rival faction mini badge
    FactionId rival = campaign_get_rival(f);
    fill(CARD_X + CARD_W - 36, y+5, 28, 16, PAL[rival].primary);
    border_rect(CARD_X + CARD_W - 36, y+5, 28, 16, RGBA32(0, 0, 0, 200));
    // "VS" dots
    fill(CARD_X + CARD_W - 44, y+8,  4, 4, RGBA32(180,180,180,200));
    fill(CARD_X + CARD_W - 44, y+14, 4, 4, RGBA32(180,180,180,200));
}

// =============================================================================
// FACTION SELECT — EXPANDED CARD (selected)
// =============================================================================
// 300 x 94px layout:
//  [0..20 ]  Header: faction color + name + sel marker
//  [22..38]  Description
//  [40..58]  Unit icons: 6 squares sized by role
//  [60..70]  Stat bars: HP / DMG / RNG / SPD
//  [72..88]  VS row: rival badge + rival name

#define CARD_LG_H  94

static void draw_card_large(int y, FactionId f) {
    const FactionPalette* p = &PAL[f];
    uint8_t pulse_val = (uint8_t)(180 + 75 * pulse);
    color_t glow = { p->secondary.r, p->secondary.g,
                     p->secondary.b, pulse_val };

    // Background
    fill(CARD_X, y, CARD_W, CARD_LG_H, p->dark);

    // Pulsing double border
    border_rect(CARD_X,   y,   CARD_W,   CARD_LG_H,   glow);
    border_rect(CARD_X+2, y+2, CARD_W-4, CARD_LG_H-4,
                RGBA32(p->secondary.r, p->secondary.g, p->secondary.b, 60));

    // Header strip (top 21px)
    fill(CARD_X+1, y+1, CARD_W-2, 20, p->primary);
    // Name block
    rdpq_text_printf(NULL, 1, CARD_X+8, y+14, "%s", FACTION_NAME(f));
    // "SELECTED" gold marker (top-right)
    fill(CARD_X + CARD_W - 22, y+5, 14, 12, RGBA32(255, 215, 0, 255));
    rdpq_text_printf(NULL, 1, CARD_X + CARD_W - 19, y+14, "S");

    // Description (y+23..y+38)
    rdpq_text_printf(NULL, 1, CARD_X+8, y+32, "%s", faction_get_description(f));

    // Unit type icons (y+41..y+57) — the 6 playable roles, each slightly
    // different size to hint at their role. The enemy-only elite variants
    // (UNIT_ARMORED/WARDED/FLYER) are never placeable, so they don't belong
    // in a "what can I build" preview.
    static const int ICON_W[PLAYABLE_UNIT_TYPE_COUNT] = { 8, 11, 9, 10, 14, 13 };
    static const int ICON_H[PLAYABLE_UNIT_TYPE_COUNT] = { 11, 13, 9, 13, 15, 16 };
    int ix = CARD_X + 8;
    for (int t = 0; t < PLAYABLE_UNIT_TYPE_COUNT; t++) {
        int iw = ICON_W[t], ih = ICON_H[t];
        int iy = y + 41 + (16 - ih); // bottom-align
        panel(ix, iy, iw, ih, p->primary, RGBA32(0,0,0,200));
        // Inner detail
        fill(ix+2, iy+2, iw-4, 3, p->secondary);
        // Hero gets a gold crown bar on top
        if (t == UNIT_HERO) {
            fill(ix, iy-3, iw, 3, RGBA32(255, 215, 0, 255));
        }
        ix += iw + 6;
    }

    // Stat bars (y+60..y+69), 4 bars side by side
    static const color_t STAT_COLORS[4] = {
        RGBA32(50, 220, 70,  255),  // HP    — green
        RGBA32(220, 60, 40,  255),  // DMG   — red
        RGBA32(50, 120, 220, 255),  // RNG   — blue
        RGBA32(220,200, 40,  255),  // SPD   — yellow
    };
    static const float STAT_MAX[4] = { 420.0f, 60.0f, 180.0f, 65.0f };

    int bar_w  = (CARD_W - 20) / 4 - 4;
    int bar_x  = CARD_X + 8;
    for (int s = 0; s < 4; s++) {
        float pct = faction_stat_pct(f, s, STAT_MAX[s]);
        stat_bar(bar_x, y+61, bar_w, 8, pct, STAT_COLORS[s]);
        // Tiny color dot label above bar
        fill(bar_x, y+57, 6, 3, STAT_COLORS[s]);
        bar_x += bar_w + 5;
    }

    // VS / Rival row (y+72..y+88)
    FactionId rival = campaign_get_rival(f);
    const FactionPalette* rp = &PAL[rival];

    // "VS" label
    rdpq_text_printf(NULL, 1, CARD_X+8, y+82, "VS");
    // Arrow
    fill(CARD_X+24, y+77, 8, 4, RGBA32(200, 200, 200, 180));
    // Rival faction badge
    panel(CARD_X+34, y+72, 70, 14, rp->primary, RGBA32(0,0,0,200));
    rdpq_text_printf(NULL, 1, CARD_X+38, y+82, "%s", FACTION_NAME(rival));
    // Rival label to the left
    rdpq_text_printf(NULL, 1, CARD_X + CARD_W - 88, y+82, "%s", T(STR_LABEL_RIVAL));
}

// =============================================================================
// FACTION SELECT — FULL SCREEN
// =============================================================================
// Header (18px) + 4 dynamic cards + hint bar (16px)
//
// Heights: 3 x SM(26) + 1 x LG(94) + 4 gaps(3) = 184px
// Total used: 18 + 4 + 184 + 16 = 222px, fits in 240px

static void render_faction_select(void) {
    // Background
    fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA32(10, 10, 22, 255));

    // Header bar
    fill(0, 0, SCREEN_WIDTH, 18, RGBA32(18, 18, 45, 255));
    // Active faction accent line
    fill(0, 16, SCREEN_WIDTH, 2, PAL[fs_sel].primary);
    // Title
    rdpq_text_printf(NULL, 1, 10, 12, "%s", T(STR_FACTION_SELECT_TITLE));
    // Subtitle (right-anchored: 142px margin from the right edge at design res)
    rdpq_text_printf(NULL, 1, SCREEN_WIDTH - 142, 12, "%s", T(STR_FACTION_SELECT_SUBTITLE));

    // Faction cards
    int cur_y = 22; // start just below header
    for (int i = 0; i < FACTION_COUNT; i++) {
        if (i == fs_sel)
            draw_card_large(cur_y, (FactionId)i);
        else
            draw_card_small(cur_y, (FactionId)i);

        cur_y += (i == fs_sel ? CARD_LG_H : CARD_SM_H) + 3;
    }

    // Hint bar (bottom)
    int hy = SCREEN_HEIGHT - 16;
    fill(0, hy, SCREEN_WIDTH, 16, RGBA32(18, 18, 45, 230));
    fill(0, hy, SCREEN_WIDTH, 1,  RGBA32(60, 60, 110, 255));

    // D-pad icon
    fill(10, hy+4, 6,  8, RGBA32(180,180,220,220));   // vertical bar
    fill( 7, hy+7, 12, 3, RGBA32(180,180,220,220));   // horizontal bar

    // A button
    fill(88, hy+3, 12, 10, RGBA32(50,200,80,255));
    border_rect(88, hy+3, 12, 10, RGBA32(0,0,0,200));

    // B button
    fill(158, hy+3, 12, 10, RGBA32(200,50,50,255));
    border_rect(158, hy+3, 12, 10, RGBA32(0,0,0,200));

    // Labels beside buttons
    rdpq_text_printf(NULL, 1, 25,  hy+12, "%s", T(STR_HINT_NAVIGATE));
    rdpq_text_printf(NULL, 1, 103, hy+12, "%s", T(STR_HINT_CONFIRM));
    rdpq_text_printf(NULL, 1, 173, hy+12, "%s", T(STR_HINT_BACK));
}

// =============================================================================
// MAIN MENU
// =============================================================================

static void render_main_menu(void) {
    fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA32(10, 10, 22, 255));

    // Title block
    panel(20, 20, SCREEN_WIDTH-40, 48, RGBA32(30,30,70,255),
          RGBA32(80,80,160,255));
    rdpq_text_printf(NULL, 1, 35, 42, "%s", T(STR_TITLE_MAIN));

    // 6 options: Play Campaign / Custom Maps / Controls / Stats / Language / Credits
    const color_t opt_col[MAIN_MENU_OPTION_COUNT] = {
        RGBA32(60,120,220,255), RGBA32(90,150,90,220), RGBA32(110,110,180,220),
        RGBA32(180,140,60,220), RGBA32(150,120,60,220), RGBA32(70,70,80,200)
    };
    char lang_label[32];
    snprintf(lang_label, sizeof(lang_label), "%s: %s", T(STR_MENU_LANGUAGE), lang_name(lang_get()));
    const char* labels[MAIN_MENU_OPTION_COUNT] = {
        T(STR_MENU_PLAY_CAMPAIGN), T(STR_MENU_CUSTOM_MAPS), T(STR_MENU_CONTROLS),
        T(STR_MENU_STATS), lang_label, T(STR_MENU_CREDITS)
    };
    for (int i = 0; i < MAIN_MENU_OPTION_COUNT; i++) {
        bool sel = (i == main_sel);
        int oy = 78 + i*28;
        panel(40, oy, SCREEN_WIDTH-80, 24,
              sel ? opt_col[i] : RGBA32(22,22,55,220),
              sel ? RGBA32(255,255,180,255) : RGBA32(55,55,100,200));
        // Left accent
        fill(40, oy, 5, 24, opt_col[i]);
        // Label
        rdpq_text_printf(NULL, 1, 55, oy+16, "%s", labels[i]);
        // Arrow
        if (sel) fill(SCREEN_WIDTH-65, oy+9, 16, 6,
                      RGBA32(255,255,100,255));
    }

    // Hint
    fill(0, SCREEN_HEIGHT-14, SCREEN_WIDTH, 14, RGBA32(18,18,45,230));
    fill(0, SCREEN_HEIGHT-14, SCREEN_WIDTH, 1,  RGBA32(60,60,110,255));
    rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, "%s", T(STR_HINT_MAIN_MENU));
}

// =============================================================================
// DIFFICULTY SELECT — shown after "PLAY CAMPAIGN", before faction select.
// Not used by custom maps, which have their own per-map difficulty label.
// =============================================================================

static void render_difficulty_select(void) {
    fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA32(10, 10, 22, 255));

    panel(20, 20, SCREEN_WIDTH-40, 32, RGBA32(30,30,70,255), RGBA32(80,80,160,255));
    rdpq_text_printf(NULL, 1, 35, 40, "%s", T(STR_DIFF_SELECT_TITLE));

    static const StringId DIFF_STR[GAME_DIFF_COUNT] = {
        STR_DIFF_EASY, STR_DIFF_NORMAL, STR_DIFF_HARD, STR_DIFF_EXTREME
    };
    static const color_t DIFF_COL[GAME_DIFF_COUNT] = {
        RGBA32(90,170,90,220), RGBA32(90,130,210,220),
        RGBA32(210,140,60,220), RGBA32(200,60,60,220)
    };

    for (int i = 0; i < GAME_DIFF_COUNT; i++) {
        bool sel      = (i == diff_sel);
        bool unlocked = game_difficulty_unlocked((GameDifficulty)i);
        int  oy       = 66 + i*30;

        color_t bg = !unlocked ? RGBA32(20,20,35,220)
                   : sel        ? DIFF_COL[i]
                                : RGBA32(22,22,55,220);
        panel(40, oy, SCREEN_WIDTH-80, 24, bg,
              sel ? RGBA32(255,255,180,255) : RGBA32(55,55,100,200));
        fill(40, oy, 5, 24, unlocked ? DIFF_COL[i] : RGBA32(70,70,80,255));

        rdpq_text_printf(NULL, 1, 55, oy+16, "%s", T(DIFF_STR[i]));
        if (!unlocked) rdpq_text_printf(NULL, 1, SCREEN_WIDTH-95, oy+16, "%s", T(STR_DIFF_LOCKED_BADGE));
        if (sel) fill(SCREEN_WIDTH-65, oy+9, 16, 6, RGBA32(255,255,100,255));
    }

    fill(0, SCREEN_HEIGHT-24, SCREEN_WIDTH, 24, RGBA32(18,18,45,230));
    fill(0, SCREEN_HEIGHT-24, SCREEN_WIDTH, 1,  RGBA32(60,60,110,255));
    if (!game_difficulty_unlocked((GameDifficulty)diff_sel)) {
        rdpq_text_printf(NULL, 1, 10, SCREEN_HEIGHT-12, "%s", T(STR_DIFF_LOCKED_HINT));
    } else {
        rdpq_text_printf(NULL, 1, 10, SCREEN_HEIGHT-12, "%s", T(STR_HINT_MAIN_MENU));
    }
}

// =============================================================================
// PAUSE MENU
// =============================================================================

static void render_paused(const GameState* game) {
    // Overlay
    fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA32(0,0,0,170));

    // Dialog box (centered both ways: dy tuned to sit 65px above vertical
    // center at design resolution, so it stays centered at any SCREEN_HEIGHT)
    int dx = SCREEN_WIDTH/2-72, dw = 144, dy = SCREEN_HEIGHT/2 - 65, dh = 118;
    panel(dx, dy, dw, dh, RGBA32(15,15,38,245), RGBA32(90,90,180,255));

    // Header
    fill(dx+1, dy+1, dw-2, 20, PAL[game->player_faction].primary);
    rdpq_text_printf(NULL, 1, dx+20, dy+14, "%s", T(STR_PAUSE_TITLE));

    // 3 options
    static const color_t OPT_COL[3] = {
        RGBA32(40,160,75,255),    // Resume — green
        RGBA32(50,110,200,255),   // Restart — blue
        RGBA32(160,45,45,255),    // Menu — red
    };

    for (int i = 0; i < 3; i++) {
        bool sel = (i == pause_sel);
        int oy = dy + 27 + i*28;
        panel(dx+8, oy, dw-16, 22,
              sel ? OPT_COL[i] : RGBA32(30,30,65,220),
              sel ? RGBA32(255,255,180,200) : RGBA32(55,55,100,200));
        const char* label = (i == 0) ? T(STR_PAUSE_RESUME)
                           : (i == 1) ? T(STR_PAUSE_RESTART)
                                      : T(STR_PAUSE_MAIN_MENU);
        rdpq_text_printf(NULL, 1, dx+20, oy+15, "%s", label);
    }
}

// =============================================================================
// GAME OVER / VICTORY
// =============================================================================

static void render_end_screen(bool victory, const GameState* game) {
    fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA32(10,10,22,255));

    // Header
    color_t hbg = victory ? RGBA32(25,95,25,255) : RGBA32(110,18,18,255);
    panel(20, 18, SCREEN_WIDTH-40, 48, hbg, RGBA32(255,255,255,200));
    rdpq_text_printf(NULL, 1, 30, 38, "%s", victory ? T(STR_END_VICTORY) : T(STR_END_DEFEAT));

    // Stats card
    panel(20, 72, SCREEN_WIDTH-40, 92, RGBA32(18,18,48,220),
          RGBA32(75,75,140,200));
    // Faction color top strip
    fill(21, 73, SCREEN_WIDTH-42, 5, PAL[game->player_faction].primary);

    // 4 stat rows
    static const color_t ROW_COL[4] = {
        RGBA32(60,200,255,200), RGBA32(255,215,0,200),
        RGBA32(220,60,60,200),  RGBA32(50,220,80,200),
    };
    for (int i = 0; i < 4; i++) {
        int ry = 82 + i*20;
        fill(30, ry, SCREEN_WIDTH-60, 16, RGBA32(28,28,58,200));
        float pct = 0.0f;
        if (i == 0) {
            rdpq_text_printf(NULL, 1, 34, ry+12, T(STR_STAT_KILLS), game->score.total_kills);
            pct = game->score.total_kills / 100.0f;
        } else if (i == 1) {
            rdpq_text_printf(NULL, 1, 34, ry+12, T(STR_STAT_GOLD), game->score.total_gold);
            pct = game->score.total_gold / 3000.0f;
        } else if (i == 2) {
            rdpq_text_printf(NULL, 1, 34, ry+12, T(STR_STAT_WAVE), game->wave);
            pct = game->wave / 10.0f;
        } else if (i == 3) {
            rdpq_text_printf(NULL, 1, 34, ry+12, T(STR_STAT_PERFECT), game->score.perfect_waves);
            pct = game->wave > 0 ? (float)game->score.perfect_waves / game->wave : 0.0f;
        }
        if (pct > 1.0f) pct = 1.0f;
        stat_bar(112, ry+4, SCREEN_WIDTH-146, 8, pct, ROW_COL[i]);
    }

    // 2 buttons (bottom-anchored: 68px from the bottom edge at i=0, so they
    // hug the bottom instead of drifting away from it at other SCREEN_HEIGHTs)
    color_t btn0 = victory ? RGBA32(25,95,25,240) : RGBA32(95,20,20,240);
    for (int i = 0; i < 2; i++) {
        bool sel = (i == end_sel);
        int by = SCREEN_HEIGHT - 68 + i*30;
        panel(SCREEN_WIDTH/2-68, by, 136, 24,
              sel ? (i==0 ? btn0 : RGBA32(35,45,95,240)) : RGBA32(22,22,50,220),
              sel ? RGBA32(255,255,180,255) : RGBA32(55,55,100,200));
        rdpq_text_printf(NULL, 1, SCREEN_WIDTH/2-44, by+15, "%s", (i == 0) ? T(STR_END_RETRY) : T(STR_END_CHANGE_FACTION));
    }
}

// =============================================================================
// PUBLIC: RENDER
// =============================================================================

void menu_render(const GameState* game) {
    // Advance pulse animation (sawtooth 0->1) using a separate accumulator
    static float pulse_time = 0.0f;
    pulse_time += 0.018f;
    if (pulse_time > 1.0f) pulse_time -= 1.0f;
    // Convert to triangle wave (0->1->0) and store in the global pulse variable
    pulse = pulse_time < 0.5f ? pulse_time * 2.0f : (1.0f - pulse_time) * 2.0f;

    switch (game->flow) {
        case STATE_MAIN_MENU:        render_main_menu();               break;
        case STATE_DIFFICULTY_SELECT: render_difficulty_select();      break;
        case STATE_FACTION_SELECT: render_faction_select();          break;
        case STATE_PAUSED:         render_paused(game);              break;
        case STATE_GAME_OVER:      render_end_screen(false, game);   break;
        case STATE_VICTORY:        render_end_screen(true,  game);   break;
        case STATE_CUSTOM_MAP_MENU: custom_map_menu_render(game);    break;
        default: break;
    }
}

// =============================================================================
// PUBLIC: INPUT
// =============================================================================

int menu_handle_input(GameState* game) {
    joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    switch (game->flow) {

        // ── MAIN MENU ────────────────────────────────────────────────────────
        case STATE_MAIN_MENU: {
            if (keys.d_up)   { main_sel--; if (main_sel < 0) main_sel = MAIN_MENU_OPTION_COUNT-1; }
            if (keys.d_down) { main_sel++; if (main_sel > MAIN_MENU_OPTION_COUNT-1) main_sel = 0; }

            if (keys.a || keys.start) {
                if (main_sel == 0) {
                    // -> Choose a difficulty first, then faction select
                    game->flow = STATE_DIFFICULTY_SELECT;
                    diff_sel   = (int)game_difficulty_get();
                } else if (main_sel == 1) {
                    // -> Custom maps (editor + Controller Pak)
                    custom_map_menu_enter();
                    game->flow = STATE_CUSTOM_MAP_MENU;
                } else if (main_sel == 2) {
                    // -> Rebind in-game actions (Controller Pak)
                    controls_menu_enter();
                    game->flow = STATE_CONTROLS_MENU;
                } else if (main_sel == 3) {
                    // -> Lifetime stats + last run highlights
                    stats_menu_enter();
                    game->flow = STATE_STATS_MENU;
                } else if (main_sel == 4) {
                    // -> Cycle language in place, stay on this menu
                    lang_cycle();
                }
                // main_sel == 5: credits (future)
            }
            return main_sel;
        }

        // ── DIFFICULTY SELECT ────────────────────────────────────────────────
        case STATE_DIFFICULTY_SELECT: {
            if (keys.d_up)   { diff_sel--; if (diff_sel < 0) diff_sel = GAME_DIFF_COUNT-1; }
            if (keys.d_down) { diff_sel++; if (diff_sel > GAME_DIFF_COUNT-1) diff_sel = 0; }

            if ((keys.a || keys.start) && game_difficulty_unlocked((GameDifficulty)diff_sel)) {
                game_difficulty_set((GameDifficulty)diff_sel);
                game->flow = STATE_FACTION_SELECT;
                fs_sel = 0;
            }

            if (keys.b) {
                game->flow = STATE_MAIN_MENU;
            }
            return diff_sel;
        }

        // ── FACTION SELECT ───────────────────────────────────────────────────
        case STATE_FACTION_SELECT: {
            if (keys.d_up || keys.l) {
                fs_sel--;
                if (fs_sel < 0) fs_sel = FACTION_COUNT - 1;
            }
            if (keys.d_down || keys.r) {
                fs_sel++;
                if (fs_sel >= FACTION_COUNT) fs_sel = 0;
            }

            if (keys.a || keys.start) {
                if (game->has_pending_custom_map) {
                    // Coming from the editor or from "play" on a saved custom map
                    game->has_pending_custom_map = false;
                    game_start_custom_map(game, (FactionId)fs_sel, &game->pending_custom_map);
                } else {
                    // Confirm faction -> start the matching campaign
                    campaign_start_with_faction(game, (FactionId)fs_sel);
                }
                // both paths leave game->flow = STATE_PLAYING
            }

            if (keys.b) {
                // Back to main menu (discards any pending custom map)
                game->has_pending_custom_map = false;
                game->flow = STATE_MAIN_MENU;
            }

            return fs_sel;
        }

        // ── PAUSED ───────────────────────────────────────────────────────────
        case STATE_PAUSED: {
            if (keys.d_up)   { pause_sel--; if (pause_sel < 0) pause_sel = 2; }
            if (keys.d_down) { pause_sel++; if (pause_sel > 2) pause_sel = 0; }

            if (keys.start) {
                // Start = resume directly
                game->paused = false;
                game->flow   = STATE_PLAYING;
                pause_sel    = 0;
            } else if (keys.a) {
                switch (pause_sel) {
                    case 0:  // Resume
                        game->paused = false;
                        game->flow   = STATE_PLAYING;
                        break;
                    case 1:  // Restart (same faction)
                        campaign_start_with_faction(game, game->player_faction);
                        break;
                    case 2:  // Back to faction select
                        game->flow   = STATE_FACTION_SELECT;
                        game->paused = false;
                        fs_sel       = (int)game->player_faction;
                        break;
                }
                pause_sel = 0;
            }
            return pause_sel;
        }

        // ── GAME OVER / VICTORY ──────────────────────────────────────────────
        case STATE_GAME_OVER:
        case STATE_VICTORY: {
            if (keys.d_up || keys.d_down)
                end_sel = !end_sel;

            if (keys.a || keys.start) {
                if (end_sel == 0) {
                    // Retry — same faction
                    campaign_start_with_faction(game, game->player_faction);
                } else {
                    // Change faction
                    game->flow      = STATE_FACTION_SELECT;
                    game->game_over = false;
                    game->victory   = false;
                    fs_sel          = (int)game->player_faction;
                }
                end_sel = 0;
            }
            return end_sel;
        }

        default:
            return 0;
    }
}

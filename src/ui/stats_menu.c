#include "stats_menu.h"
#include "../core/engine.h"
#include <stdio.h>
#include <string.h>

// =============================================================================
// STATS MENU — two pages, switched with Z:
//  Page 0 "Progress": lifetime totals read from the Controller Pak (total
//    kills across every run, best score + fastest clear per campaign).
//  Page 1 "Highlights": the current/last completed run's curated event
//    timeline (ScoreSystem.log, see score.h) — not a frame-accurate replay,
//    just a scrollable list of notable moments. Only meaningful if a run
//    has ended this session, since the log resets at the start of every new
//    run (score_init()) — leaving this screen and starting another run
//    before checking Highlights loses the previous run's log.
// D-pad scrolls the highlights list, B goes back to the main menu.
// =============================================================================

#define STATS_PAGE_COUNT 2
#define HIGHLIGHTS_VISIBLE_ROWS 12

static int page = 0;
static int hl_scroll = 0; // first visible row index in the highlights list

void stats_menu_enter(void) {
    page = 0;
    hl_scroll = 0;
}

void stats_menu_handle_input(GameState* game) {
    joypad_buttons_t kd = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if (kd.z) { page = (page + 1) % STATS_PAGE_COUNT; hl_scroll = 0; }

    if (page == 1) {
        int count = game->score.log.count;
        if (kd.d_up)   hl_scroll--;
        if (kd.d_down) hl_scroll++;
        int max_scroll = count - HIGHLIGHTS_VISIBLE_ROWS;
        if (max_scroll < 0) max_scroll = 0;
        if (hl_scroll < 0) hl_scroll = 0;
        if (hl_scroll > max_scroll) hl_scroll = max_scroll;
    }

    if (kd.b) game->flow = STATE_MAIN_MENU;
}

static void format_mmss(float seconds, char* out, size_t out_size) {
    int total = (int)seconds;
    snprintf(out, out_size, "%02d:%02d", total / 60, total % 60);
}

static const char* event_label(RunEventType type) {
    switch (type) {
        case RUN_EVENT_MAP_STARTED:  return T(STR_EVENT_MAP_STARTED);
        case RUN_EVENT_WAVE_CLEARED: return T(STR_EVENT_WAVE_CLEARED);
        case RUN_EVENT_HERO_KILLED:  return T(STR_EVENT_HERO_KILLED);
        case RUN_EVENT_VICTORY:      return T(STR_EVENT_VICTORY);
        case RUN_EVENT_DEFEAT:       return T(STR_EVENT_DEFEAT);
        default: return "";
    }
}

// Whether `value` means "wave number" for this event type (worth printing);
// for MAP_STARTED it's a map id and for VICTORY it's a campaign id — neither
// reads sensibly as "(wave N)" so those are shown without a suffix.
static bool event_value_is_wave(RunEventType type) {
    return type == RUN_EVENT_WAVE_CLEARED || type == RUN_EVENT_HERO_KILLED || type == RUN_EVENT_DEFEAT;
}

static void render_progress_page(void) {
    int y = 40;

    SaveStatus status = save_system_check();
    const char* pak_msg = status == SAVE_STATUS_READY      ? T(STR_PAK_READY)
                        : status == SAVE_STATUS_UNFORMATTED ? T(STR_PAK_UNFORMATTED)
                                                             : T(STR_PAK_MISSING);
    rdpq_set_prim_color(status == SAVE_STATUS_READY ? RGBA32(150, 255, 150, 255) : RGBA32(255, 180, 100, 255));
    rdpq_text_printf(NULL, 1, 26, y, "%s", pak_msg);
    y += 18;

    if (status != SAVE_STATUS_READY) return;

    GameProgress progress;
    if (!save_read_progress(&progress)) {
        memset(&progress, 0, sizeof(progress));
    }

    rdpq_set_prim_color(RGBA32(255, 215, 0, 255));
    rdpq_text_printf(NULL, 1, 26, y, "%s: %u", T(STR_STATS_TOTAL_KILLS), (unsigned)progress.total_kills);
    y += 18;

    for (int i = 0; i < campaign_get_count(); i++) {
        const Campaign* c = campaign_get(i);
        if (!c) continue;

        rdpq_set_prim_color(RGBA32(200, 200, 230, 255));
        rdpq_text_printf(NULL, 1, 26, y, "%s", c->name);
        y += 12;

        char clear_buf[16];
        if (progress.fastest_clear_seconds[i] > 0) {
            format_mmss((float)progress.fastest_clear_seconds[i], clear_buf, sizeof(clear_buf));
        } else {
            snprintf(clear_buf, sizeof(clear_buf), "%s", T(STR_STATS_NOT_CLEARED));
        }

        rdpq_set_prim_color(RGBA32(160, 200, 255, 255));
        rdpq_text_printf(NULL, 1, 36, y, "%s: %u   %s: %s",
                          T(STR_STATS_BEST_SCORE), (unsigned)progress.best_score[i],
                          T(STR_STATS_FASTEST_CLEAR), clear_buf);
        y += 16;
    }
}

static void render_highlights_page(const GameState* game) {
    const RunLog* log = &game->score.log;

    if (log->count == 0) {
        rdpq_set_prim_color(RGBA32(200, 200, 220, 255));
        rdpq_text_printf(NULL, 1, 26, 60, "%s", T(STR_STATS_NO_RUN));
        return;
    }

    int row_h = 14;
    int y = 40;
    int end = hl_scroll + HIGHLIGHTS_VISIBLE_ROWS;
    if (end > log->count) end = log->count;

    for (int i = hl_scroll; i < end; i++) {
        const RunEvent* ev = &log->events[i];
        char ts[16];
        format_mmss(ev->timestamp, ts, sizeof(ts));

        color_t col = RGBA32(220, 220, 230, 255);
        if (ev->type == RUN_EVENT_VICTORY)      col = RGBA32(150, 255, 150, 255);
        else if (ev->type == RUN_EVENT_DEFEAT)  col = RGBA32(255, 130, 130, 255);
        else if (ev->type == RUN_EVENT_HERO_KILLED) col = RGBA32(255, 215, 0, 255);

        rdpq_set_prim_color(col);
        if (event_value_is_wave(ev->type)) {
            rdpq_text_printf(NULL, 1, 26, y, "%s  %s (wave %d)", ts, event_label(ev->type), ev->value);
        } else {
            rdpq_text_printf(NULL, 1, 26, y, "%s  %s", ts, event_label(ev->type));
        }
        y += row_h;
    }
}

void stats_menu_render(const GameState* game) {
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(10, 10, 22, 255));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    rdpq_set_prim_color(RGBA32(30, 30, 70, 255));
    rdpq_fill_rectangle(20, 8, SCREEN_WIDTH-20, 26);
    const char* page_title = page == 0 ? T(STR_STATS_PAGE_PROGRESS) : T(STR_STATS_PAGE_HIGHLIGHTS);
    rdpq_text_printf(NULL, 1, 30, 24, "%s: %s", T(STR_STATS_TITLE), page_title);

    if (page == 0) render_progress_page();
    else render_highlights_page(game);

    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(10, 10, 25, 220));
    rdpq_fill_rectangle(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_set_prim_color(RGBA32(200, 200, 220, 255));
    rdpq_text_printf(NULL, 1, 15, SCREEN_HEIGHT-4, "%s", T(STR_STATS_HINT));
}

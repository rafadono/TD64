#include "test_harness.h"
#include "../src/systems/score.h"
#include "../src/config/game_config.h"

// =============================================================================
// Tests for src/systems/score.c — pure formulas, no rendering/hardware
// involved except score_render_combo() (compiled in for linkage, never
// called here). Assertions are written against the game_config.h constants
// rather than hardcoded numbers, so balance tuning doesn't spuriously break
// these — they check the FORMULA SHAPE, not specific magic numbers.
// =============================================================================

static void test_init_and_combo(void) {
    SECTION("score_init / combo");
    ScoreSystem s;
    score_init(&s);
    CHECK(s.score == 0);
    CHECK(s.combo_mult == 1);
    CHECK(s.total_kills == 0);
    CHECK(s.total_gold == 0);
    CHECK(s.highest_wave == 0);
    CHECK_NEAR(s.run_elapsed, 0.0f, 0.0001f);
    CHECK(s.log.count == 0);

    // Kill 1: combo x1
    score_on_kill(&s, /*gold*/5, /*score*/50);
    CHECK(s.score == 50 * 1);
    CHECK(s.combo_mult == 2); // increments AFTER applying the current mult
    CHECK(s.total_kills == 1);
    CHECK(s.total_gold == 5);

    // Kill 2: combo x2
    score_on_kill(&s, 5, 50);
    CHECK(s.score == 50 * 1 + 50 * 2);
    CHECK(s.combo_mult == 3);

    // Combo caps at SCORE_COMBO_MAX_MULT regardless of how many more kills
    for (int i = 0; i < 20; i++) score_on_kill(&s, 0, 0);
    CHECK(s.combo_mult == SCORE_COMBO_MAX_MULT);

    // Letting the combo timer expire resets the multiplier back to 1
    score_update(&s, SCORE_COMBO_TIMEOUT + 0.01f);
    CHECK(s.combo_mult == 1);
    CHECK_NEAR(s.combo_timer, 0.0f, 0.0001f);
}

static void test_run_elapsed_accumulates(void) {
    SECTION("run_elapsed");
    ScoreSystem s;
    score_init(&s);
    score_update(&s, 0.5f);
    score_update(&s, 0.25f);
    CHECK_NEAR(s.run_elapsed, 0.75f, 0.0001f);
}

static void test_wave_clear_bonus(void) {
    SECTION("score_on_wave_complete: base bonus, no perfect/speed/penalty");
    ScoreSystem s;
    score_init(&s);
    score_on_wave_start(&s);
    // Duration strictly between the speed-bonus and time-penalty thresholds
    // -> neither bonus nor penalty applies, only the base formula.
    float mid_duration = (SCORE_SPEED_BONUS_THRESHOLD + SCORE_TIME_PENALTY_THRESHOLD) / 2.0f;
    score_update(&s, mid_duration);

    int wave = 3;
    score_on_wave_complete(&s, wave, /*perfect*/false);
    int expected = SCORE_WAVE_CLEAR_BASE + wave * SCORE_WAVE_CLEAR_PER_WAVE;
    CHECK(s.score == expected);
    CHECK(s.highest_wave == wave);
    CHECK(s.wave_start_time == 0.0f); // reset after the wave
    // Not perfect and not a hero-interval wave -> no highlight logged.
    CHECK(s.log.count == 0);
}

static void test_perfect_wave_bonus_and_log(void) {
    SECTION("score_on_wave_complete: perfect bonus + highlight log");
    ScoreSystem s;
    score_init(&s);
    score_on_wave_start(&s);
    float mid_duration = (SCORE_SPEED_BONUS_THRESHOLD + SCORE_TIME_PENALTY_THRESHOLD) / 2.0f;
    score_update(&s, mid_duration);

    int wave = 3; // not a hero-interval wave, so only "perfect" should log it
    score_on_wave_complete(&s, wave, /*perfect*/true);
    int expected = (SCORE_WAVE_CLEAR_BASE + wave * SCORE_WAVE_CLEAR_PER_WAVE)
                 + (SCORE_PERFECT_WAVE_BASE + wave * SCORE_PERFECT_WAVE_PER_WAVE);
    CHECK(s.score == expected);
    CHECK(s.perfect_waves == 1);
    CHECK(s.log.count == 1);
    CHECK(s.log.events[0].type == RUN_EVENT_WAVE_CLEARED);
    CHECK(s.log.events[0].value == wave);
}

static void test_hero_interval_wave_logs_even_if_not_perfect(void) {
    SECTION("score_on_wave_complete: hero-interval wave logs without perfect");
    ScoreSystem s;
    score_init(&s);
    score_on_wave_start(&s);

    int wave = WAVE_HERO_INTERVAL; // e.g. 5 — divisible, not perfect
    CHECK(wave % WAVE_HERO_INTERVAL == 0);
    score_on_wave_complete(&s, wave, /*perfect*/false);
    CHECK(s.log.count == 1);
    CHECK(s.log.events[0].type == RUN_EVENT_WAVE_CLEARED);

    // One past the interval, still not perfect -> should NOT log.
    score_init(&s);
    score_on_wave_start(&s);
    score_on_wave_complete(&s, wave + 1, /*perfect*/false);
    CHECK(s.log.count == 0);
}

static void test_speed_bonus(void) {
    SECTION("score_on_wave_complete: speed bonus");
    ScoreSystem s;
    score_init(&s);
    score_on_wave_start(&s);

    float time_saved = 10.0f;
    score_update(&s, SCORE_SPEED_BONUS_THRESHOLD - time_saved);

    int wave = 1;
    score_on_wave_complete(&s, wave, false);
    int base = SCORE_WAVE_CLEAR_BASE + wave * SCORE_WAVE_CLEAR_PER_WAVE;
    int speed_bonus = (int)(time_saved * SCORE_SPEED_BONUS_PER_SECOND);
    if (speed_bonus > SCORE_SPEED_BONUS_MAX) speed_bonus = SCORE_SPEED_BONUS_MAX;
    CHECK(s.score == base + speed_bonus);
}

static void test_speed_bonus_cap_is_currently_unreachable(void) {
    SECTION("score_on_wave_complete: speed bonus cap vs. current balance constants");
    // Found while writing this test, not asserted as a requirement: with
    // the current constants, the speed bonus can never actually reach
    // SCORE_SPEED_BONUS_MAX (a near-zero clear time saves at most
    // SCORE_SPEED_BONUS_THRESHOLD seconds, capped well below the cap) — so
    // the MAX clamp in score.c is dead code today. This check just keeps
    // that fact visible; if a balance change makes the cap reachable, this
    // simply starts failing as a reminder to add a real cap test.
    long max_possible_speed_bonus = (long)(SCORE_SPEED_BONUS_THRESHOLD * SCORE_SPEED_BONUS_PER_SECOND);
    CHECK(max_possible_speed_bonus < SCORE_SPEED_BONUS_MAX);
}

static void test_time_penalty(void) {
    SECTION("score_on_wave_complete: time penalty");
    ScoreSystem s;
    score_init(&s);
    score_on_wave_start(&s);

    float time_over = 5.0f;
    score_update(&s, SCORE_TIME_PENALTY_THRESHOLD + time_over);

    int wave = 5;
    score_on_wave_complete(&s, wave, false);
    int base = SCORE_WAVE_CLEAR_BASE + wave * SCORE_WAVE_CLEAR_PER_WAVE;
    int penalty = (int)(time_over * SCORE_TIME_PENALTY_PER_SECOND);
    if (penalty > SCORE_TIME_PENALTY_MAX) penalty = SCORE_TIME_PENALTY_MAX;
    CHECK(s.score == base - penalty);
}

static void test_time_penalty_never_takes_score_negative(void) {
    SECTION("score_on_wave_complete: score floors at 0, never negative");
    ScoreSystem s;
    score_init(&s);
    score_on_wave_start(&s);
    // Pre-seed a deep deficit directly (plain struct field) so base_bonus -
    // max_penalty still leaves the total negative — the only way to
    // actually exercise the clamp with the current balance constants,
    // since a real wave's base bonus alone is never smaller than the
    // capped max penalty.
    s.score = -100000;
    score_update(&s, SCORE_TIME_PENALTY_THRESHOLD + 999.0f); // huge overrun -> penalty caps at MAX

    score_on_wave_complete(&s, 1, false);
    CHECK(s.score == 0);
}

static void test_map_complete_bonus(void) {
    SECTION("score_on_map_complete");
    ScoreSystem s;
    score_init(&s);
    s.score = 1000;

    int lives = 5;
    int gold_below_threshold = SCORE_GOLD_REMAINING_THRESHOLD - 1;
    score_on_map_complete(&s, lives, gold_below_threshold, /*difficulty*/2 /*NORMAL*/);
    int expected = (int)((1000 + lives * SCORE_LIFE_REMAINING_MULT) * SCORE_DIFFICULTY_MULT_NORMAL);
    CHECK(s.score == expected);
}

static void test_map_complete_gold_bonus_only_above_threshold(void) {
    SECTION("score_on_map_complete: gold bonus only kicks in at/above threshold");
    ScoreSystem s;
    score_init(&s);
    s.score = 1000;

    int lives = 5;
    int gold_at_threshold = SCORE_GOLD_REMAINING_THRESHOLD;
    score_on_map_complete(&s, lives, gold_at_threshold, /*difficulty*/1 /*EASY*/);
    int expected = (int)((1000 + lives * SCORE_LIFE_REMAINING_MULT
                              + gold_at_threshold * SCORE_GOLD_REMAINING_MULT)
                         * SCORE_DIFFICULTY_MULT_EASY);
    CHECK(s.score == expected);
}

static void test_map_complete_difficulty_scales_whole_score(void) {
    SECTION("score_on_map_complete: difficulty multiplier scales the WHOLE accumulated score");
    // Documented (if easy to miss) behavior: the multiplier isn't applied
    // only to the map-complete bonus, it rescales everything accumulated
    // so far this run. Locking this in so a future refactor can't silently
    // narrow its scope to "just this bonus" without a test noticing.
    ScoreSystem s;
    score_init(&s);
    s.score = 2000; // score from kills/waves earlier in the run

    score_on_map_complete(&s, /*lives*/0, /*gold*/0, /*difficulty*/4 /*EXTREME*/);
    int expected = (int)(2000 * SCORE_DIFFICULTY_MULT_EXTREME);
    CHECK(s.score == expected);
}

static void test_campaign_complete_and_final(void) {
    SECTION("score_on_campaign_complete / score_final");
    ScoreSystem s;
    score_init(&s);
    s.score = 500;
    score_on_campaign_complete(&s);
    CHECK(s.score == 500 + SCORE_CAMPAIGN_COMPLETE_BONUS);

    s.total_kills = 7;
    int before = s.score;
    int final_score = score_final(&s);
    CHECK(final_score == before + 7 * 10);
    CHECK(s.score == final_score);
}

static void test_run_log_basic(void) {
    SECTION("run_log_add / run_log_clear");
    RunLog log;
    run_log_clear(&log);
    CHECK(log.count == 0);

    run_log_add(&log, RUN_EVENT_MAP_STARTED, 0.0f, 2);
    run_log_add(&log, RUN_EVENT_HERO_KILLED, 12.5f, 5);
    CHECK(log.count == 2);
    CHECK(log.events[0].type == RUN_EVENT_MAP_STARTED);
    CHECK(log.events[0].value == 2);
    CHECK(log.events[1].type == RUN_EVENT_HERO_KILLED);
    CHECK_NEAR(log.events[1].timestamp, 12.5f, 0.0001f);
    CHECK(log.events[1].value == 5);

    run_log_clear(&log);
    CHECK(log.count == 0);
}

static void test_run_log_caps_without_overflow(void) {
    SECTION("run_log_add: silently stops at RUN_LOG_MAX_EVENTS, no overflow");
    RunLog log;
    run_log_clear(&log);

    for (int i = 0; i < RUN_LOG_MAX_EVENTS + 10; i++) {
        run_log_add(&log, RUN_EVENT_WAVE_CLEARED, (float)i, i);
    }
    CHECK(log.count == RUN_LOG_MAX_EVENTS);
    // The events that DID fit are the first ones attempted, not overwritten
    // by the ones that didn't fit.
    CHECK(log.events[0].value == 0);
    CHECK(log.events[RUN_LOG_MAX_EVENTS - 1].value == RUN_LOG_MAX_EVENTS - 1);
}

int main(void) {
    test_init_and_combo();
    test_run_elapsed_accumulates();
    test_wave_clear_bonus();
    test_perfect_wave_bonus_and_log();
    test_hero_interval_wave_logs_even_if_not_perfect();
    test_speed_bonus();
    test_speed_bonus_cap_is_currently_unreachable();
    test_time_penalty();
    test_time_penalty_never_takes_score_negative();
    test_map_complete_bonus();
    test_map_complete_gold_bonus_only_above_threshold();
    test_map_complete_difficulty_scales_whole_score();
    test_campaign_complete_and_final();
    test_run_log_basic();
    test_run_log_caps_without_overflow();
    SUMMARY_AND_RETURN();
}

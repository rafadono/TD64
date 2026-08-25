#include "score.h"
#include "../core/engine.h"
#include "../config/game_config.h"
#include <stdio.h>

void run_log_clear(RunLog* log) {
    log->count = 0;
}

void run_log_add(RunLog* log, RunEventType type, float timestamp, int value) {
    // Silently stop once full — a highlights reel doesn't need every event,
    // and a realistic full campaign run stays well under the cap anyway
    // since only perfect/hero-interval waves and a few milestones are logged.
    if (log->count >= RUN_LOG_MAX_EVENTS) return;
    log->events[log->count].type      = type;
    log->events[log->count].timestamp = timestamp;
    log->events[log->count].value     = value;
    log->count++;
}

void score_init(ScoreSystem* s) {
    s->score = 0;
    s->combo_mult = 1;
    s->combo_timer = 0;
    s->kills_this_wave = 0;
    s->perfect_waves = 0;
    s->total_kills = 0;
    s->total_gold = 0;
    s->highest_wave = 0;
    s->wave_start_time = 0.0f;
    s->wave_duration = 0.0f;
    s->run_elapsed = 0.0f;
    run_log_clear(&s->log);
}

void score_update(ScoreSystem* s, float dt) {
    s->run_elapsed += dt;

    // Update combo timer
    if (s->combo_timer > 0) {
        s->combo_timer -= dt;
        if (s->combo_timer <= 0) {
            s->combo_mult = 1;
            s->combo_timer = 0;
        }
    }

    // Track wave duration (wave_start_time is used as a flag: >0 = wave in progress)
    if (s->wave_start_time > 0.0f) {
        s->wave_duration += dt;
    }
}

void score_on_kill(ScoreSystem* s, int gold_val, int score_val) {
    // Base score with combo multiplier
    int combo_bonus = score_val * s->combo_mult;
    s->score += combo_bonus;
    
    // Increase combo
    s->combo_mult++;
    if (s->combo_mult > SCORE_COMBO_MAX_MULT)
        s->combo_mult = SCORE_COMBO_MAX_MULT;
    
    // Reset combo timer
    s->combo_timer = SCORE_COMBO_TIMEOUT;
    
    // Stats
    s->kills_this_wave++;
    s->total_kills++;
    s->total_gold += gold_val;
}

void score_on_wave_start(ScoreSystem* s) {
    s->wave_start_time = 1.0f;  // Flag: wave in progress (score_update accumulates wave_duration)
    s->wave_duration = 0.0f;
    s->kills_this_wave = 0;
}

void score_on_wave_complete(ScoreSystem* s, int wave, bool perfect) {
    // Base wave clear bonus
    int base_bonus = SCORE_WAVE_CLEAR_BASE + wave * SCORE_WAVE_CLEAR_PER_WAVE;
    s->score += base_bonus;
    
    // Perfect wave bonus (no lives lost)
    if (perfect) {
        int perfect_bonus = SCORE_PERFECT_WAVE_BASE + wave * SCORE_PERFECT_WAVE_PER_WAVE;
        s->score += perfect_bonus;
        s->perfect_waves++;
    }
    
    // Speed bonus — if completed quickly
    if (s->wave_duration > 0.0f && s->wave_duration < SCORE_SPEED_BONUS_THRESHOLD) {
        float time_saved = SCORE_SPEED_BONUS_THRESHOLD - s->wave_duration;
        int speed_bonus = (int)(time_saved * SCORE_SPEED_BONUS_PER_SECOND);
        if (speed_bonus > SCORE_SPEED_BONUS_MAX)
            speed_bonus = SCORE_SPEED_BONUS_MAX;
        s->score += speed_bonus;
    }
    
    // Time penalty — if took too long
    if (s->wave_duration > SCORE_TIME_PENALTY_THRESHOLD) {
        float time_over = s->wave_duration - SCORE_TIME_PENALTY_THRESHOLD;
        int penalty = (int)(time_over * SCORE_TIME_PENALTY_PER_SECOND);
        if (penalty > SCORE_TIME_PENALTY_MAX)
            penalty = SCORE_TIME_PENALTY_MAX;
        s->score -= penalty;
        if (s->score < 0) s->score = 0;
    }
    
    // Update highest wave
    if (wave > s->highest_wave)
        s->highest_wave = wave;

    // Highlights reel: only perfect or hero-interval waves are notable
    // enough to log (every wave would flood a "highlights" list).
    if (perfect || wave % WAVE_HERO_INTERVAL == 0) {
        run_log_add(&s->log, RUN_EVENT_WAVE_CLEARED, s->run_elapsed, wave);
    }

    // Reset wave timer
    s->wave_start_time = 0.0f;
    s->wave_duration = 0.0f;
}

void score_on_map_complete(ScoreSystem* s, int lives_remaining, int gold_remaining, int map_difficulty) {
    // Lives remaining bonus
    int life_bonus = lives_remaining * SCORE_LIFE_REMAINING_MULT;
    s->score += life_bonus;
    
    // Gold remaining bonus (only if above threshold)
    if (gold_remaining >= SCORE_GOLD_REMAINING_THRESHOLD) {
        int gold_bonus = gold_remaining * SCORE_GOLD_REMAINING_MULT;
        s->score += gold_bonus;
    }
    
    // Difficulty multiplier
    float diff_mult = SCORE_DIFFICULTY_MULT_EASY;
    switch (map_difficulty) {
        case 1: diff_mult = SCORE_DIFFICULTY_MULT_EASY;    break;
        case 2: diff_mult = SCORE_DIFFICULTY_MULT_NORMAL;  break;
        case 3: diff_mult = SCORE_DIFFICULTY_MULT_HARD;    break;
        case 4: diff_mult = SCORE_DIFFICULTY_MULT_EXTREME; break;
        default: diff_mult = SCORE_DIFFICULTY_MULT_NORMAL; break;
    }
    
    s->score = (int)(s->score * diff_mult);
}

void score_on_campaign_complete(ScoreSystem* s) {
    s->score += SCORE_CAMPAIGN_COMPLETE_BONUS;
}

void score_render_combo(const ScoreSystem* s) {
    if (s->combo_mult < UI_COMBO_DISPLAY_MIN) return;
    
    // Combo display with pulsing effect
    color_t combo_col = s->combo_mult >= SCORE_COMBO_MAX_MULT
        ? RGBA32(255, 100, 255, 255)  // Max combo = purple
        : RGBA32(255, 215, 0, 255);   // Normal = gold
    
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(combo_col);
    rdpq_fill_rectangle(SCREEN_WIDTH/2 - 30, 50,
                        SCREEN_WIDTH/2 + 30, 64);
    
    // Combo multiplier as bars
    for (int i = 0; i < s->combo_mult && i < SCORE_COMBO_MAX_MULT; i++) {
        rdpq_set_prim_color(RGBA32(255, 255, 255, 200));
        rdpq_fill_rectangle(SCREEN_WIDTH/2 - 25 + i*6, 53,
                            SCREEN_WIDTH/2 - 21 + i*6, 61);
    }
}

int score_final(ScoreSystem* s) {
    // The perfect-wave bonus was already added incrementally in score_on_wave_complete;
    // here we only add the end-of-run bonus for total kills (not covered anywhere
    // else, so this isn't double counting).
    s->score += s->total_kills * 10;
    return s->score;
}

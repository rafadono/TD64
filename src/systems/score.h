#ifndef SCORE_H
#define SCORE_H
#include <stdbool.h>
typedef struct {
    int score, combo_mult, kills_this_wave, perfect_waves;
    int total_kills, total_gold, highest_wave;
    float combo_timer, wave_start_time, wave_duration;
} ScoreSystem;
void score_init(ScoreSystem* s);
void score_update(ScoreSystem* s, float dt);
void score_on_kill(ScoreSystem* s, int gold, int score);
void score_on_wave_start(ScoreSystem* s);
void score_on_wave_complete(ScoreSystem* s, int wave, bool perfect);
void score_on_map_complete(ScoreSystem* s, int lives, int gold, int difficulty);
void score_on_campaign_complete(ScoreSystem* s);
void score_render_combo(const ScoreSystem* s);
int score_final(ScoreSystem* s);
#endif

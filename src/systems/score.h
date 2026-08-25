#ifndef SCORE_H
#define SCORE_H
#include <stdbool.h>

// =============================================================================
// RUN LOG — a curated event timeline for the current run, used by the
// "last run highlights" viewer (see ui/stats_menu.c). This is NOT a
// frame-accurate replay/spectate system (that would need deterministic RNG
// + recorded input, a much bigger rearchitecture) — it's a short list of
// notable moments (map started, standout waves, hero kills, the run's
// outcome) the player can scroll through after a run ends. RAM-only, reset
// by score_init() at the start of every run, never persisted to the
// Controller Pak.
// =============================================================================
#define RUN_LOG_MAX_EVENTS 48

typedef enum {
    RUN_EVENT_MAP_STARTED,
    RUN_EVENT_WAVE_CLEARED,   // only logged for perfect or hero-interval waves — see score_on_wave_complete
    RUN_EVENT_HERO_KILLED,
    RUN_EVENT_VICTORY,
    RUN_EVENT_DEFEAT,
} RunEventType;

typedef struct {
    RunEventType type;
    float timestamp; // seconds since the run started (ScoreSystem.run_elapsed at log time)
    int   value;      // meaning depends on type: wave number, map id, etc.
} RunEvent;

typedef struct {
    RunEvent events[RUN_LOG_MAX_EVENTS];
    int count; // stops growing past RUN_LOG_MAX_EVENTS — a highlights reel doesn't need every event
} RunLog;

void run_log_clear(RunLog* log);
void run_log_add(RunLog* log, RunEventType type, float timestamp, int value);

typedef struct {
    int score, combo_mult, kills_this_wave, perfect_waves;
    int total_kills, total_gold, highest_wave;
    float combo_timer, wave_start_time, wave_duration;
    float run_elapsed; // seconds since score_init() — run duration clock, used for
                        // both RunLog timestamps and "fastest campaign clear" tracking
    RunLog log;
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

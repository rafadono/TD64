#ifndef CAMPAIGN_H
#define CAMPAIGN_H
#include <stdbool.h>
#include "../config/factions.h"
typedef struct {
    int id;
    char name[48], description[128];
    FactionId player_faction, enemy_faction;
    int map_ids[4], map_count;
} Campaign;
struct GameState; // forward
int campaign_get_count(void);
const Campaign* campaign_get(int id);
void campaign_start(struct GameState* game, int campaign_id);
void campaign_start_with_faction(struct GameState* game, FactionId chosen);
FactionId campaign_get_rival(FactionId f);
void campaign_advance(struct GameState* game);

// =============================================================================
// GAME DIFFICULTY — a player-facing runtime scale for the 4 fixed
// campaigns, layered on top of the DIFFICULTY_* balance constants in
// game_config.h (which stay at 1.0 = "Normal" and remain the dev-tunable
// baseline). Not used by custom maps, which have their own per-map
// difficulty label instead (see save.h) — game_start_custom_map() resets
// this back to GAME_DIFF_NORMAL so a custom map is never affected by
// whatever difficulty was last picked for a campaign.
// =============================================================================
typedef enum {
    GAME_DIFF_EASY = 0,
    GAME_DIFF_NORMAL,
    GAME_DIFF_HARD,
    GAME_DIFF_EXTREME,
    GAME_DIFF_COUNT
} GameDifficulty;

void           game_difficulty_set(GameDifficulty d);
GameDifficulty game_difficulty_get(void);

// Hard requires at least 1 completed campaign, Extreme requires all 4 (see
// GameProgress.campaigns_completed in save.h). Always unlocked if no
// Controller Pak/progress is available, so players without one aren't
// permanently locked out of higher difficulties.
bool game_difficulty_unlocked(GameDifficulty d);

float game_difficulty_enemy_hp_mult(void);
float game_difficulty_enemy_dmg_mult(void);
float game_difficulty_enemy_speed_mult(void);
float game_difficulty_tower_cost_mult(void);
float game_difficulty_starting_gold_mult(void);
float game_difficulty_starting_lives_mult(void);
#endif

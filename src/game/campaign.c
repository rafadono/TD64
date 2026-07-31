#include "campaign.h"
#include "../core/engine.h"
#include "../config/game_config.h"
#include <string.h>

// =============================================================================
// CAMPAIGNS
//
// Each campaign defines:
//  - Which faction the player uses (as towers)
//  - Which faction attacks (as runners/enemies)
//  - The 3 maps that make it up
//
// "Dawnguard vs Ironbone" means the player places Dawnguard units and the
// enemies that arrive are Ironbone.
//
// =============================================================================

static const Campaign CAMPAIGNS[] = {
    {
        .id             = 0,
        .name           = "Dawn's Defense",
        .description    = "Defend the realm as Dawnguard\nagainst the Ironbone invasion.",
        .player_faction = FACTION_DAWNGUARD,
        .enemy_faction  = FACTION_IRONBONE,
        .map_ids        = { 0, 1, 2, -1 },
        .map_count      = 3
    },
    {
        .id             = 1,
        .name           = "Iron Tide",
        .description    = "Lead the Ironbone to the front.\nDefeat the Dawnguard on their own soil.",
        .player_faction = FACTION_IRONBONE,
        .enemy_faction  = FACTION_DAWNGUARD,
        .map_ids        = { 1, 0, 2, -1 },
        .map_count      = 3
    },
    {
        .id             = 2,
        .name           = "Ashclaw Rampage",
        .description    = "The Ashclaw attack from the east.\nVeilstorm tries to stop them.",
        .player_faction = FACTION_ASHCLAW,
        .enemy_faction  = FACTION_VEILSTORM,
        .map_ids        = { 2, 3, 1, -1 },
        .map_count      = 3
    },
    {
        .id             = 3,
        .name           = "Storm the Gates",
        .description    = "Veilstorm defends its arcane tower.\nAshclaw advances relentlessly.",
        .player_faction = FACTION_VEILSTORM,
        .enemy_faction  = FACTION_ASHCLAW,
        .map_ids        = { 3, 2, 0, -1 },
        .map_count      = 3
    }
};

// Rival assigned to each playable faction
static const FactionId FACTION_RIVAL[FACTION_COUNT] = {
    [FACTION_DAWNGUARD] = FACTION_IRONBONE,
    [FACTION_IRONBONE]  = FACTION_DAWNGUARD,
    [FACTION_ASHCLAW]   = FACTION_VEILSTORM,
    [FACTION_VEILSTORM] = FACTION_ASHCLAW,
};

// Campaign that corresponds to each faction as the player
static const int FACTION_TO_CAMPAIGN[FACTION_COUNT] = {
    [FACTION_DAWNGUARD] = 0,
    [FACTION_IRONBONE]  = 1,
    [FACTION_ASHCLAW]   = 2,
    [FACTION_VEILSTORM] = 3,
};

// Starts the correct campaign for the faction the player picked
void campaign_start_with_faction(GameState* game, FactionId chosen) {
    int camp_id = FACTION_TO_CAMPAIGN[chosen];
    campaign_start(game, camp_id);
}

FactionId campaign_get_rival(FactionId f) {
    return FACTION_RIVAL[f];
}

#define CAMPAIGN_COUNT (int)(sizeof(CAMPAIGNS) / sizeof(CAMPAIGNS[0]))

int campaign_get_count(void) {
    return CAMPAIGN_COUNT;
}

const Campaign* campaign_get(int id) {
    if (id < 0 || id >= CAMPAIGN_COUNT) return NULL;
    return &CAMPAIGNS[id];
}

void campaign_start(GameState* game, int campaign_id) {
    const Campaign* c = campaign_get(campaign_id);
    if (!c) return;

    game->campaign_id     = campaign_id;
    game->player_faction  = c->player_faction;
    game->enemy_faction   = c->enemy_faction;
    game->current_map_index = 0;

    // Load first map
    map_load(&game->map, c->map_ids[0]);

    // Init game values from map
    game->gold  = (int)(game->map.starting_gold  * DIFFICULTY_STARTING_GOLD_MULT);
    game->lives = (int)(game->map.starting_lives * DIFFICULTY_STARTING_LIVES_MULT);
    game->lives_wave_start = game->lives;
    game->wave  = 0;
    game->enemies_remaining = 0;
    game->wave_clear_handled = false;
    memset(game->spawn_remaining, 0, sizeof(game->spawn_remaining));
    game->spawn_hero_bonus     = 0;
    game->spawn_timer          = 0.0f;
    game->spawn_ev_double_gold = false;
    game->spawn_ev_speed_boost = false;
    game->spawn_ev_hp_boost    = false;
    game->game_over = false;
    game->victory   = false;
    game->paused    = false;

    score_init(&game->score);
    camera_init(&game->camera);
    entities_clear();

    game->flow = STATE_PLAYING;
}

void campaign_advance(GameState* game) {
    const Campaign* c = campaign_get(game->campaign_id);
    if (!c) return;

    // Tally the map being left BEFORE overwriting game->map with the next one
    score_on_map_complete(&game->score, game->lives, game->gold, game->map.difficulty);

    game->current_map_index++;

    if (game->current_map_index >= c->map_count) {
        // Campaign finished!
        score_on_campaign_complete(&game->score);
        score_final(&game->score);
        game->victory = true;
        game->flow    = STATE_VICTORY;

        // Persist progress to the Controller Pak (if one is ready). Status
        // is checked on demand here instead of cached across screens —
        // finishing a campaign is a rare event, not a hot path.
        if (save_system_check() == SAVE_STATUS_READY) {
            GameProgress progress;
            if (!save_read_progress(&progress)) {
                memset(&progress, 0, sizeof(progress));
            }
            if ((uint32_t)game->score.score > progress.best_score[game->campaign_id]) {
                progress.best_score[game->campaign_id] = (uint32_t)game->score.score;
            }
            progress.campaigns_completed |= (1 << game->campaign_id);
            save_write_progress(&progress);
        }
        return;
    }

    // Next map
    int map_id = c->map_ids[game->current_map_index];
    map_load(&game->map, map_id);

    // Keep gold and score, reset lives
    game->lives = (int)(game->map.starting_lives * DIFFICULTY_STARTING_LIVES_MULT);
    game->lives_wave_start = game->lives;
    game->wave  = 0;
    game->enemies_remaining = 0;
    game->wave_clear_handled = false;
    memset(game->spawn_remaining, 0, sizeof(game->spawn_remaining));
    game->spawn_hero_bonus     = 0;
    game->spawn_timer          = 0.0f;
    game->spawn_ev_double_gold = false;
    game->spawn_ev_speed_boost = false;
    game->spawn_ev_hp_boost    = false;
    game->game_over = false;
    game->paused    = false;

    entities_clear();
    game->flow = STATE_PLAYING;
}

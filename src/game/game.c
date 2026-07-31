#include "game.h"
#include "../core/engine.h"
#include "../config/game_config.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Wave composition: how many of each unit to spawn per wave number
// Scouts appear first, then warriors, then archers, then mages, then tanks
// Boss (hero) appears every WAVE_HERO_INTERVAL waves
//
// Formula (documented in game_config.h): BASE + (wave - START_WAVE) * SCALE
// Archer/Mage/Tank also divide progress every N waves before scaling (this
// IS documented in the comments next to each SCALE constant in
// game_config.h — "see code").

static int wave_count_for_type(UnitType t, int wave) {
    switch (t) {
        case UNIT_SCOUT:
            if (wave < WAVE_SCOUT_START_WAVE) return 0;
            return WAVE_SCOUT_BASE + (wave - WAVE_SCOUT_START_WAVE) * WAVE_SCOUT_SCALE;
        case UNIT_WARRIOR:
            if (wave < WAVE_WARRIOR_START_WAVE) return 0;
            return WAVE_WARRIOR_BASE + (wave - WAVE_WARRIOR_START_WAVE) * WAVE_WARRIOR_SCALE;
        case UNIT_ARCHER:
            if (wave < WAVE_ARCHER_START_WAVE) return 0;
            return WAVE_ARCHER_BASE + (wave - WAVE_ARCHER_START_WAVE) / 2 * WAVE_ARCHER_SCALE;
        case UNIT_MAGE:
            if (wave < WAVE_MAGE_START_WAVE) return 0;
            return WAVE_MAGE_BASE + (wave - WAVE_MAGE_START_WAVE) / 3 * WAVE_MAGE_SCALE;
        case UNIT_TANK:
            if (wave < WAVE_TANK_START_WAVE) return 0;
            return WAVE_TANK_BASE + (wave - WAVE_TANK_START_WAVE) / 4 * WAVE_TANK_SCALE;
        default: return 0;
    }
}

void game_init_from_campaign(GameState* game, int campaign_id) {
    campaign_start(game, campaign_id);
}

void game_start_custom_map(GameState* game, FactionId faction, const CustomMapSave* saved) {
    game->campaign_id       = -1;
    game->player_faction    = faction;
    game->enemy_faction     = (FactionId)saved->enemy_faction; // chosen in the editor, not necessarily the "rival"
    game->current_map_index = 0;
    game->is_custom_map     = true;

    map_load_custom(&game->map, saved);

    // Custom maps use the gold/lives the player configured in the editor
    // as-is, without the global difficulty multiplier (those values are
    // already the player's explicit choice).
    game->gold  = game->map.starting_gold;
    game->lives = game->map.starting_lives;
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

// -----------------------------------------------------------------------------
// Staggered spawning: instead of instantiating the whole wave in a single
// frame (a potential hitch on large waves), spawn one unit at a time every
// WAVE_SPAWN_INTERVAL seconds (WAVE_SPAWN_INTERVAL_BOSS for the hero).
// DEBUG_INSTANT_WAVES restores the old behavior (everything at once) for
// testing.
// -----------------------------------------------------------------------------
static void game_spawn_one(GameState* game, UnitType t) {
    Entity* r = unit_spawn_runner(game->enemy_faction, t, &game->map.runner_path, game->wave);
    if (!r) return; // pool full; spawn_remaining is not decremented, will retry

    game->spawn_remaining[t]--;

    if (game->spawn_ev_double_gold) r->gold_reward *= 2;
    if (game->spawn_ev_speed_boost) { r->base_speed *= 1.3f; r->speed = r->base_speed; }
    if (game->spawn_ev_hp_boost)    { r->hp_max = (int)(r->hp_max * 1.5f); r->hp = r->hp_max; }

    if (t == UNIT_HERO) {
        bool is_bonus = game->spawn_hero_bonus > 0;
        if (is_bonus) game->spawn_hero_bonus--;
        floating_text_spawn(is_bonus ? "MINI-BOSS!" : "BOSS!",
            SCREEN_WIDTH/2 - (is_bonus ? 25 : 15), SCREEN_HEIGHT/2 - (is_bonus ? 45 : 30),
            is_bonus ? RGBA32(255, 120, 255, 255) : RGBA32(255, 50, 50, 255));
        camera_shake(&game->camera, 10.0f, 0.5f);
    }
}

static void game_process_spawns(GameState* game, float dt) {
    if (debug.instant_waves) {
        // Debug mode: instantiate the whole remaining queue at once.
        for (int t = 0; t < UNIT_TYPE_COUNT; t++) {
            int guard = game->spawn_remaining[t]; // avoids an infinite loop if the pool is full
            while (game->spawn_remaining[t] > 0 && guard-- > 0) {
                int before = game->spawn_remaining[t];
                game_spawn_one(game, (UnitType)t);
                if (game->spawn_remaining[t] == before) break; // couldn't spawn (pool full)
            }
        }
        return;
    }

    game->spawn_timer -= dt;
    if (game->spawn_timer > 0.0f) return;

    for (int t = 0; t < UNIT_TYPE_COUNT; t++) {
        if (game->spawn_remaining[t] <= 0) continue;
        int before = game->spawn_remaining[t];
        game_spawn_one(game, (UnitType)t);
        // If the pool was full, retry soon without losing the planned slot.
        game->spawn_timer = (game->spawn_remaining[t] == before) ? 0.2f
                           : (t == UNIT_HERO ? WAVE_SPAWN_INTERVAL_BOSS : WAVE_SPAWN_INTERVAL);
        return;
    }
}

void game_update(GameState* game, float dt) {
    if (game->paused || game->game_over || game->victory) return;

    if (debug.infinite_gold && game->gold < 99999) game->gold = 99999;

    game_process_spawns(game, dt);

    score_update(&game->score, dt);
    camera_update(&game->camera, dt);
    entities_update(dt, game);
    particles_update(dt);
    floating_text_update(dt);

    // entities_update() may have ended the run this very frame (last life
    // lost). If that happened, don't keep processing "wave cleared" —
    // otherwise campaign_advance() could clear the game_over/victory flags
    // that were just set.
    if (game->game_over || game->victory) return;

    // Wave cleared?
    if (game->wave > 0 && game->enemies_remaining <= 0 && !game->wave_clear_handled) {
        game->wave_clear_handled = true;

        bool perfect = (game->lives == game->lives_wave_start);
        int  bonus   = ECONOMY_WAVE_CLEAR_BASE_GOLD + game->wave * ECONOMY_WAVE_CLEAR_PER_WAVE;
        game->gold  += bonus;

        score_on_wave_complete(&game->score, game->wave, perfect);

        camera_shake(&game->camera, 6.0f, 0.3f);

        char txt[24];
        snprintf(txt, sizeof(txt), "WAVE CLEAR +%d", bonus);
        floating_text_spawn(txt,
            SCREEN_WIDTH/2 - 30, SCREEN_HEIGHT/2 - 20,
            RGBA32(255, 215, 0, 255));

        audio_play_sfx("levelup");

        // Last wave of the map? Advance campaign
        // (for simplicity: every WAVE_HERO_INTERVAL waves = map complete)
        // Custom maps don't belong to any Campaign — they're played in
        // indefinite waves until the player loses, they never "advance map".
        if (!game->is_custom_map && game->wave % WAVE_HERO_INTERVAL == 0) {
            campaign_advance(game);
        }
    }
}

void game_spawn_wave(GameState* game) {
    if (game->enemies_remaining > 0) return;  // Previous wave not done

    game->wave++;
    game->wave_clear_handled = false;
    game->lives_wave_start = game->lives;

    // Random wave events (independent of each other)
    game->spawn_ev_double_gold = (rand() % 100) < WAVE_RANDOM_DOUBLE_GOLD;
    game->spawn_ev_speed_boost = (rand() % 100) < WAVE_RANDOM_SPEED_BOOST;
    game->spawn_ev_hp_boost    = (rand() % 100) < WAVE_RANDOM_HP_BOOST;
    bool ev_mini_boss          = (rand() % 100) < WAVE_RANDOM_MINI_BOSS;

    // Builds the spawn queue — game_process_spawns() drains it one unit per
    // tick on each game_update() call, instead of instantiating everything here.
    int total = 0;
    for (int t = 0; t < UNIT_TYPE_COUNT - 1; t++) {  // -1: skip hero
        int count = wave_count_for_type((UnitType)t, game->wave);
        game->spawn_remaining[t] = count;
        total += count;
    }

    int hero_count = (game->wave % WAVE_HERO_INTERVAL == 0) ? 1 : 0;
    game->spawn_hero_bonus = ev_mini_boss ? 1 : 0;
    hero_count += game->spawn_hero_bonus;
    game->spawn_remaining[UNIT_HERO] = hero_count;
    total += hero_count;

    game->spawn_timer = 0.0f; // the first one goes out on the next game_update tick

    if (game->spawn_ev_double_gold) {
        floating_text_spawn("2x GOLD WAVE!",
            SCREEN_WIDTH/2 - 35, SCREEN_HEIGHT/2 - 60,
            RGBA32(255, 215, 0, 255));
    }

    score_on_wave_start(&game->score);
    game->enemies_remaining = total;

    debugf("Wave %d: %d enemies queued (enemy faction: %s)\n",
           game->wave, total,
           FACTION_NAME(game->enemy_faction));
}

bool game_can_place(const GameState* game, UnitType t) {
    return game->gold >= game_unit_cost(game->player_faction, t);
}

int game_unit_cost(FactionId f, UnitType t) {
    const UnitStats* s = unit_get_stats(f, t);
    if (!s) return 999999;
    return (int)(s->cost * DIFFICULTY_TOWER_COST_MULT);
}

int game_upgrade_cost(const Entity* e) {
    if (!e) return 999999;
    const UnitStats* s = unit_get_stats(e->faction, e->unit_type);
    if (!s) return 999999;
    return (int)(s->cost * ECONOMY_UPGRADE_BASE_MULT * powf(ECONOMY_UPGRADE_COST_MULT, e->upgrade_tier));
}

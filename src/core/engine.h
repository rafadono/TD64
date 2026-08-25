#ifndef ENGINE_H
#define ENGINE_H

#include <libdragon.h>
#include <stdbool.h>

#include "screen.h"

// Include all subsystem headers
#include "../config/factions.h"
#include "../config/units_data.h"
#include "../entities/animation.h"
#include "../entities/collision.h"
#include "../world/terrain.h"
#include "../world/pathfinding.h"
#include "../world/maps.h"
#include "../game/campaign.h"
#include "../systems/score.h"
#include "../systems/leveling.h"
#include "../systems/effects.h"
#include "../systems/debug.h"
#include "../systems/save.h"
#include "../systems/lang.h"

// =============================================================================
// ENTITY LIMITS
// =============================================================================
#define MAX_ENTITIES       128
// MAX_PARTICLES / MAX_FLOATING_TEXTS: defined in effects.h (single source of truth)
#define MAX_WAYPOINTS       16

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
typedef struct Entity    Entity;
typedef struct GameState GameState;

// =============================================================================
// GAME STATES (flow control)
// =============================================================================
typedef enum {
    STATE_MAIN_MENU,
    STATE_DIFFICULTY_SELECT,
    STATE_FACTION_SELECT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_VICTORY,
    STATE_CUSTOM_MAP_MENU,
    STATE_MAP_EDITOR,
    STATE_CONTROLS_MENU,
    STATE_STATS_MENU
} GameFlowState;

// =============================================================================
// ENTITY
// =============================================================================
struct Entity {
    bool      active;
    uint32_t  id;
    bool      is_projectile;
    Team      team;
    UnitMode  mode;
    FactionId faction;
    UnitType  unit_type;

    float x, y, w, h;
    float vx, vy;
    float speed;
    float base_speed;

    int   hp;
    int   hp_max;
    int   damage;
    int   base_damage;
    float attack_range;
    float base_range;
    float attack_cooldown;
    float base_cooldown;
    float attack_timer;

    int   current_waypoint;
    Entity* target;
    uint32_t target_id;
    Entity* proj_target;
    uint32_t proj_target_id;
    int     proj_damage;
    uint32_t owner_id;   // id of the tower that fired this projectile (for XP attribution)

    int gold_reward;
    int score_reward;
    int xp_reward;

    UnitLevel level_data;
    int       upgrade_tier;

    float ability_timer;
    bool  ability_ready;

    float slow_timer;
    float slow_mult;

    Animator   anim;
    sprite_t*  sprite;
    color_t    tint;
};

// =============================================================================
// GAME STATE
// =============================================================================
struct GameState {
    GameFlowState flow;

    int        campaign_id;
    FactionId  player_faction;
    FactionId  enemy_faction;
    int        current_map_index;
    MapData    map;

    int   gold;
    int   lives;
    int   lives_wave_start;
    int   wave;
    int   enemies_remaining;
    bool  wave_clear_handled;  // avoids processing the "wave cleared" bonus more than once

    // Staggered spawn queue: how many units of each type are still pending
    // for the current wave, and the timer until the next one.
    // Avoids the hitch of instantiating the whole wave in a single frame.
    int   spawn_remaining[UNIT_TYPE_COUNT];
    int   spawn_hero_bonus;     // how many queued heroes are "mini-boss" (only affects the floating text)
    float spawn_timer;
    bool  spawn_ev_double_gold;
    bool  spawn_ev_speed_boost;
    bool  spawn_ev_hp_boost;

    bool  game_over;
    bool  victory;
    bool  paused;

    // Custom maps (editor + Controller Pak): if is_custom_map is true, this
    // run doesn't belong to any Campaign — game_update() must not call
    // campaign_advance() when a wave clears, since a standalone map has no
    // "next map". pending_custom_map/has_pending_custom_map are the bridge
    // between "chose to play/edit a custom map" and the existing faction
    // select screen (reused as-is).
    bool           is_custom_map;
    bool           has_pending_custom_map;
    CustomMapSave  pending_custom_map;

    ScoreSystem score;
    Camera      camera;

    int         selected_unit_type;
    Entity*     selected_tower;
};

// Include modular sub-system headers
#include "../entities/entities.h"
#include "../game/game.h"
#include "../ui/menu.h"
#include "../ui/ui.h"
#include "../resources/resources.h"
#include "../systems/audio.h"

#endif // ENGINE_H

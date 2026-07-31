#ifndef GAME_H
#define GAME_H
#include <stdbool.h>
#include "../config/factions.h"
#include "../systems/save.h"
struct GameState;
struct Entity;
void game_init_from_campaign(struct GameState* game, int campaign_id);
// Starts a standalone run on a custom map (no Campaign): the enemy faction
// is whatever was chosen in the map editor, not campaign_get_rival(faction).
// Ends in GAME_OVER only — it never "advances map".
void game_start_custom_map(struct GameState* game, FactionId faction, const CustomMapSave* saved);
void game_update(struct GameState* game, float dt);
void game_spawn_wave(struct GameState* game);
bool game_can_place(const struct GameState* game, UnitType t);
int game_unit_cost(FactionId f, UnitType t);
int game_upgrade_cost(const struct Entity* e);
#endif

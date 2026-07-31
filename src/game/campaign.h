#ifndef CAMPAIGN_H
#define CAMPAIGN_H
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
#endif

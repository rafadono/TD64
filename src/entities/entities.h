#ifndef ENTITIES_H
#define ENTITIES_H

#include <stdbool.h>
#include "../config/factions.h"

// Forward declarations
struct GameState;
struct Path;

// Entity type defined in engine.h
struct Entity;

// Entity management
void entities_init(void);
void entities_clear(void);
struct Entity* entity_alloc(void);
int entities_count_active(void);

// Spawning
struct Entity* unit_spawn_tower (FactionId f, UnitType t, float x, float y);
struct Entity* unit_spawn_runner(FactionId f, UnitType t, const struct Path* path, int wave);
struct Entity* projectile_spawn(struct Entity* owner, struct Entity* target);

// Upgrades
bool unit_upgrade(struct Entity* e, struct GameState* game);

// Update and render
void entities_update(float dt, struct GameState* game);
void entities_render(const struct Camera* cam, bool show_debug);

#endif // ENTITIES_H

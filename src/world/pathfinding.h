#ifndef PATHFINDING_H
#define PATHFINDING_H
#include <stdbool.h>
#include "terrain.h"
#define MAX_WAYPOINTS 16
typedef struct { float x, y; } Waypoint;
typedef struct Path { Waypoint points[MAX_WAYPOINTS]; int count; } Path;
struct Entity; // forward
void path_init_straight(Path* p);
void path_init_curve(Path* p);
void path_init_zigzag(Path* p);
void path_init_spiral(Path* p);
// Derives a path by tracing the TERRAIN_PATH cells painted on a custom map's
// grid (see the map editor), instead of using one of the fixed algorithms
// above - this is what gives custom maps free-form waypoints. Returns false
// (and leaves *p* untouched) if the grid doesn't contain a traceable
// corridor from one grid edge to another; the caller should fall back to a
// fixed path type in that case.
bool path_init_from_terrain(Path* p, const TerrainMap* map);
void path_follow(struct Entity* e, const Path* path, float dt);
#endif

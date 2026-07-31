#ifndef COLLISION_H
#define COLLISION_H
#include <stdbool.h>
typedef struct { float x, y, w, h; } AABB;
struct Entity; // forward decl
bool  aabb_intersects(const AABB* a, const AABB* b);
AABB  entity_get_aabb(const struct Entity* e);
float distance_squared(float x1, float y1, float x2, float y2);
#endif

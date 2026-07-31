#include "collision.h"
#include "../core/engine.h"
#include <math.h>
bool aabb_intersects(const AABB* a, const AABB* b) {
    return a->x < b->x+b->w && a->x+a->w > b->x &&
           a->y < b->y+b->h && a->y+a->h > b->y;
}
AABB entity_get_aabb(const Entity* e) {
    return (AABB){e->x, e->y, e->w, e->h};
}
float distance_squared(float x1,float y1,float x2,float y2) {
    float dx=x2-x1, dy=y2-y1; return dx*dx+dy*dy;
}

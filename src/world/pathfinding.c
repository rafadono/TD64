#include "pathfinding.h"
#include "../core/engine.h"
#include <math.h>

void path_init_straight(Path* p) {
    p->count = 4;
    p->points[0] = (Waypoint){-20, 120};
    p->points[1] = (Waypoint){100, 120};
    p->points[2] = (Waypoint){220, 120};
    p->points[3] = (Waypoint){SCREEN_WIDTH + 20, 120};
}

void path_init_curve(Path* p) {
    p->count = 6;
    p->points[0] = (Waypoint){-20,  60};
    p->points[1] = (Waypoint){ 80,  60};
    p->points[2] = (Waypoint){120, 120};
    p->points[3] = (Waypoint){200, 120};
    p->points[4] = (Waypoint){240, 180};
    p->points[5] = (Waypoint){SCREEN_WIDTH + 20, 180};
}

void path_init_zigzag(Path* p) {
    p->count = 8;
    p->points[0] = (Waypoint){-20,  50};
    p->points[1] = (Waypoint){100,  50};
    p->points[2] = (Waypoint){100, 190};
    p->points[3] = (Waypoint){200, 190};
    p->points[4] = (Waypoint){200,  80};
    p->points[5] = (Waypoint){280,  80};
    p->points[6] = (Waypoint){280, 160};
    p->points[7] = (Waypoint){SCREEN_WIDTH + 20, 160};
}

void path_init_spiral(Path* p) {
    // A real spiral (9 points) plus an off-screen entry/exit leg, so the
    // enemy enters and exits at the edge just like the other 3 map types,
    // instead of appearing/disappearing in the middle of the terrain.
    const int spiral_pts = 9;
    float cx = 160, cy = 120;
    float r  = 90;

    p->count = spiral_pts + 2;
    p->points[0] = (Waypoint){-20, cy};

    for (int i = 0; i < spiral_pts; i++) {
        float t = (float)i / (spiral_pts - 1);
        float angle  = t * 3.14159f * 3.0f;
        float radius = r * (1.0f - t * 0.7f);
        p->points[i + 1].x = cx + cosf(angle) * radius;
        p->points[i + 1].y = cy + sinf(angle) * radius;
    }

    p->points[spiral_pts + 1] = (Waypoint){SCREEN_WIDTH + 20, cy};
}

void path_follow(Entity* e, const Path* path, float dt) {
    if (!path || e->current_waypoint >= path->count) {
        e->active = false;
        return;
    }

    // Consumes the frame's travel distance (speed*dt) by advancing through
    // as many waypoints as needed, instead of always moving at full speed
    // toward the current waypoint regardless of remaining distance. The
    // latter allowed "overshoot": if speed*dt exceeded the arrival radius,
    // the enemy would never fall within the threshold and would oscillate
    // forever without advancing to the next waypoint (critical bug on
    // late, high-speed waves).
    float remaining = e->speed * dt;

    while (remaining > 0.0f && e->current_waypoint < path->count) {
        Waypoint tgt = path->points[e->current_waypoint];
        float dx = tgt.x - e->x;
        float dy = tgt.y - e->y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist <= remaining) {
            e->x = tgt.x;
            e->y = tgt.y;
            remaining -= dist;
            e->current_waypoint++;
        } else {
            if (dist > 0.001f) {
                e->vx = (dx / dist) * e->speed;
                e->vy = (dy / dist) * e->speed;
                e->x += (dx / dist) * remaining;
                e->y += (dy / dist) * remaining;
            }
            remaining = 0.0f;
        }
    }

    if (e->current_waypoint >= path->count) {
        e->active = false;
    }
}

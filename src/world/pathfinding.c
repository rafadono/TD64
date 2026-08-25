#include "pathfinding.h"
#include "../core/engine.h"
#include <math.h>
#include <string.h>

#define PF_GRID_W (SCREEN_WIDTH  / TERRAIN_GRID_SIZE)
#define PF_GRID_H (SCREEN_HEIGHT / TERRAIN_GRID_SIZE)

static bool pf_is_path(const TerrainMap* map, int gx, int gy) {
    if (gx < 0 || gx >= PF_GRID_W || gy < 0 || gy >= PF_GRID_H) return false;
    return map->grid[gx][gy] == TERRAIN_PATH;
}

static bool pf_on_edge(int gx, int gy) {
    return gx == 0 || gx == PF_GRID_W - 1 || gy == 0 || gy == PF_GRID_H - 1;
}

// Extends the traced corridor with one off-screen waypoint so the runner
// enters/exits past the visible map instead of popping in/out mid-terrain,
// matching the fixed path types. Priority order (top/bottom/left/right)
// only matters for a corner cell, where any choice is equally valid.
static Waypoint pf_offscreen_extension(int gx, int gy, float wx, float wy) {
    if (gy == 0)                return (Waypoint){ wx, -20 };
    if (gy == PF_GRID_H - 1)    return (Waypoint){ wx, SCREEN_HEIGHT + 20 };
    if (gx == 0)                return (Waypoint){ -20, wy };
    return (Waypoint){ SCREEN_WIDTH + 20, wy };
}

void path_scale(Path* p, float sx, float sy) {
    for (int i = 0; i < p->count; i++) {
        p->points[i].x *= sx;
        p->points[i].y *= sy;
    }
}

bool path_init_from_terrain(Path* p, const TerrainMap* map) {
    // Find the first PATH cell touching a grid edge, scanning top row, then
    // bottom row, then left column, then right column.
    int start_gx = -1, start_gy = -1;
    for (int gx = 0; gx < PF_GRID_W && start_gx < 0; gx++) {
        if (pf_is_path(map, gx, 0))              { start_gx = gx; start_gy = 0; }
        else if (pf_is_path(map, gx, PF_GRID_H-1)) { start_gx = gx; start_gy = PF_GRID_H-1; }
    }
    for (int gy = 0; gy < PF_GRID_H && start_gx < 0; gy++) {
        if (pf_is_path(map, 0, gy))              { start_gx = 0; start_gy = gy; }
        else if (pf_is_path(map, PF_GRID_W-1, gy)) { start_gx = PF_GRID_W-1; start_gy = gy; }
    }
    if (start_gx < 0) return false; // no PATH cell touches the border at all

    // Walk the painted corridor: at each step, move to the first unvisited
    // orthogonal PATH neighbor. This assumes a single-width, non-branching
    // corridor (a reasonable hand-painted shape); a branch just means one
    // branch gets ignored rather than followed, which is an acceptable
    // limitation for a free-form editor feature.
    static bool visited[PF_GRID_W][PF_GRID_H];
    memset(visited, 0, sizeof(visited));

    int trace_gx[MAX_WAYPOINTS - 2];
    int trace_gy[MAX_WAYPOINTS - 2];
    int trace_count = 0;

    int gx = start_gx, gy = start_gy;
    visited[gx][gy] = true;
    trace_gx[trace_count] = gx; trace_gy[trace_count] = gy; trace_count++;

    static const int DX[4] = {  0, 0, -1, 1 };
    static const int DY[4] = { -1, 1,  0, 0 };

    while (trace_count < MAX_WAYPOINTS - 2) {
        int next_gx = -1, next_gy = -1;
        for (int d = 0; d < 4; d++) {
            int nx = gx + DX[d], ny = gy + DY[d];
            if (pf_is_path(map, nx, ny) && !visited[nx][ny]) {
                next_gx = nx; next_gy = ny;
                break;
            }
        }
        if (next_gx < 0) break; // end of the corridor

        gx = next_gx; gy = next_gy;
        visited[gx][gy] = true;
        trace_gx[trace_count] = gx; trace_gy[trace_count] = gy; trace_count++;
    }

    if (trace_count < 2) return false; // too short to be a usable path
    // The corridor must reach a second grid edge to have a valid exit - a
    // dead end in the middle of the map has nowhere sensible to extend to.
    if (!pf_on_edge(trace_gx[trace_count-1], trace_gy[trace_count-1])) return false;

    // Build the final Path: off-screen entry, every traced cell's center,
    // off-screen exit.
    float first_wx = trace_gx[0] * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE/2.0f;
    float first_wy = trace_gy[0] * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE/2.0f;
    float last_wx  = trace_gx[trace_count-1] * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE/2.0f;
    float last_wy  = trace_gy[trace_count-1] * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE/2.0f;

    int n = 0;
    p->points[n++] = pf_on_edge(trace_gx[0], trace_gy[0])
        ? pf_offscreen_extension(trace_gx[0], trace_gy[0], first_wx, first_wy)
        : (Waypoint){ first_wx, first_wy };

    for (int i = 0; i < trace_count; i++) {
        p->points[n++] = (Waypoint){
            trace_gx[i] * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE/2.0f,
            trace_gy[i] * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE/2.0f
        };
    }

    p->points[n++] = pf_offscreen_extension(trace_gx[trace_count-1], trace_gy[trace_count-1], last_wx, last_wy);

    p->count = n;
    return true;
}

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

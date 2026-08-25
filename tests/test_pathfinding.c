#include "test_harness.h"
#include "../src/core/engine.h" // pulls in pathfinding.h + terrain.h + a full Entity definition
#include <string.h>

// =============================================================================
// Tests for src/world/pathfinding.c — path_follow() in particular is where
// this session found and fixed a real bug class (diagonal movement running
// at sqrt(2)x speed because dx/dy weren't normalized by distance together).
// These tests lock in that the fix holds and that a fast-moving entity
// advances through multiple waypoints in one frame instead of overshooting
// and oscillating (the "critical bug on late, high-speed waves" the code
// comment describes).
// =============================================================================

static Entity make_entity(float x, float y, float speed, int current_waypoint) {
    Entity e;
    memset(&e, 0, sizeof(e));
    e.x = x; e.y = y;
    e.speed = speed;
    e.current_waypoint = current_waypoint;
    e.active = true;
    return e;
}

static void test_path_scale(void) {
    SECTION("path_scale");
    Path p;
    p.count = 3;
    p.points[0] = (Waypoint){ 10, 20 };
    p.points[1] = (Waypoint){ -5, 40 };
    p.points[2] = (Waypoint){ 100, 0 };

    path_scale(&p, 2.0f, 3.0f);

    CHECK_NEAR(p.points[0].x, 20.0f, 0.0001f);
    CHECK_NEAR(p.points[0].y, 60.0f, 0.0001f);
    CHECK_NEAR(p.points[1].x, -10.0f, 0.0001f);
    CHECK_NEAR(p.points[1].y, 120.0f, 0.0001f);
    CHECK_NEAR(p.points[2].x, 200.0f, 0.0001f);
    CHECK_NEAR(p.points[2].y, 0.0f, 0.0001f);
}

static void test_path_follow_snaps_to_reached_waypoint(void) {
    SECTION("path_follow: reaching a waypoint exactly snaps to it and advances");
    Path p;
    p.count = 2;
    p.points[0] = (Waypoint){ 10, 0 };
    p.points[1] = (Waypoint){ 200, 0 };

    Entity e = make_entity(0, 0, /*speed*/10, 0);
    path_follow(&e, &p, /*dt*/1.0f); // remaining = 10, exactly the distance to waypoint 0

    CHECK_NEAR(e.x, 10.0f, 0.0001f);
    CHECK_NEAR(e.y, 0.0f, 0.0001f);
    CHECK(e.current_waypoint == 1);
    CHECK(e.active == true);
}

static void test_path_follow_advances_through_multiple_waypoints_in_one_frame(void) {
    SECTION("path_follow: a big frame step consumes several close waypoints, doesn't skip/oscillate");
    Path p;
    p.count = 3;
    p.points[0] = (Waypoint){ 10, 0 };
    p.points[1] = (Waypoint){ 20, 0 };
    p.points[2] = (Waypoint){ 100, 0 };

    Entity e = make_entity(0, 0, /*speed*/15, 0);
    path_follow(&e, &p, /*dt*/1.0f); // remaining = 15: reaches wp0 (10, remaining=5), then 5 short of wp1

    CHECK_NEAR(e.x, 15.0f, 0.0001f); // 10 (snapped to wp0) + 5 (leftover travel toward wp1)
    CHECK_NEAR(e.y, 0.0f, 0.0001f);
    CHECK(e.current_waypoint == 1); // consumed wp0, still en route to wp1 — not skipped past it
    CHECK(e.active == true);
}

static void test_path_follow_diagonal_speed_matches_axis_aligned_speed(void) {
    SECTION("path_follow: diagonal movement travels the same distance per frame as axis-aligned");
    // Regression guard for the exact bug class flagged this session: moving
    // toward a diagonal target must NOT cover sqrt(2)x more ground than
    // moving toward a same-distance axis-aligned target, for the same
    // speed*dt budget.
    Path diag;
    diag.count = 1;
    diag.points[0] = (Waypoint){ 30, 40 }; // distance 50 from origin

    Entity e_diag = make_entity(0, 0, /*speed*/25, 0);
    path_follow(&e_diag, &diag, /*dt*/1.0f); // remaining = 25, half the distance to target

    float dist_traveled_diag = sqrtf(e_diag.x * e_diag.x + e_diag.y * e_diag.y);
    CHECK_NEAR(dist_traveled_diag, 25.0f, 0.01f);

    Path straight;
    straight.count = 1;
    straight.points[0] = (Waypoint){ 50, 0 }; // same total distance (50), axis-aligned

    Entity e_straight = make_entity(0, 0, /*speed*/25, 0);
    path_follow(&e_straight, &straight, 1.0f);

    float dist_traveled_straight = sqrtf(e_straight.x * e_straight.x + e_straight.y * e_straight.y);
    CHECK_NEAR(dist_traveled_straight, 25.0f, 0.01f);

    // Both cover the same distance for the same speed*dt, regardless of direction.
    CHECK_NEAR(dist_traveled_diag, dist_traveled_straight, 0.01f);
}

static void test_path_follow_deactivates_when_path_exhausted(void) {
    SECTION("path_follow: current_waypoint already past the end deactivates immediately");
    Path p;
    p.count = 2;
    p.points[0] = (Waypoint){ 10, 0 };
    p.points[1] = (Waypoint){ 20, 0 };

    Entity e = make_entity(5, 5, 10, /*current_waypoint*/2); // == p.count already
    path_follow(&e, &p, 1.0f);

    CHECK(e.active == false);
    CHECK_NEAR(e.x, 5.0f, 0.0001f); // untouched — the early-return branch, no movement
}

// ---- path_init_from_terrain ----

static void clear_grid(TerrainMap* map) {
    memset(map, 0, sizeof(*map)); // TERRAIN_GRASS == 0, a safe "not path" default
}

#define GRID_W (SCREEN_WIDTH / TERRAIN_GRID_SIZE)
#define GRID_H (SCREEN_HEIGHT / TERRAIN_GRID_SIZE)

static void test_terrain_trace_short_corridor_along_an_edge_succeeds(void) {
    SECTION("path_init_from_terrain: a short corridor touching an edge throughout traces successfully");
    // Kept well under MAX_WAYPOINTS-2 cells on purpose — see the next test
    // for what happens to a corridor that ISN'T short enough.
    TerrainMap map;
    clear_grid(&map);
    int gy_start = 3, gy_end = 10; // 8 cells along the left edge column (gx=0)
    for (int gy = gy_start; gy <= gy_end; gy++) map.grid[0][gy] = TERRAIN_PATH;

    Path p;
    bool ok = path_init_from_terrain(&p, &map);

    CHECK(ok == true);
    int traced_cells = gy_end - gy_start + 1;
    CHECK(p.count == traced_cells + 2); // + off-screen entry/exit

    // Both ends sit on the left edge (gx=0) the whole way, so both
    // extensions push off-screen to the left.
    CHECK(p.points[0].x < 0);
    CHECK(p.points[p.count - 1].x < 0);

    // Second point is the real world-space center of the first traced cell.
    float expected_x = 0 * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE / 2.0f;
    float expected_y = gy_start * TERRAIN_GRID_SIZE + TERRAIN_GRID_SIZE / 2.0f;
    CHECK_NEAR(p.points[1].x, expected_x, 0.0001f);
    CHECK_NEAR(p.points[1].y, expected_y, 0.0001f);
}

static void test_terrain_trace_corridor_longer_than_the_waypoint_cap_fails(void) {
    SECTION("path_init_from_terrain: a corridor longer than MAX_WAYPOINTS-2 cells fails, even though it validly spans two edges");
    // Discovered while writing these tests: the trace buffer is sized
    // MAX_WAYPOINTS-2 (14) cells, so a full-width straight corridor (20
    // cells, GRID_W) never finishes tracing and falls back to a preset path
    // — worth knowing for anyone painting a very long free-form corridor in
    // the map editor.
    TerrainMap map;
    clear_grid(&map);
    int row = GRID_H / 2;
    for (int gx = 0; gx < GRID_W; gx++) map.grid[gx][row] = TERRAIN_PATH; // 20 cells > 14

    Path p;
    p.count = 999; // sentinel — must be untouched on failure
    bool ok = path_init_from_terrain(&p, &map);

    CHECK(ok == false);
    CHECK(p.count == 999);
}

static void test_terrain_trace_dead_end_fails_and_leaves_path_untouched(void) {
    SECTION("path_init_from_terrain: a corridor that never reaches a second edge fails");
    TerrainMap map;
    clear_grid(&map);
    int row = GRID_H / 2;
    // Starts on the left edge but stops well short of the right edge.
    for (int gx = 0; gx < GRID_W / 2; gx++) map.grid[gx][row] = TERRAIN_PATH;

    Path p;
    p.count = 999; // sentinel — must be untouched on failure

    bool ok = path_init_from_terrain(&p, &map);
    CHECK(ok == false);
    CHECK(p.count == 999);
}

static void test_terrain_trace_empty_grid_fails(void) {
    SECTION("path_init_from_terrain: no PATH tiles at all fails");
    TerrainMap map;
    clear_grid(&map);

    Path p;
    p.count = 999;
    bool ok = path_init_from_terrain(&p, &map);
    CHECK(ok == false);
    CHECK(p.count == 999);
}

static void test_terrain_trace_isolated_single_cell_fails(void) {
    SECTION("path_init_from_terrain: a single isolated edge cell (too short) fails");
    TerrainMap map;
    clear_grid(&map);
    map.grid[0][3] = TERRAIN_PATH; // on the left edge, but alone — no corridor

    Path p;
    p.count = 999;
    bool ok = path_init_from_terrain(&p, &map);
    CHECK(ok == false);
    CHECK(p.count == 999);
}

int main(void) {
    test_path_scale();
    test_path_follow_snaps_to_reached_waypoint();
    test_path_follow_advances_through_multiple_waypoints_in_one_frame();
    test_path_follow_diagonal_speed_matches_axis_aligned_speed();
    test_path_follow_deactivates_when_path_exhausted();
    test_terrain_trace_short_corridor_along_an_edge_succeeds();
    test_terrain_trace_corridor_longer_than_the_waypoint_cap_fails();
    test_terrain_trace_dead_end_fails_and_leaves_path_untouched();
    test_terrain_trace_empty_grid_fails();
    test_terrain_trace_isolated_single_cell_fails();
    SUMMARY_AND_RETURN();
}

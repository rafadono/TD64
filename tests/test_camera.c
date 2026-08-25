#include "test_harness.h"
#include "../src/core/engine.h"
#include "../src/config/game_config.h" // CAMERA_SHAKE_MULTIPLIER

// =============================================================================
// Tests for the Camera part of src/systems/effects.c —
// camera_ensure_visible() is the function that drives ALL camera scrolling
// this session's camera/bigger-maps work introduced (see README's "Camera
// and Bigger Maps" section); it was hand-verified logically but never
// exercised by an actual test until now. particles/floating text (the other
// half of effects.c) are rendering-only and not covered here.
// =============================================================================

static void test_camera_apply(void) {
    SECTION("camera_apply: world -> screen offset (scroll + shake)");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 50; cam.scroll_y = 20;
    cam.shake_x = 3; cam.shake_y = -2;

    float x = 100, y = 80;
    camera_apply(&cam, &x, &y);
    CHECK_NEAR(x, 100 - 50 + 3, 0.0001f);
    CHECK_NEAR(y, 80 - 20 - 2, 0.0001f);
}

static void test_ensure_visible_noop_when_already_in_view(void) {
    SECTION("camera_ensure_visible: no-op if the point is already comfortably visible");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 100; cam.scroll_y = 50;

    // Center of the current viewport, well within any margin.
    float world_x = cam.scroll_x + SCREEN_WIDTH / 2.0f;
    float world_y = cam.scroll_y + SCREEN_HEIGHT / 2.0f;
    camera_ensure_visible(&cam, world_x, world_y, 0, 0, 0, 0, 2000, 2000);

    CHECK_NEAR(cam.scroll_x, 100, 0.0001f);
    CHECK_NEAR(cam.scroll_y, 50, 0.0001f);
}

static void test_ensure_visible_scrolls_right_and_down_past_margin(void) {
    SECTION("camera_ensure_visible: scrolls forward just enough when the point crosses the far margin");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 100; cam.scroll_y = 50;

    // 10px beyond the right/bottom edge of the current viewport, no margin.
    float world_x = cam.scroll_x + SCREEN_WIDTH + 10;
    float world_y = cam.scroll_y + SCREEN_HEIGHT + 10;
    camera_ensure_visible(&cam, world_x, world_y, 0, 0, 0, 0, 2000, 2000);

    // Scrolled exactly enough that the point now sits right at the edge.
    CHECK_NEAR(cam.scroll_x, world_x - SCREEN_WIDTH, 0.0001f);
    CHECK_NEAR(cam.scroll_y, world_y - SCREEN_HEIGHT, 0.0001f);
}

static void test_ensure_visible_scrolls_left_and_up_before_margin(void) {
    SECTION("camera_ensure_visible: scrolls back when the point is before the near margin");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 100; cam.scroll_y = 100;

    float world_x = 50; // left of the current viewport (which starts at 100)
    float world_y = 60;
    camera_ensure_visible(&cam, world_x, world_y, 0, 0, 0, 0, 2000, 2000);

    CHECK_NEAR(cam.scroll_x, world_x, 0.0001f);
    CHECK_NEAR(cam.scroll_y, world_y, 0.0001f);
}

static void test_ensure_visible_respects_margins(void) {
    SECTION("camera_ensure_visible: margins shrink the usable viewport before scrolling kicks in");
    Camera cam;
    camera_init(&cam);
    // Start away from 0 so the margin adjustment below stays positive —
    // the function's own negative-scroll clamp would otherwise mask the
    // exact compensation amount this test is checking.
    cam.scroll_x = 100; cam.scroll_y = 100;

    float world_x = 150; // interior point, far from any x edge — x must not move
    float margin_top = 20, margin_bottom = 10;

    // Exactly at the top margin boundary -> should NOT scroll yet.
    float world_y_at_boundary = cam.scroll_y + margin_top; // 120
    camera_ensure_visible(&cam, world_x, world_y_at_boundary, 0, 0, margin_top, margin_bottom, 2000, 2000);
    CHECK_NEAR(cam.scroll_x, 100, 0.0001f);
    CHECK_NEAR(cam.scroll_y, 100, 0.0001f);

    // One pixel above the margin boundary -> must scroll up to compensate.
    float world_y_past_boundary = world_y_at_boundary - 1; // 119
    camera_ensure_visible(&cam, world_x, world_y_past_boundary, 0, 0, margin_top, margin_bottom, 2000, 2000);
    CHECK_NEAR(cam.scroll_y, world_y_past_boundary - margin_top, 0.0001f); // 99
}

static void test_ensure_visible_clamps_to_zero_zero_for_a_screen_sized_map(void) {
    SECTION("camera_ensure_visible: a map no bigger than one screen always clamps back to (0,0)");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 500; cam.scroll_y = 500; // intentionally out of any sane range

    camera_ensure_visible(&cam, 9999, 9999, 0, 0, 0, 0, /*map_w*/100, /*map_h*/80);

    CHECK_NEAR(cam.scroll_x, 0, 0.0001f);
    CHECK_NEAR(cam.scroll_y, 0, 0.0001f);
}

static void test_ensure_visible_clamps_to_max_scroll_on_a_bigger_map(void) {
    SECTION("camera_ensure_visible: never scrolls past a bigger map's far edge");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 0; cam.scroll_y = 0;

    float map_w = 400, map_h = 300; // bigger than one screen, but not by much
    // A point far beyond what the margin math alone would request —
    // the second (map-bounds) clamp phase must cap it.
    camera_ensure_visible(&cam, 5000, 5000, 0, 0, 0, 0, map_w, map_h);

    CHECK_NEAR(cam.scroll_x, map_w - SCREEN_WIDTH, 0.0001f);
    CHECK_NEAR(cam.scroll_y, map_h - SCREEN_HEIGHT, 0.0001f);
}

static void test_ensure_visible_never_scrolls_negative(void) {
    SECTION("camera_ensure_visible: never scrolls to a negative position");
    Camera cam;
    camera_init(&cam);
    cam.scroll_x = 0; cam.scroll_y = 0;

    // A point "before" the map's own origin — pathological, but the
    // function must stay defensively clamped at 0 regardless.
    camera_ensure_visible(&cam, -50, -50, 0, 0, 0, 0, 2000, 2000);

    CHECK_NEAR(cam.scroll_x, 0, 0.0001f);
    CHECK_NEAR(cam.scroll_y, 0, 0.0001f);
}

static void test_camera_shake_keeps_the_stronger_shake(void) {
    SECTION("camera_shake: a weaker shake doesn't cut a stronger one short");
    Camera cam;
    camera_init(&cam);

    camera_shake(&cam, 10.0f, 1.0f);
    CHECK_NEAR(cam.shake_intensity, 10.0f * CAMERA_SHAKE_MULTIPLIER, 0.0001f);
    CHECK_NEAR(cam.shake_duration, 1.0f, 0.0001f);

    camera_shake(&cam, 2.0f, 5.0f); // weaker but longer — must NOT override
    CHECK_NEAR(cam.shake_intensity, 10.0f * CAMERA_SHAKE_MULTIPLIER, 0.0001f);
    CHECK_NEAR(cam.shake_duration, 1.0f, 0.0001f);

    camera_shake(&cam, 20.0f, 0.5f); // stronger — must override, even if shorter
    CHECK_NEAR(cam.shake_intensity, 20.0f * CAMERA_SHAKE_MULTIPLIER, 0.0001f);
    CHECK_NEAR(cam.shake_duration, 0.5f, 0.0001f);
}

static void test_camera_update_decays_and_clears_shake(void) {
    SECTION("camera_update: shake decays over time and clears when its duration elapses");
    Camera cam;
    camera_init(&cam);
    camera_shake(&cam, 10.0f, 0.2f);

    camera_update(&cam, 0.1f);
    CHECK_NEAR(cam.shake_duration, 0.1f, 0.0001f);
    CHECK_NEAR(cam.shake_intensity, 10.0f * CAMERA_SHAKE_MULTIPLIER * 0.88f, 0.0001f);

    camera_update(&cam, 0.1f); // duration reaches exactly 0 -> shake fully clears
    CHECK_NEAR(cam.shake_duration, 0.0f, 0.0001f);
    CHECK_NEAR(cam.shake_intensity, 0.0f, 0.0001f);
    CHECK_NEAR(cam.shake_x, 0.0f, 0.0001f);
    CHECK_NEAR(cam.shake_y, 0.0f, 0.0001f);
}

static void test_camera_update_noop_with_no_active_shake(void) {
    SECTION("camera_update: no jitter at all while no shake is active");
    Camera cam;
    camera_init(&cam);
    camera_update(&cam, 1.0f);
    CHECK_NEAR(cam.shake_x, 0.0f, 0.0001f);
    CHECK_NEAR(cam.shake_y, 0.0f, 0.0001f);
}

int main(void) {
    test_camera_apply();
    test_ensure_visible_noop_when_already_in_view();
    test_ensure_visible_scrolls_right_and_down_past_margin();
    test_ensure_visible_scrolls_left_and_up_before_margin();
    test_ensure_visible_respects_margins();
    test_ensure_visible_clamps_to_zero_zero_for_a_screen_sized_map();
    test_ensure_visible_clamps_to_max_scroll_on_a_bigger_map();
    test_ensure_visible_never_scrolls_negative();
    test_camera_shake_keeps_the_stronger_shake();
    test_camera_update_decays_and_clears_shake();
    test_camera_update_noop_with_no_active_shake();
    SUMMARY_AND_RETURN();
}

#ifndef EFFECTS_H
#define EFFECTS_H
#include <libdragon.h>
#define MAX_PARTICLES 256
#define MAX_FLOATING_TEXTS 32
// Camera scroll is driven entirely by camera_ensure_visible(), called from
// the D-pad cursor movement in core/main.c — there is deliberately no
// separate manual pan control (e.g. the analog stick). The N64 controller
// has 3 separate hand positions ("wings": D-pad+L, stick+Z+Start, A/B/C+R),
// and the core play loop (D-pad cursor + A to place) already lives entirely
// on the left+right wings; requiring the stick for camera control would
// force a hand off the D-pad mid-build to reach the center wing. Having the
// cursor itself pull the camera along removes that repositioning entirely.
typedef struct Camera {
    float scroll_x, scroll_y;              // persistent world-space position of the viewport's top-left corner
    float shake_x, shake_y;                // transient shake jitter, added on top of scroll (see camera_shake)
    float shake_intensity, shake_duration;
} Camera;
typedef struct {
    bool active;
    float x, y, vx, vy, life, max_life, size, gravity;
    color_t color;
} Particle;
typedef struct {
    bool active;
    char text[16];
    float x, y, vy, life;
    color_t color;
} FloatingText;
void camera_init(Camera* cam);
void camera_update(Camera* cam, float dt);
void camera_shake(Camera* cam, float intensity, float duration);
// Scrolls just enough to bring (world_x, world_y) within the viewport,
// leaving at least the given margin from each edge (e.g. HUD clearance at
// the top/bottom) — a no-op if the point is already comfortably visible.
// Clamped so the viewport never shows past the edges of a map_w x map_h
// world; for a map no bigger than one screen (e.g. every custom map), this
// always clamps back to (0,0), so calling it is harmless there.
void camera_ensure_visible(Camera* cam, float world_x, float world_y,
                            float margin_left, float margin_right,
                            float margin_top, float margin_bottom,
                            float map_w, float map_h);
// World -> screen: subtracts the scroll position and adds the current
// shake jitter. Used by every render pass that draws a world-space
// position (entities, particles, path debug markers, ...).
void camera_apply(const Camera* cam, float* x, float* y);
void particles_init(void);
void particles_update(float dt);
void particles_render(const Camera* cam);
void particles_emit_explosion(float x, float y, color_t color, int count);
void particles_emit_coins(float x, float y, int gold);
void particles_emit_hit(float x, float y, color_t color);
void floating_text_init(void);
void floating_text_spawn(const char* txt, float x, float y, color_t color);
void floating_text_update(float dt);
void floating_text_render(void);
#endif

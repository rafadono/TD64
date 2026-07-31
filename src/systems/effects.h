#ifndef EFFECTS_H
#define EFFECTS_H
#include <libdragon.h>
#define MAX_PARTICLES 256
#define MAX_FLOATING_TEXTS 32
typedef struct Camera { float offset_x, offset_y, shake_intensity, shake_duration; } Camera;
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
void camera_apply(const Camera* cam, float* x, float* y);
void particles_init(void);
void particles_update(float dt);
void particles_render(void);
void particles_emit_explosion(float x, float y, color_t color, int count);
void particles_emit_coins(float x, float y, int gold);
void particles_emit_hit(float x, float y, color_t color);
void floating_text_init(void);
void floating_text_spawn(const char* txt, float x, float y, color_t color);
void floating_text_update(float dt);
void floating_text_render(void);
#endif

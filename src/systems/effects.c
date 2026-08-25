#include "effects.h"
#include "../core/engine.h"
#include "../config/game_config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// =============================================================================
// CAMERA
// =============================================================================

void camera_init(Camera* cam) {
    cam->scroll_x = cam->scroll_y = 0;
    cam->shake_x = cam->shake_y = 0;
    cam->shake_intensity = cam->shake_duration = 0;
}

void camera_update(Camera* cam, float dt) {
    if (cam->shake_duration <= 0) { cam->shake_x = cam->shake_y = 0; return; }
    cam->shake_x = ((rand() % 200) - 100) / 100.0f * cam->shake_intensity;
    cam->shake_y = ((rand() % 200) - 100) / 100.0f * cam->shake_intensity;
    cam->shake_duration  -= dt;
    cam->shake_intensity *= 0.88f;
    if (cam->shake_duration <= 0) {
        cam->shake_x = cam->shake_y = 0;
        cam->shake_intensity = 0;
    }
}

void camera_shake(Camera* cam, float intensity, float duration) {
    intensity *= CAMERA_SHAKE_MULTIPLIER;
    if (intensity > cam->shake_intensity) {
        cam->shake_intensity = intensity;
        cam->shake_duration  = duration;
    }
}

void camera_ensure_visible(Camera* cam, float world_x, float world_y,
                            float margin_left, float margin_right,
                            float margin_top, float margin_bottom,
                            float map_w, float map_h) {
    // Valid scroll range that keeps world_x within
    // [scroll_x+margin_left, scroll_x+SCREEN_WIDTH-margin_right]; only
    // moves scroll_x if it's currently outside that range.
    float min_x = world_x - SCREEN_WIDTH + margin_right;
    float max_x = world_x - margin_left;
    if (cam->scroll_x < min_x) cam->scroll_x = min_x;
    if (cam->scroll_x > max_x) cam->scroll_x = max_x;

    float min_y = world_y - SCREEN_HEIGHT + margin_bottom;
    float max_y = world_y - margin_top;
    if (cam->scroll_y < min_y) cam->scroll_y = min_y;
    if (cam->scroll_y > max_y) cam->scroll_y = max_y;

    // Re-clamp to the map's actual bounds — handles a map no bigger than
    // one screen (scroll must stay at (0,0) regardless of the above), and
    // prevents scrolling past the far edge of a bigger map.
    float max_scroll_x = map_w - SCREEN_WIDTH;  if (max_scroll_x < 0) max_scroll_x = 0;
    float max_scroll_y = map_h - SCREEN_HEIGHT; if (max_scroll_y < 0) max_scroll_y = 0;
    if (cam->scroll_x < 0) cam->scroll_x = 0;
    if (cam->scroll_x > max_scroll_x) cam->scroll_x = max_scroll_x;
    if (cam->scroll_y < 0) cam->scroll_y = 0;
    if (cam->scroll_y > max_scroll_y) cam->scroll_y = max_scroll_y;
}

void camera_apply(const Camera* cam, float* x, float* y) {
    *x = *x - cam->scroll_x + cam->shake_x;
    *y = *y - cam->scroll_y + cam->shake_y;
}

// =============================================================================
// PARTICLES
// =============================================================================

static Particle parts[MAX_PARTICLES];

void particles_init(void) {
    memset(parts, 0, sizeof(parts));
}

static Particle* part_alloc(void) {
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (!parts[i].active) { memset(&parts[i],0,sizeof(Particle)); parts[i].active=true; return &parts[i]; }
    return NULL;
}

void particles_emit_explosion(float x, float y, color_t color, int count) {
    count = (int)(count * PARTICLE_SPAWN_MULTIPLIER);
    for (int i = 0; i < count; i++) {
        Particle* p = part_alloc(); if (!p) break;
        float a = (float)i / count * 6.2832f;
        float spd = 40.0f + rand()%60;
        p->x=x; p->y=y;
        p->vx=cosf(a)*spd; p->vy=sinf(a)*spd;
        p->life=p->max_life=0.4f+(rand()%40)/100.0f;
        p->size=2.0f+rand()%3; p->gravity=200.0f; p->color=color;
    }
}

void particles_emit_coins(float x, float y, int gold) {
    int n = gold/5; if (n<1) n=1; if (n>8) n=8;
    for (int i=0;i<n;i++) {
        Particle* p = part_alloc(); if (!p) break;
        p->x=x; p->y=y;
        p->vx=(rand()%80-40); p->vy=-80-rand()%40;
        p->life=p->max_life=1.0f; p->size=4.0f;
        p->gravity=200.0f; p->color=RGBA32(255,215,0,255);
    }
}

void particles_emit_hit(float x, float y, color_t color) {
    for (int i=0;i<5;i++) {
        Particle* p = part_alloc(); if (!p) break;
        p->x=x; p->y=y;
        p->vx=(rand()%100-50); p->vy=(rand()%100-50);
        p->life=p->max_life=0.2f; p->size=2.0f;
        p->gravity=0; p->color=color;
    }
}

void particles_update(float dt) {
    for (int i=0;i<MAX_PARTICLES;i++) {
        Particle* p=&parts[i]; if(!p->active) continue;
        p->vy+=p->gravity*dt;
        p->x+=p->vx*dt; p->y+=p->vy*dt;
        p->life-=dt;
        if (p->life<=0) p->active=false;
    }
}

void particles_render(const Camera* cam) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_ADDITIVE);
    for (int i=0;i<MAX_PARTICLES;i++) {
        Particle* p=&parts[i]; if(!p->active) continue;
        float alpha = p->life/p->max_life;
        color_t c = p->color; c.a=(uint8_t)(255*alpha);
        rdpq_set_prim_color(c);
        float hs=p->size/2;
        float rx = p->x, ry = p->y;
        camera_apply(cam, &rx, &ry);
        rdpq_fill_rectangle(rx-hs,ry-hs,rx+hs,ry+hs);
    }
}

// =============================================================================
// FLOATING TEXT
// =============================================================================

static FloatingText ft_pool[MAX_FLOATING_TEXTS];

void floating_text_init(void) {
    memset(ft_pool, 0, sizeof(ft_pool));
}

void floating_text_spawn(const char* txt, float x, float y, color_t color) {
    for (int i=0;i<MAX_FLOATING_TEXTS;i++) {
        if (!ft_pool[i].active) {
            strncpy(ft_pool[i].text, txt, 15);
            ft_pool[i].text[15]='\0';
            ft_pool[i].x=x; ft_pool[i].y=y;
            ft_pool[i].vy=-35.0f; ft_pool[i].life=1.0f;
            ft_pool[i].color=color; ft_pool[i].active=true;
            return;
        }
    }
}

void floating_text_update(float dt) {
    for (int i=0;i<MAX_FLOATING_TEXTS;i++) {
        FloatingText* t=&ft_pool[i]; if(!t->active) continue;
        t->y+=t->vy*dt; t->vy+=25.0f*dt;
        t->life-=dt;
        if(t->life<=0) t->active=false;
    }
}

void floating_text_render(void) {
    rdpq_set_mode_standard();
    for (int i=0;i<MAX_FLOATING_TEXTS;i++) {
        FloatingText* t=&ft_pool[i]; if(!t->active) continue;
        color_t c=t->color; c.a=(uint8_t)(255*t->life);
        rdpq_set_prim_color(c);
        rdpq_text_printf(NULL, 1, (int)t->x, (int)t->y, "%s", t->text);
    }
}

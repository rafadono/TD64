#include "animation.h"
#include "../core/engine.h"
#include "../config/game_config.h"
void animator_play(Animator* a, const AnimDef* def) {
    if (a->current == def) return;
    a->current = def; a->timer = 0; a->frame_idx = 0; a->finished = false;
}
void animator_update(Animator* a, float dt) {
    if (!a->current || a->finished) return;
    a->timer += dt * ANIMATION_SPEED_MULT;
    if (a->timer >= a->current->frame_duration) {
        a->timer -= a->current->frame_duration;
        a->frame_idx++;
        if (a->frame_idx >= a->current->frame_count) {
            if (a->current->loop) a->frame_idx = 0;
            else { a->frame_idx = a->current->frame_count-1; a->finished = true; }
        }
    }
}

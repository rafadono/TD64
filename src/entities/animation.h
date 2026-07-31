#ifndef ANIMATION_H
#define ANIMATION_H
#include <stdbool.h>
// frame_width: width in pixels of ONE frame within the horizontal sprite
// sheet (start_frame*frame_width, (start_frame+1)*frame_width, ...). 0 = the
// sprite isn't an animated sheet (a single pose) — drawn whole, no sub-rect.
// To animate a unit: generate the PNG as a horizontal strip of
// `frame_count` poses of `frame_width` px each (same height), and set
// frame_width to the real width of each pose. No further code changes needed.
typedef struct { int start_frame; int frame_count; float frame_duration; bool loop; int frame_width; } AnimDef;
typedef struct { const AnimDef* current; float timer; int frame_idx; bool finished; } Animator;
void animator_play(Animator* a, const AnimDef* def);
void animator_update(Animator* a, float dt);
#endif

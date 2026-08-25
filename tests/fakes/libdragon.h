#ifndef FAKE_LIBDRAGON_H
#define FAKE_LIBDRAGON_H

// =============================================================================
// FAKE LIBDRAGON — a minimal stand-in for <libdragon.h>, used ONLY to compile
// the pure-logic source files under tests/ with a host compiler (see
// tests/README.md). It is NOT a real SDK: it defines just enough types and
// no-op function stubs for src/core/engine.h's include chain to compile, and
// for the handful of rdpq_*/joypad_* calls that appear inside the specific
// .c files the test binary links (score.c, effects.c, pathfinding.c,
// controls.c). Never included by the real N64 build — the makefile never
// points at tests/fakes, and the real toolchain's own <libdragon.h> always
// wins there.
// =============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h> // NULL — real libdragon.h transitively pulls this in too

// ---- color_t: used by value in several structs (Entity.tint, particles,
// terrain modifiers) and passed to draw calls. Matches the real struct's
// shape closely enough for our purposes (only fields are ever touched, no
// pointer arithmetic/size assumptions rely on it beyond that). ----
typedef struct { uint8_t r, g, b, a; } color_t;
// A compound literal, not a function call: units_data.c uses RGBA32() (via
// its local C() shorthand) inside a file-scope `const` array initializer,
// which requires a constant expression — a function call wouldn't qualify.
#define RGBA32(r, g, b, a) ((color_t){ (uint8_t)(r), (uint8_t)(g), (uint8_t)(b), (uint8_t)(a) })

// ---- sprite_t: only ever used as an opaque pointer in headers/compiled
// .c files here, never dereferenced — an incomplete type is enough. ----
typedef struct sprite_s sprite_t;

// ---- rdpq: only the constants/functions actually called by score.c
// (score_render_combo) and effects.c (particles_render/floating_text_render)
// need to exist, as no-ops — nothing under test reads their effect. ----
typedef int rdpq_combiner_t;
typedef int rdpq_blender_t;
#define RDPQ_COMBINER_FLAT ((rdpq_combiner_t)1)
#define RDPQ_BLENDER_ADDITIVE ((rdpq_blender_t)1)
static inline void rdpq_set_mode_standard(void) {}
static inline void rdpq_mode_combiner(rdpq_combiner_t c) { (void)c; }
static inline void rdpq_mode_blender(rdpq_blender_t b) { (void)b; }
static inline void rdpq_set_prim_color(color_t c) { (void)c; }
static inline void rdpq_fill_rectangle(float x0, float y0, float x1, float y1) {
    (void)x0; (void)y0; (void)x1; (void)y1;
}
static inline void rdpq_text_printf(void* style, int8_t font_id, int x, int y, const char* fmt, ...) {
    (void)style; (void)font_id; (void)x; (void)y; (void)fmt;
}

// ---- joypad: only the bit fields actually read by controls.c's
// button_matches() need to exist. Real libdragon packs these into a
// bitfield union; a plain struct of bools is layout-equivalent for our
// purposes since nothing here relies on the union's raw integer form. ----
typedef struct {
    bool a, b, z, start, l, r;
    bool d_up, d_down, d_left, d_right;
    bool c_up, c_down, c_left, c_right;
} joypad_buttons_t;

// ---- debugf: not called by anything the test binaries link, but some
// headers may reference it in comments/prototypes pulled transitively;
// defined defensively as a no-op variadic. ----
static inline void debugf(const char* fmt, ...) { (void)fmt; }

#endif // FAKE_LIBDRAGON_H

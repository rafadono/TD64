#ifndef SCREEN_H
#define SCREEN_H

// =============================================================================
// SCREEN — single source of truth for the game's logical resolution.
//
// Deliberately has zero dependencies (not even libdragon.h) so it can be
// included from low-level headers like world/terrain.h without creating a
// circular include with engine.h (which pulls in nearly every other header).
//
// This drives BOTH the actual video mode (see the custom resolution_t built
// from these two macros in core/main.c's display_init() call) and every
// resolution-dependent computation in the game (terrain grid size, UI layout
// anchors, cursor bounds). Changing these two numbers and rebuilding is
// enough to retarget the whole game to a different resolution — nothing
// else needs to be hardcoded to 320x240 elsewhere.
//
// What does NOT automatically adapt: the 4 fixed campaign maps (world/maps.c)
// and their preset path shapes (world/pathfinding.c) are hand-designed
// content tuned for a 320x240 canvas, same as the pixel-art sprites/tiles are
// tuned for their fixed pixel size — a bigger SCREEN_WIDTH/HEIGHT just gives
// the (still 16px) terrain grid more cells and shows more of that grid, it
// doesn't redesign the existing maps' layout or rescale art.
// =============================================================================
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// WORLD_WIDTH/HEIGHT: the size of the biggest scrollable playing field a map
// can use (currently the 4 fixed campaign maps — see world/maps.c). 2x each
// dimension = 4x the area of one screen. The camera (systems/effects.h)
// scrolls within [0, WORLD_WIDTH-SCREEN_WIDTH] x [0, WORLD_HEIGHT-SCREEN_HEIGHT].
// Custom maps (the in-game editor) are NOT affected by this — they stay
// exactly SCREEN_WIDTH x SCREEN_HEIGHT (MapData.width/height, set in
// map_load_custom()), so a map with width==height==SCREEN_* simply has a
// camera whose scroll range clamps to (0,0) — no special-casing needed
// anywhere that already goes through the camera.
#define WORLD_WIDTH  (2 * SCREEN_WIDTH)
#define WORLD_HEIGHT (2 * SCREEN_HEIGHT)

#endif // SCREEN_H

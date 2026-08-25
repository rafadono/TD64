#ifndef SAVE_H
#define SAVE_H
#include <stdbool.h>
#include <stdint.h>
#include "../world/terrain.h"
#include "controls.h"

// =============================================================================
// SAVE — persistence on the N64 Controller Pak (memory pak)
//
// Uses exclusively libdragon's high-level "notes" API (mempak.h), never raw
// sector access — that way our notes coexist with any other game's notes on
// the same Controller Pak.
// =============================================================================

#define SAVE_MAP_SLOTS   8
// Derived from screen.h + TERRAIN_GRID_SIZE (terrain.h) instead of separately
// hardcoded numbers that had to be kept in sync by convention — changing the
// game's resolution automatically resizes what gets saved, no risk of drift.
#define SAVE_GRID_W      (SCREEN_WIDTH  / TERRAIN_GRID_SIZE)
#define SAVE_GRID_H      (SCREEN_HEIGHT / TERRAIN_GRID_SIZE)

// Serialized format of a custom map (what gets saved to the Controller Pak).
// grid[] uses the same values as TerrainType (terrain.h).
#define SAVE_NAME_LEN 12  // 11 usable chars + null terminator

typedef struct {
    uint8_t  grid[SAVE_GRID_W * SAVE_GRID_H];
    uint8_t  path_type;      // 0=curve 1=zigzag 2=spiral 3=straight 4=custom/painted (see pathfinding.c)
    uint8_t  enemy_faction;  // FactionId (factions.h) of the attacking side
    uint8_t  difficulty;     // 1-5, informational label shown in the menu/editor
                             // (fixed campaign maps also store a difficulty label
                             // that isn't read anywhere for scaling yet — same here)
    uint16_t starting_gold;
    uint8_t  starting_lives;
    char     name[SAVE_NAME_LEN]; // player-entered name, always null-terminated; empty = unnamed
    uint8_t  used;           // 0 = empty/invalid slot
} CustomMapSave;

// Game progress (best score per campaign).
typedef struct {
    uint32_t best_score[4];
    uint8_t  campaigns_completed; // bitmask, bit N = campaign N completed at least once
} GameProgress;

// Remapped button bindings (see systems/controls.h). `bindings[action]` is
// a PhysicalButton value; `valid` is 0 for a note that was never actually
// saved (e.g. a freshly formatted pak), so controls_init() knows to keep
// the compiled-in defaults instead of trusting an all-zero note.
typedef struct {
    uint8_t bindings[ACTION_COUNT];
    uint8_t valid;
} InputConfig;

typedef enum {
    SAVE_STATUS_UNKNOWN = 0,
    SAVE_STATUS_NO_PAK,        // no Controller Pak found on any of the 4 ports
    SAVE_STATUS_UNFORMATTED,   // a pak is present, but unformatted or corrupt
    SAVE_STATUS_READY,         // ready to read/write
} SaveStatus;

// Scans all 4 controller ports and validates whichever Controller Pak it
// finds (any port works, not just port 1 - see save.c). Does real I/O
// (Joybus) — call only when entering a screen that needs it, not every frame.
SaveStatus save_system_check(void);

// Last result cached by save_system_check(), without doing I/O again.
SaveStatus save_system_status(void);

// Formats the Controller Pak. THIS ERASES EVERYTHING ON THE PAK, not just
// TD64's data — the UI must require an explicit, unusual confirmation (not a
// single accidental button) before calling this.
bool save_format_pak_confirmed(void);

// Custom maps (slots 0..SAVE_MAP_SLOTS-1)
bool save_list_custom_maps(bool used_out[SAVE_MAP_SLOTS]);
bool save_read_custom_map(int slot, CustomMapSave* out);
bool save_write_custom_map(int slot, const CustomMapSave* data);
bool save_delete_custom_map(int slot);

// Game progress
bool save_read_progress(GameProgress* out);
bool save_write_progress(const GameProgress* progress);

// Remapped button bindings
bool save_read_input_config(InputConfig* out);
bool save_write_input_config(const InputConfig* cfg);

#endif // SAVE_H

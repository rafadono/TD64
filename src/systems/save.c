#include "save.h"
#include <libdragon.h>
#include <string.h>

// Which port currently holds the Controller Pak we're using, set by
// save_system_check() (see below) - not hardcoded to port 1, since the
// player might have it plugged into any of the 4 ports. -1 = not detected
// yet / no pak found.
static int save_port = -1;

// Arbitrary vendor/game IDs (TD64 is homebrew, it has no officially
// registered ID) — only used to fill in the entry fields, they don't affect
// compatibility with other notes on the pak.
#define SAVE_VENDOR   0x54443634u
#define SAVE_GAMEID   ((uint16_t)0x5464u)

// Note names. The charset mempak preserves without converting to spaces is
// uppercase letters + symbols (digits are not guaranteed to survive), so we
// avoid numbers in the names.
static const char* MAP_SLOT_NAMES[SAVE_MAP_SLOTS] = {
    "TD MAP A", "TD MAP B", "TD MAP C", "TD MAP D",
    "TD MAP E", "TD MAP F", "TD MAP G", "TD MAP H",
};
#define PROGRESS_NAME "TD SAVE"

// Scratch buffer for reading/writing notes. CustomMapSave (~305B) needs 2
// blocks (512B); left some headroom so a small format growth later doesn't
// require revisiting this.
#define NOTE_BUF_SIZE (MEMPAK_BLOCK_SIZE * 4)
static uint8_t note_buf[NOTE_BUF_SIZE];

static SaveStatus cached_status = SAVE_STATUS_UNKNOWN;

SaveStatus save_system_check(void) {
    // Scan every port: the first one with a formatted, valid pak wins
    // outright. If none is ready, fall back to the first unformatted pak we
    // saw (so the "hold L+R+A to format" flow has a port to act on). Only
    // if no port has a Controller Pak at all do we report SAVE_STATUS_NO_PAK.
    int first_unformatted = -1;
    for (int p = 0; p < JOYPAD_PORT_COUNT; p++) {
        if (joypad_get_accessory_type((joypad_port_t)p) != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) continue;
        if (validate_mempak(p) == 0) {
            save_port = p;
            cached_status = SAVE_STATUS_READY;
            return cached_status;
        }
        if (first_unformatted < 0) first_unformatted = p;
    }

    if (first_unformatted >= 0) {
        save_port = first_unformatted;
        cached_status = SAVE_STATUS_UNFORMATTED;
        return cached_status;
    }

    save_port = -1;
    cached_status = SAVE_STATUS_NO_PAK;
    return cached_status;
}

SaveStatus save_system_status(void) {
    return cached_status;
}

bool save_format_pak_confirmed(void) {
    if (save_port < 0) return false;
    if (format_mempak(save_port) != 0) return false;
    cached_status = SAVE_STATUS_READY;
    return true;
}

static int find_entry(const char* name, entry_structure_t* out) {
    // Only the real name length is compared (not sizeof(e.name)): the fixed
    // 19-byte field may come back space-padded or zero-padded depending on
    // the mempak driver, and we don't want a padding mismatch against what
    // we write (zero-padded via memset) to break the comparison.
    size_t len = strlen(name);
    for (int i = 0; i < 16; i++) {
        entry_structure_t e;
        if (get_mempak_entry(save_port, i, &e) != 0) continue;
        if (!e.valid) continue;
        if (strncmp(e.name, name, len) == 0) {
            if (out) *out = e;
            return i;
        }
    }
    return -1;
}

static bool write_note(const char* name, const void* data, int size) {
    if (cached_status != SAVE_STATUS_READY) return false;
    if (size <= 0 || size > NOTE_BUF_SIZE) return false;

    int blocks = (size + MEMPAK_BLOCK_SIZE - 1) / MEMPAK_BLOCK_SIZE;

    // write_mempak_entry_data does not overwrite: if a note with this name
    // already exists (previous save), it must be deleted first.
    entry_structure_t existing;
    if (find_entry(name, &existing) >= 0) {
        if (delete_mempak_entry(save_port, &existing) != 0) return false;
    }

    if (get_mempak_free_space(save_port) < blocks) return false;

    memset(note_buf, 0, sizeof(note_buf));
    memcpy(note_buf, data, size);

    entry_structure_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.vendor  = SAVE_VENDOR;
    entry.game_id = SAVE_GAMEID;
    entry.region  = 0;
    entry.blocks  = (uint8_t)blocks;
    strncpy(entry.name, name, sizeof(entry.name) - 1);

    return write_mempak_entry_data(save_port, &entry, note_buf) == 0;
}

static bool read_note(const char* name, void* out, int size) {
    if (cached_status != SAVE_STATUS_READY) return false;

    entry_structure_t entry;
    if (find_entry(name, &entry) < 0) return false;
    if ((int)entry.blocks * MEMPAK_BLOCK_SIZE > NOTE_BUF_SIZE) return false;

    if (read_mempak_entry_data(save_port, &entry, note_buf) != 0) return false;
    memcpy(out, note_buf, size);
    return true;
}

bool save_list_custom_maps(bool used_out[SAVE_MAP_SLOTS]) {
    for (int i = 0; i < SAVE_MAP_SLOTS; i++) used_out[i] = false;
    if (cached_status != SAVE_STATUS_READY) return false;

    for (int i = 0; i < SAVE_MAP_SLOTS; i++) {
        entry_structure_t e;
        used_out[i] = (find_entry(MAP_SLOT_NAMES[i], &e) >= 0);
    }
    return true;
}

bool save_read_custom_map(int slot, CustomMapSave* out) {
    if (slot < 0 || slot >= SAVE_MAP_SLOTS || !out) return false;
    return read_note(MAP_SLOT_NAMES[slot], out, sizeof(CustomMapSave));
}

bool save_write_custom_map(int slot, const CustomMapSave* data) {
    if (slot < 0 || slot >= SAVE_MAP_SLOTS || !data) return false;
    return write_note(MAP_SLOT_NAMES[slot], data, sizeof(CustomMapSave));
}

bool save_delete_custom_map(int slot) {
    if (slot < 0 || slot >= SAVE_MAP_SLOTS) return false;
    if (cached_status != SAVE_STATUS_READY) return false;

    entry_structure_t e;
    int idx = find_entry(MAP_SLOT_NAMES[slot], &e);
    if (idx < 0) return true; // already empty
    return delete_mempak_entry(save_port, &e) == 0;
}

bool save_read_progress(GameProgress* out) {
    if (!out) return false;
    return read_note(PROGRESS_NAME, out, sizeof(GameProgress));
}

bool save_write_progress(const GameProgress* progress) {
    if (!progress) return false;
    return write_note(PROGRESS_NAME, progress, sizeof(GameProgress));
}

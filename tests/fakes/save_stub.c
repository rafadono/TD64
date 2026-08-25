// A test-controlled stand-in for src/systems/save.c's Controller Pak layer,
// linked into the controls test binary ONLY (see tests/README.md). Real
// save.c does actual Joybus I/O, which isn't something worth faking realistically
// here — controls.c only needs *some* implementation of these 3 functions to
// link, and tests want to dictate their return values/output directly rather
// than fight real hardware I/O.
#include "../../src/systems/save.h"
#include <string.h>

SaveStatus stub_save_status = SAVE_STATUS_NO_PAK;
InputConfig stub_input_config;
bool        stub_input_config_present = false;
InputConfig stub_last_written_config;
bool        stub_write_was_called = false;

SaveStatus save_system_check(void) { return stub_save_status; }
SaveStatus save_system_status(void) { return stub_save_status; }

bool save_read_input_config(InputConfig* out) {
    if (!stub_input_config_present) return false;
    *out = stub_input_config;
    return true;
}

bool save_write_input_config(const InputConfig* cfg) {
    stub_write_was_called = true;
    stub_last_written_config = *cfg;
    return true;
}

// Not called by controls.c, but declared in save.h — provide harmless
// stand-ins so any future controls.c change that touches these still links.
bool save_format_pak_confirmed(void) { return false; }
bool save_list_custom_maps(bool used_out[SAVE_MAP_SLOTS]) { memset(used_out, 0, sizeof(bool) * SAVE_MAP_SLOTS); return false; }
bool save_read_custom_map(int slot, CustomMapSave* out) { (void)slot; (void)out; return false; }
bool save_write_custom_map(int slot, const CustomMapSave* data) { (void)slot; (void)data; return false; }
bool save_delete_custom_map(int slot) { (void)slot; return false; }
bool save_read_progress(GameProgress* out) { (void)out; return false; }
bool save_write_progress(const GameProgress* progress) { (void)progress; return false; }

#ifndef LANG_H
#define LANG_H

// =============================================================================
// LANG — simple string-table localization
//
// All player-facing UI text goes through T(STR_ID) instead of being a literal
// string at the call site. Adding a language later means adding one more row
// to STRINGS[] in lang.c, not touching any UI code. The current language can
// be changed at runtime (see lang_set/lang_cycle, wired to a Main Menu option
// in menu.c) or the default can be changed at build time via LANG_DEFAULT
// below.
// =============================================================================

typedef enum {
    LANG_EN = 0,
    LANG_ES = 1,
    LANG_COUNT
} Language;

#ifndef LANG_DEFAULT
#define LANG_DEFAULT LANG_EN
#endif

typedef enum {
    STR_TITLE_MAIN,
    STR_MENU_PLAY_CAMPAIGN,
    STR_MENU_CUSTOM_MAPS,
    STR_MENU_CONTROLS,
    STR_MENU_STATS,
    STR_MENU_CREDITS,
    STR_MENU_LANGUAGE,
    STR_HINT_MAIN_MENU,

    STR_FACTION_SELECT_TITLE,
    STR_FACTION_SELECT_SUBTITLE,
    STR_HINT_NAVIGATE,
    STR_HINT_CONFIRM,
    STR_HINT_BACK,
    STR_LABEL_RIVAL,

    STR_PAUSE_TITLE,
    STR_PAUSE_RESUME,
    STR_PAUSE_RESTART,
    STR_PAUSE_MAIN_MENU,

    STR_END_VICTORY,
    STR_END_DEFEAT,
    STR_STAT_KILLS,
    STR_STAT_GOLD,
    STR_STAT_WAVE,
    STR_STAT_PERFECT,
    STR_END_RETRY,
    STR_END_CHANGE_FACTION,

    STR_TERRAIN_GRASS,
    STR_TERRAIN_WATER,
    STR_TERRAIN_MOUNTAIN,
    STR_TERRAIN_DESERT,
    STR_TERRAIN_SNOW,
    STR_TERRAIN_LAVA,
    STR_TERRAIN_PATH,

    STR_PATH_CURVE,
    STR_PATH_ZIGZAG,
    STR_PATH_SPIRAL,
    STR_PATH_STRAIGHT,
    STR_PATH_CUSTOM,

    STR_EDITOR_TERRAIN_LABEL,
    STR_EDITOR_PATH_LABEL,
    STR_EDITOR_ENEMY_LABEL,
    STR_EDITOR_DIFFICULTY_LABEL,
    STR_EDITOR_SLOT_LABEL,
    STR_EDITOR_NEW,
    STR_EDITOR_GOLD_LABEL,
    STR_EDITOR_CONTROLS_HINT,
    STR_EDITOR_NAME_LABEL,
    STR_EDITOR_NAMING_HINT,

    STR_DIFF_SELECT_TITLE,
    STR_DIFF_EASY,
    STR_DIFF_NORMAL,
    STR_DIFF_HARD,
    STR_DIFF_EXTREME,
    STR_DIFF_LOCKED_BADGE,
    STR_DIFF_LOCKED_HINT,

    STR_PAK_READY,
    STR_PAK_UNFORMATTED,
    STR_PAK_MISSING,

    STR_CMM_TITLE,
    STR_CMM_PAK_READY_MSG,
    STR_CMM_PAK_UNFORMATTED_MSG,
    STR_CMM_PAK_MISSING_MSG,
    STR_CMM_SLOT_SAVED,
    STR_CMM_SLOT_EMPTY,
    STR_CMM_CONFIRM_DELETE,
    STR_CMM_CONTROLS_HINT,

    STR_FACTION_DESC_DAWNGUARD,
    STR_FACTION_DESC_IRONBONE,
    STR_FACTION_DESC_ASHCLAW,
    STR_FACTION_DESC_VEILSTORM,

    STR_CONTROLS_TITLE,
    STR_CONTROLS_HINT,
    STR_CONTROLS_WAITING,
    STR_CONTROLS_SAVED,
    STR_ACTION_PLACE,
    STR_ACTION_CANCEL,
    STR_ACTION_PREV_UNIT,
    STR_ACTION_NEXT_UNIT,
    STR_ACTION_UPGRADE,
    STR_ACTION_SPAWN_WAVE,
    STR_ACTION_PAUSE,

    STR_STATS_TITLE,
    STR_STATS_PAGE_PROGRESS,
    STR_STATS_PAGE_HIGHLIGHTS,
    STR_STATS_TOTAL_KILLS,
    STR_STATS_BEST_SCORE,
    STR_STATS_FASTEST_CLEAR,
    STR_STATS_NOT_CLEARED,
    STR_STATS_NO_RUN,
    STR_STATS_HINT,
    STR_EVENT_MAP_STARTED,
    STR_EVENT_WAVE_CLEARED,
    STR_EVENT_HERO_KILLED,
    STR_EVENT_VICTORY,
    STR_EVENT_DEFEAT,

    STR_COMBAT_RESIST,

    STR_COUNT
} StringId;

void        lang_set(Language l);
void        lang_cycle(void);          // cycles EN -> ES -> EN... for the menu toggle
Language    lang_get(void);
const char* lang_name(Language l);     // "ENGLISH" / "ESPANOL", for the toggle display
const char* T(StringId id);            // text for id in the current language

#endif // LANG_H

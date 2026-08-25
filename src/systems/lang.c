#include "lang.h"

// All Spanish strings intentionally skip accents/enie: libdragon's builtin
// font has no confirmed extended Latin-1/UTF-8 charset, and English (the
// default language) sidesteps the issue entirely. This is a common retro
// game convention for the same reason.

static const char* STRINGS[STR_COUNT][LANG_COUNT] = {
    [STR_TITLE_MAIN]              = { "FACTION WARS TOWER DEFENSE", "FACTION WARS TOWER DEFENSE" },
    [STR_MENU_PLAY_CAMPAIGN]      = { "PLAY CAMPAIGN",              "JUGAR CAMPANA" },
    [STR_MENU_CUSTOM_MAPS]        = { "CUSTOM MAPS",                "MAPAS CUSTOM" },
    [STR_MENU_CONTROLS]           = { "CONTROLS",                   "CONTROLES" },
    [STR_MENU_CREDITS]            = { "CREDITS",                    "CREDITOS" },
    [STR_MENU_LANGUAGE]           = { "LANGUAGE",                   "IDIOMA" },
    [STR_HINT_MAIN_MENU]          = { "D-PAD: MOVE  A: CONFIRM",    "D-PAD: MOVER  A: CONFIRMAR" },

    [STR_FACTION_SELECT_TITLE]    = { "SELECT YOUR FACTION",        "SELECCIONA TU FACCION" },
    [STR_FACTION_SELECT_SUBTITLE] = { "CAMPAIGNS",                  "CAMPANAS" },
    [STR_HINT_NAVIGATE]           = { "NAVIGATE",                   "NAVEGAR" },
    [STR_HINT_CONFIRM]            = { "CONFIRM",                    "CONFIRMAR" },
    [STR_HINT_BACK]               = { "BACK",                       "VOLVER" },
    [STR_LABEL_RIVAL]             = { "RIVAL",                      "RIVAL" },

    [STR_PAUSE_TITLE]             = { "GAME PAUSED",                "JUEGO PAUSADO" },
    [STR_PAUSE_RESUME]            = { "RESUME",                     "REANUDAR" },
    [STR_PAUSE_RESTART]           = { "RESTART",                    "REINICIAR" },
    [STR_PAUSE_MAIN_MENU]         = { "MAIN MENU",                  "MENU PRINCIPAL" },

    [STR_END_VICTORY]             = { "CAMPAIGN COMPLETE!",         "CAMPANA COMPLETADA!" },
    [STR_END_DEFEAT]               = { "GAME OVER!",                 "JUEGO TERMINADO!" },
    [STR_STAT_KILLS]              = { "KILLS: %d",                  "MUERTES: %d" },
    [STR_STAT_GOLD]               = { "GOLD: %d",                   "ORO: %d" },
    [STR_STAT_WAVE]               = { "WAVE: %d",                   "OLEADA: %d" },
    [STR_STAT_PERFECT]            = { "PERFECT: %d",                "PERFECTA: %d" },
    [STR_END_RETRY]               = { "RETRY",                      "REINTENTAR" },
    [STR_END_CHANGE_FACTION]      = { "CHANGE FACTION",             "CAMBIAR FACCION" },

    [STR_TERRAIN_GRASS]           = { "GRASS",                      "PASTO" },
    [STR_TERRAIN_WATER]           = { "WATER",                      "AGUA" },
    [STR_TERRAIN_MOUNTAIN]        = { "MOUNTAIN",                   "MONTANA" },
    [STR_TERRAIN_DESERT]          = { "DESERT",                     "DESIERTO" },
    [STR_TERRAIN_SNOW]            = { "SNOW",                       "NIEVE" },
    [STR_TERRAIN_LAVA]            = { "LAVA",                       "LAVA" },
    [STR_TERRAIN_PATH]            = { "PATH",                       "CAMINO" },

    [STR_PATH_CURVE]              = { "CURVE",                      "CURVA" },
    [STR_PATH_ZIGZAG]             = { "ZIGZAG",                     "ZIGZAG" },
    [STR_PATH_SPIRAL]             = { "SPIRAL",                     "ESPIRAL" },
    [STR_PATH_STRAIGHT]           = { "STRAIGHT",                   "RECTA" },
    [STR_PATH_CUSTOM]             = { "CUSTOM (PAINT PATH TILES)",  "CUSTOM (PINTAR CAMINO)" },

    [STR_EDITOR_TERRAIN_LABEL]    = { "TERRAIN:%s",                 "TERRENO:%s" },
    [STR_EDITOR_PATH_LABEL]       = { "PATH:%s",                    "PATH:%s" },
    [STR_EDITOR_ENEMY_LABEL]      = { "ENEMY:%s",                   "ENEMIGO:%s" },
    [STR_EDITOR_DIFFICULTY_LABEL] = { "DIFFICULTY:%d",              "DIFICULTAD:%d" },
    [STR_EDITOR_SLOT_LABEL]       = { "SLOT %c",                    "SLOT %c" },
    [STR_EDITOR_NEW]              = { "NEW",                        "NUEVO" },
    [STR_EDITOR_GOLD_LABEL]       = { "STARTING GOLD:%d (Z)",       "ORO INICIAL:%d (Z)" },
    [STR_EDITOR_CONTROLS_HINT]    = {
        "D-PAD MOVE  A PAINT  L/R TERRAIN  Z TAB  C-U/D ADJUST  C-LEFT NAME  START SAVE  B EXIT",
        "D-PAD MOVER  A PINTAR  L/R TERRENO  Z PESTANA  C-A/B AJUSTAR  C-IZQ NOMBRE  START GUARDAR  B SALIR"
    },
    [STR_EDITOR_NAME_LABEL]       = { "NAME:%s",                    "NOMBRE:%s" },
    [STR_EDITOR_NAMING_HINT]      = {
        "D-PAD SELECT  A TYPE  Z BACKSPACE  B DONE",
        "D-PAD SELECCIONAR  A ESCRIBIR  Z BORRAR  B LISTO"
    },

    [STR_DIFF_SELECT_TITLE]       = { "SELECT DIFFICULTY",          "SELECCIONA DIFICULTAD" },
    [STR_DIFF_EASY]               = { "EASY",                       "FACIL" },
    [STR_DIFF_NORMAL]             = { "NORMAL",                     "NORMAL" },
    [STR_DIFF_HARD]               = { "HARD",                       "DIFICIL" },
    [STR_DIFF_EXTREME]            = { "EXTREME",                    "EXTREMA" },
    [STR_DIFF_LOCKED_BADGE]       = { "LOCK",                       "BLOQ" },
    [STR_DIFF_LOCKED_HINT]        = {
        "LOCKED - COMPLETE MORE CAMPAIGNS TO UNLOCK",
        "BLOQUEADO - COMPLETA MAS CAMPANAS PARA DESBLOQUEAR"
    },

    [STR_PAK_READY]               = { "PAK READY",                  "PAK LISTO" },
    [STR_PAK_UNFORMATTED]         = { "PAK NOT FORMATTED",          "PAK SIN FORMATEAR" },
    [STR_PAK_MISSING]             = { "NO CONTROLLER PAK",          "SIN CONTROLLER PAK" },

    [STR_CMM_TITLE]               = { "CUSTOM MAPS",                "MAPAS CUSTOM" },
    [STR_CMM_PAK_READY_MSG]       = { "CONTROLLER PAK READY",       "CONTROLLER PAK LISTO" },
    [STR_CMM_PAK_UNFORMATTED_MSG] = {
        "PAK NOT FORMATTED - HOLD L+R AND PRESS A TO FORMAT (ERASES EVERYTHING)",
        "PAK SIN FORMATEAR - SOSTENER L+R Y A PARA FORMATEAR (BORRA TODO)"
    },
    [STR_CMM_PAK_MISSING_MSG]     = {
        "NO CONTROLLER PAK - YOU CAN STILL PLAY WITHOUT SAVING (FROM THE EDITOR)",
        "SIN CONTROLLER PAK - SE PUEDE JUGAR SIN GUARDAR (DESDE EL EDITOR)"
    },
    [STR_CMM_SLOT_SAVED]          = { "MAP SAVED",                  "MAPA GUARDADO" },
    [STR_CMM_SLOT_EMPTY]          = { "[EMPTY]",                    "[VACIO]" },
    [STR_CMM_CONFIRM_DELETE]      = { "DELETE SLOT %c? A=YES B=NO", "BORRAR SLOT %c? A=SI B=NO" },
    [STR_CMM_CONTROLS_HINT]       = {
        "A PLAY  START EDIT  C-LEFT DELETE  B BACK",
        "A JUGAR  START EDITAR  C-IZQ BORRAR  B VOLVER"
    },

    [STR_FACTION_DESC_DAWNGUARD]  = { "Holy knights. Balanced and disciplined.",
                                       "Caballeros sagrados. Balanceados y disciplinados." },
    [STR_FACTION_DESC_IRONBONE]   = { "Undead. High health, energy drain.",
                                       "No-muertos. Alta vida, drenaje de energia." },
    [STR_FACTION_DESC_ASHCLAW]    = { "Savages. Maximum damage, minimal defense.",
                                       "Salvajes. Maximo dano, minima defensa." },
    [STR_FACTION_DESC_VEILSTORM]  = { "Arcane mages. Maximum range, very fragile.",
                                       "Magos arcanos. Alcance maximo, muy fragiles." },

    [STR_CONTROLS_TITLE]         = { "CONTROLS",                    "CONTROLES" },
    [STR_CONTROLS_HINT]          = {
        "D-PAD SELECT  A REBIND  Z RESET  START SAVE  B BACK",
        "D-PAD SELECCIONAR  A REASIGNAR  Z RESETEAR  START GUARDAR  B VOLVER"
    },
    [STR_CONTROLS_WAITING]       = { "PRESS A BUTTON... (B CANCELS)", "PRESIONA UN BOTON... (B CANCELA)" },
    [STR_CONTROLS_SAVED]         = { "SAVED TO CONTROLLER PAK",      "GUARDADO EN CONTROLLER PAK" },
    [STR_ACTION_PLACE]           = { "PLACE UNIT",                  "COLOCAR UNIDAD" },
    [STR_ACTION_CANCEL]          = { "CANCEL",                      "CANCELAR" },
    [STR_ACTION_PREV_UNIT]       = { "PREV UNIT",                   "UNIDAD ANTERIOR" },
    [STR_ACTION_NEXT_UNIT]       = { "NEXT UNIT",                   "UNIDAD SIGUIENTE" },
    [STR_ACTION_UPGRADE]         = { "UPGRADE TOWER",                "MEJORAR TORRE" },
    [STR_ACTION_SPAWN_WAVE]      = { "SPAWN WAVE",                  "LANZAR OLEADA" },
    [STR_ACTION_PAUSE]           = { "PAUSE",                       "PAUSA" },
};

static Language current_lang = LANG_DEFAULT;

void lang_set(Language l) {
    if (l >= 0 && l < LANG_COUNT) current_lang = l;
}

void lang_cycle(void) {
    current_lang = (Language)((current_lang + 1) % LANG_COUNT);
}

Language lang_get(void) {
    return current_lang;
}

const char* lang_name(Language l) {
    static const char* NAMES[LANG_COUNT] = { "ENGLISH", "ESPANOL" };
    if (l < 0 || l >= LANG_COUNT) return "?";
    return NAMES[l];
}

const char* T(StringId id) {
    if (id < 0 || id >= STR_COUNT) return "";
    const char* s = STRINGS[id][current_lang];
    return s ? s : "";
}

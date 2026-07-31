#include "audio.h"
#include <stdio.h>
#include <string.h>

static wav64_t sfx_hit;
static wav64_t sfx_coin;
static wav64_t sfx_level_up;
static wav64_t bgm_music;

static bool sfx_hit_loaded = false;
static bool sfx_coin_loaded = false;
static bool sfx_lvl_loaded = false;
static bool bgm_loaded = false;
static char current_bgm_path[64] = "";

static int file_exists(const char *path) {
    int fd = dfs_open(path);
    if (fd < 0) return 0;
    dfs_close(fd);
    return 1;
}

void audio_system_init(void) {
    audio_init(44100, 4);
    mixer_init(16);

    // Pre-load sound effects if they exist in filesystem
    if (file_exists("rom:/hit.wav64")) {
        wav64_open(&sfx_hit, "rom:/hit.wav64");
        sfx_hit_loaded = true;
    }
    if (file_exists("rom:/coin.wav64")) {
        wav64_open(&sfx_coin, "rom:/coin.wav64");
        sfx_coin_loaded = true;
    }
    if (file_exists("rom:/levelup.wav64")) {
        wav64_open(&sfx_level_up, "rom:/levelup.wav64");
        sfx_lvl_loaded = true;
    }
}

void audio_system_close(void) {
    if (sfx_hit_loaded) wav64_close(&sfx_hit);
    if (sfx_coin_loaded) wav64_close(&sfx_coin);
    if (sfx_lvl_loaded) wav64_close(&sfx_level_up);
    if (bgm_loaded) {
        wav64_close(&bgm_music);
        bgm_loaded = false;
    }
}

void audio_play_sfx(const char *name) {
    if (strcmp(name, "hit") == 0 && sfx_hit_loaded) {
        mixer_ch_play(1, &sfx_hit.wave); // channel 1 for generic sfx
    } else if (strcmp(name, "coin") == 0 && sfx_coin_loaded) {
        mixer_ch_play(2, &sfx_coin.wave); // channel 2 for coins
    } else if (strcmp(name, "levelup") == 0 && sfx_lvl_loaded) {
        mixer_ch_play(3, &sfx_level_up.wave); // channel 3 for levelups
    }
}

void audio_play_bgm(const char *name) {
    char path[64];
    snprintf(path, sizeof(path), "rom:/%s.wav64", name);

    if (strcmp(current_bgm_path, path) == 0 && bgm_loaded) {
        return; // Already playing this BGM
    }

    if (bgm_loaded) {
        mixer_ch_stop(0); // Stop channel 0 for music
        wav64_close(&bgm_music);
        bgm_loaded = false;
        strcpy(current_bgm_path, "");
    }

    if (file_exists(path)) {
        wav64_open(&bgm_music, path);
        wav64_set_loop(&bgm_music, true);
        mixer_ch_play(0, &bgm_music.wave);
        bgm_loaded = true;
        strcpy(current_bgm_path, path);
    }
}

void audio_update_mixer(void) {
    if (audio_can_write()) {
        short *buf = audio_write_begin();
        if (buf) {
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }
    }
}

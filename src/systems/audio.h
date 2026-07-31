#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <libdragon.h>

void audio_system_init(void);
void audio_system_close(void);
void audio_play_sfx(const char *name);
void audio_play_bgm(const char *name);
void audio_update_mixer(void);

#endif // AUDIO_SYSTEM_H

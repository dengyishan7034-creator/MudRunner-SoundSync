/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator */
#ifndef MR_GAME_AUDIO_H
#define MR_GAME_AUDIO_H
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
typedef struct { void *samples; uint32_t count,rate,fade,loaded; } Sample;
void GameAudioSetRoot(const wchar_t*root);
int GameAudioLoadWave(const char*path,Sample*out);
int GameAudioInflate(const unsigned char*src,size_t srcSize,unsigned char*out,size_t outSize);
unsigned char*GameAudioReadResource(const char*name,size_t*size);
void GameAudioFree(void*p);
#endif

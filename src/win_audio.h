/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator
 * Minimal ABI declarations for the documented Windows waveOut API. */
#ifndef MR_WIN_AUDIO_H
#define MR_WIN_AUDIO_H
#include <windows.h>
typedef HANDLE HWAVEOUT;
typedef UINT MMRESULT;
#pragma pack(push,1)
typedef struct { WORD wFormatTag,nChannels;DWORD nSamplesPerSec,nAvgBytesPerSec;WORD nBlockAlign,wBitsPerSample,cbSize; } WAVEFORMATEX;
#pragma pack(pop)
typedef struct wavehdr { char*lpData;DWORD dwBufferLength,dwBytesRecorded;DWORD_PTR dwUser;DWORD dwFlags,dwLoops;struct wavehdr*lpNext;DWORD_PTR reserved; } WAVEHDR;
#define WHDR_DONE 1
#define WHDR_PREPARED 2
#define WHDR_INQUEUE 16
#define WAVE_FORMAT_PCM 1
#define WAVE_MAPPER ((UINT)-1)
#define CALLBACK_NULL 0
#define MMSYSERR_NOERROR 0
typedef MMRESULT(WINAPI*WaveOpenFn)(HWAVEOUT*,UINT,const WAVEFORMATEX*,DWORD_PTR,DWORD_PTR,DWORD);
typedef MMRESULT(WINAPI*WaveHeaderFn)(HWAVEOUT,WAVEHDR*,UINT);
typedef MMRESULT(WINAPI*WaveSimpleFn)(HWAVEOUT);
#endif

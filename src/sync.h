/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator */
#ifndef MR_SYNC_H
#define MR_SYNC_H
#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include "game_audio.h"
#define MR_VERSION "0.4.2"
#define MR_CHANNEL 7
#define MR_PEERS 8
#define MR_MAGIC 0x3341524dU
#pragma pack(push,1)
typedef struct { uint32_t magic; uint16_t version; uint8_t engine,valid; uint32_t session,seq,entity; float pitch,x,y,z; char model[64]; } Packet;
#pragma pack(pop)
typedef struct { float position[3],right[3],up[3],forward[3]; uint32_t tick; int valid; } Listener;
typedef unsigned char (__cdecl *NetSend)(void*,uint64_t,const void*,uint32_t,int,int);
typedef unsigned char (__cdecl *NetRead)(void*,void*,uint32_t,uint32_t*,uint64_t*,int);
typedef unsigned char (__cdecl *NetAvailable)(void*,uint32_t*,int);
void Log(const char*format,...);
int ReadMemory(uintptr_t address,void*out,size_t size);
int GameGuard(uintptr_t base);
int GameSnapshot(uintptr_t base,Packet*out);
int CameraMath(const float*view,Listener*out);
void CameraCapture(const float*view);
void CameraPublish(const Listener*listener);
int CameraGet(Listener*out);
void CameraInit(void);
void CameraReferenceUpdate(const Packet*p,uint32_t now);
int CameraReference(float*position);
int CameraNativeFresh(void);
int ResolveModelSounds(const char*model,char*idle,char*high,size_t capacity);
void MixerInit(void);
void MixerUpdate(uint64_t peer,const Packet*p,uint32_t now);
void MixerRender(int16_t*out,uint32_t frames,uint32_t now,const Listener*listener);
int MixerOpen(void);
void MixerPump(uint32_t now,const Listener*listener);
void MixerClose(void);
void SyncInit(void);
void SyncLearn(uint64_t id,uint32_t now);
int SyncReceive(uint64_t id,const Packet*p,uint32_t size,uint32_t now);
int PacketValid(const Packet*p,uint32_t bytes);
void SyncStep(uintptr_t game,uint32_t now);
void NetInit(void);
void NetDiscover(void);
void NetAttach(void*iface);
void NetGet(void**iface,NetSend*send,NetRead*read,NetAvailable*available);
int GameCamera(uintptr_t game,Listener*out);
int PatchPointer(void**slot,void*replacement,void**original);
extern volatile LONG gStop,gSent,gSendFail,gAccepted,gRejected,gGameSend,gGameRead;
extern wchar_t gRoot[1024];
#endif

/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator
 * Uses the existing game-owned SteamNetworking005 object; never initializes a second Steam client. */
#include "sync.h"
#include <string.h>
typedef struct { void**table;NetSend send;NetRead read;NetAvailable available; } Context;
static Context contexts[4];static void*current;static CRITICAL_SECTION netLock;
void NetInit(void){InitializeCriticalSection(&netLock);}
static Context*contextFor(void*self){int i;void**table=NULL;if(!ReadMemory((uintptr_t)self,&table,sizeof(table)))return NULL;for(i=0;i<4;i++)if(contexts[i].table==table)return &contexts[i];return NULL;}
static unsigned char __cdecl hookedSend(void*self,uint64_t id,const void*data,uint32_t bytes,int mode,int channel){Context*c;NetSend original=NULL;EnterCriticalSection(&netLock);c=contextFor(self);if(c){current=self;original=c->send;}LeaveCriticalSection(&netLock);if(channel!=MR_CHANNEL)SyncLearn(id,GetTickCount());InterlockedIncrement(&gGameSend);return original?original(self,id,data,bytes,mode,channel):0;}
static unsigned char __cdecl hookedRead(void*self,void*data,uint32_t cap,uint32_t*bytes,uint64_t*id,int channel){Context*c;NetRead original=NULL;unsigned char result;EnterCriticalSection(&netLock);c=contextFor(self);if(c){current=self;original=c->read;}LeaveCriticalSection(&netLock);result=original?original(self,data,cap,bytes,id,channel):0;if(result&&id&&channel!=MR_CHANNEL)SyncLearn(*id,GetTickCount());InterlockedIncrement(&gGameRead);return result;}
void NetAttach(void*iface){void**table=NULL;void*functions[3];int i,slot=-1;if(!iface||!ReadMemory((uintptr_t)iface,&table,sizeof(table))||!ReadMemory((uintptr_t)table,functions,sizeof(functions))||!functions[0]||!functions[1]||!functions[2])return;
 EnterCriticalSection(&netLock);for(i=0;i<4;i++){if(contexts[i].table==table){current=iface;LeaveCriticalSection(&netLock);return;}if(!contexts[i].table)slot=i;}if(slot<0){LeaveCriticalSection(&netLock);return;}
 contexts[slot].send=(NetSend)functions[0];contexts[slot].available=(NetAvailable)functions[1];contexts[slot].read=(NetRead)functions[2];contexts[slot].table=table;
 if(!PatchPointer(table,(void*)hookedSend,NULL)){memset(&contexts[slot],0,sizeof(Context));LeaveCriticalSection(&netLock);return;}
 if(!PatchPointer(table+2,(void*)hookedRead,NULL)){PatchPointer(table,functions[0],NULL);memset(&contexts[slot],0,sizeof(Context));LeaveCriticalSection(&netLock);return;}
 current=iface;LeaveCriticalSection(&netLock);Log("NET SteamNetworking005 attached; channel %d",MR_CHANNEL);}
void NetGet(void**iface,NetSend*send,NetRead*read,NetAvailable*available){Context*c;EnterCriticalSection(&netLock);*iface=current;c=contextFor(current);*send=c?c->send:NULL;*read=c?c->read:NULL;*available=c?c->available:NULL;LeaveCriticalSection(&netLock);}
void NetDiscover(void){typedef void*(__cdecl *ClientFn)(void);typedef int(__cdecl *HandleFn)(void);typedef void*(__cdecl *GetNetFn)(void*,int,int,const char*);HMODULE module;ClientFn clientFn;HandleFn userFn,pipeFn;GetNetFn getNet;void*client,*network;int user,pipe;void*iface;NetSend send;NetRead read;NetAvailable available;
 NetGet(&iface,&send,&read,&available);if(iface)return;module=GetModuleHandleA("steam_api64.dll");if(!module)return;clientFn=(ClientFn)GetProcAddress(module,"SteamClient");userFn=(HandleFn)GetProcAddress(module,"SteamAPI_GetHSteamUser");pipeFn=(HandleFn)GetProcAddress(module,"SteamAPI_GetHSteamPipe");getNet=(GetNetFn)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamNetworking");if(!clientFn||!userFn||!pipeFn)return;user=userFn();pipe=pipeFn();if(!user||!pipe)return;client=clientFn();if(!client)return;
 if(!getNet){void**vtable=NULL;if(!ReadMemory((uintptr_t)client,&vtable,sizeof(vtable))||!ReadMemory((uintptr_t)(vtable+16),&getNet,sizeof(getNet)))return;}
 network=getNet(client,user,pipe,"SteamNetworking005");if(network)NetAttach(network);}

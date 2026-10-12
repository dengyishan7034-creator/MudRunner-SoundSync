/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator
 * Adapter for the locally verified MudRunner 221004 x64 object layout. */
#include "sync.h"
#include <string.h>
#include <math.h>
static int finitef(float f){ return f==f && f>-10000000.0f && f<10000000.0f; }
static float clampf(float f,float lo,float hi){return f<lo?lo:f>hi?hi:f;}
int ReadMemory(uintptr_t address,void*out,size_t size){SIZE_T done=0; if(address<65536)return 0;return ReadProcessMemory(GetCurrentProcess(),(const void*)address,out,size,&done)&&done==size;}
static uintptr_t readPtr(uintptr_t a){uintptr_t p=0;ReadMemory(a,&p,sizeof(p));return p;}
static int readString(uintptr_t a,char*out,size_t cap){uint64_t n=0,c=0;uintptr_t p=a;if(!ReadMemory(a+16,&n,8)||!ReadMemory(a+24,&c,8)||!n||n>=cap||c<n||c>65536)return 0;if(c>=16)p=readPtr(a);if(!ReadMemory(p,out,(size_t)n))return 0;out[n]=0;return 1;}
int GameSnapshot(uintptr_t g,Packet*p){uintptr_t ctrl,truck,model,body;uint32_t flags=0,key=0;float xyz[3],normalized=0;memset(p,0,sizeof(*p));p->magic=MR_MAGIC;p->version=3;p->pitch=1.0f;
 ctrl=readPtr(g+0xbb81c8);if(!ctrl)return 0;if(readPtr(ctrl)!=g+0xa49870)return -1;
 truck=readPtr(ctrl+8);if(!truck)return 0;if(readPtr(truck)!=g+0xa4f6d8)return -1;
 model=readPtr(truck+0x10);if(!model||!readString(model+0x3a8,p->model,64))return -1;
 body=readPtr(model+0x1a0);if(!body||!ReadMemory(truck+0x5b0,&flags,4)||!ReadMemory(truck+0x24,&key,4)||!ReadMemory(body+0x1a0,xyz,12))return -1;
 p->x=xyz[0];p->y=xyz[1];p->z=xyz[2];if(!finitef(p->x)||!finitef(p->y)||!finitef(p->z)||fabs(p->x)>100000||fabs(p->y)>100000||fabs(p->z)>100000)return -1;
 p->engine=(flags&1)?1:0;p->entity=key;p->valid=1;
 /* Normalized engine parameter used by the game's engine update. This is not a measured RPM counter. */
 if(ReadMemory(model+0x410,&normalized,4)&&finitef(normalized)&&normalized>=0&&normalized<=2.5f)p->pitch=1.0f+0.50f*clampf(normalized,0,1);
 return 1;}
int GameGuard(uintptr_t base){unsigned char*g=(unsigned char*)base;IMAGE_DOS_HEADER d;IMAGE_NT_HEADERS64 n;unsigned char s[8];static const unsigned char expected[]={0x48,0x8d,0x05,0x21,0xc4,0xba,0x00,0xc3};
 if(!g||!ReadMemory((uintptr_t)g,&d,sizeof(d))||d.e_magic!=IMAGE_DOS_SIGNATURE||d.e_lfanew<=0||d.e_lfanew>4096)return 0;
 if(!ReadMemory((uintptr_t)g+d.e_lfanew,&n,sizeof(n))||n.Signature!=IMAGE_NT_SIGNATURE||n.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64)return 0;
 if(!ReadMemory((uintptr_t)g+0xbda0,s,8)||memcmp(s,expected,8))return 0;
 if(!ReadMemory((uintptr_t)g+0x89edb2,s,7)||memcmp(s,"\xf6\x86\xb0\x05\x00\x00\x01",7))return 0;
 if(!ReadMemory((uintptr_t)g+0x8ca309,s,7)||memcmp(s,"\x48\x81\xc1\xa8\x03\x00\x00",7))return 0;
 if(!ReadMemory((uintptr_t)g+0x1d6e0,s,8)||memcmp(s,"\x48\x8d\x05\x39\xab\xb9\x00\xc3",8))return 0;
 return 1;}

/* The render engine owns the final CAMERA view, including camera movement and
 * roll. No D3D interface, shaders, window or driver calls are required. */
int GameCamera(uintptr_t g,Listener*out){uintptr_t engine=readPtr(g+0xbb8220),camera;float view[16],again[16];if(!engine||readPtr(engine)!=g+0xa4a528)return 0;camera=readPtr(engine+8);if(!camera||!ReadMemory(camera,view,sizeof(view))||!ReadMemory(camera,again,sizeof(again))||memcmp(view,again,sizeof(view)))return 0;return CameraMath(view,out);}

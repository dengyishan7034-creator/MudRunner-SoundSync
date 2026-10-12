/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator */
#include "sync.h"
#include "win_audio.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#define ASSETS 24
#define CACHE_BYTES (64U*1024U*1024U)
#define RATE 44100
#define BLOCK 512
#define BUFFERS 6
typedef struct { char model[64];Sample idle,high;uint32_t used;size_t bytes;int loaded; } Asset;
typedef struct { uint64_t id;Packet state;uint32_t seen;int asset;float xyz[3],pitch,gain;double idlePhase,highPhase; } Voice;
static Asset assets[ASSETS];static Voice voices[MR_PEERS];static size_t cachedBytes;
static WaveOpenFn waveOutOpen;static WaveHeaderFn waveOutPrepareHeader,waveOutUnprepareHeader,waveOutWrite;static WaveSimpleFn waveOutReset,waveOutClose;
static int loadWaveApi(void){static HMODULE lib;wchar_t file[1024];UINT n;if(lib)return 1;n=GetSystemDirectoryW(file,1000);if(!n||n>=1000)return 0;wcscat(file,L"\\winmm.dll");lib=LoadLibraryW(file);if(!lib)return 0;waveOutOpen=(WaveOpenFn)GetProcAddress(lib,"waveOutOpen");waveOutPrepareHeader=(WaveHeaderFn)GetProcAddress(lib,"waveOutPrepareHeader");waveOutUnprepareHeader=(WaveHeaderFn)GetProcAddress(lib,"waveOutUnprepareHeader");waveOutWrite=(WaveHeaderFn)GetProcAddress(lib,"waveOutWrite");waveOutReset=(WaveSimpleFn)GetProcAddress(lib,"waveOutReset");waveOutClose=(WaveSimpleFn)GetProcAddress(lib,"waveOutClose");return waveOutOpen&&waveOutPrepareHeader&&waveOutUnprepareHeader&&waveOutWrite&&waveOutReset&&waveOutClose;}
static CRITICAL_SECTION mixLock;static HWAVEOUT device;static WAVEHDR headers[BUFFERS];static int16_t blocks[BUFFERS][BLOCK*2];static HANDLE audioThread;static volatile LONG audioStop;static LONG blocksWritten,outputFailures;
void MixerInit(void){int i;InitializeCriticalSection(&mixLock);for(i=0;i<MR_PEERS;i++)voices[i].asset=-1;}
static int assetActive(int index,uint32_t now){int i;for(i=0;i<MR_PEERS;i++)if(voices[i].id&&now-voices[i].seen<=3000&&voices[i].asset==index)return 1;return 0;}
static void freeAsset(int i){if(assets[i].loaded){cachedBytes-=assets[i].bytes;GameAudioFree(assets[i].idle.samples);GameAudioFree(assets[i].high.samples);}memset(&assets[i],0,sizeof(Asset));}
static int cachedAsset(const char*model,uint32_t now){int i,slot=-1;char idle[256],high[256];Sample a={0},b={0};size_t bytes;
 EnterCriticalSection(&mixLock);for(i=0;i<ASSETS;i++)if(assets[i].loaded&&!strcmp(assets[i].model,model)){assets[i].used=now;LeaveCriticalSection(&mixLock);return i;}LeaveCriticalSection(&mixLock);
 if(!ResolveModelSounds(model,idle,high,sizeof(idle))||!GameAudioLoadWave(idle,&a)||!GameAudioLoadWave(high,&b)){GameAudioFree(a.samples);GameAudioFree(b.samples);Log("AUDIO model=%s could not resolve/load installed game sounds",model);return -1;}
 bytes=(size_t)a.count*2+(size_t)b.count*2;a.fade=a.count/4<128?a.count/4:128;b.fade=b.count/4<128?b.count/4:128;
 EnterCriticalSection(&mixLock);while(cachedBytes+bytes>CACHE_BYTES){int oldest=-1;for(i=0;i<ASSETS;i++)if(assets[i].loaded&&!assetActive(i,now)&&(oldest<0||(int32_t)(assets[i].used-assets[oldest].used)<0))oldest=i;if(oldest<0)break;freeAsset(oldest);}
 for(i=0;i<ASSETS;i++)if(!assets[i].loaded){slot=i;break;}if(slot<0)for(i=0;i<ASSETS;i++)if(!assetActive(i,now)&&(slot<0||(int32_t)(assets[i].used-assets[slot].used)<0))slot=i;
 if(slot<0||cachedBytes+bytes>CACHE_BYTES){LeaveCriticalSection(&mixLock);GameAudioFree(a.samples);GameAudioFree(b.samples);Log("AUDIO cache full; no active sounds evicted");return -1;}
 freeAsset(slot);strcpy(assets[slot].model,model);assets[slot].idle=a;assets[slot].high=b;assets[slot].bytes=bytes;assets[slot].loaded=1;assets[slot].used=now;cachedBytes+=bytes;LeaveCriticalSection(&mixLock);Log("AUDIO model=%s idle=%s high=%s loaded",model,idle,high);return slot;}
void MixerUpdate(uint64_t id,const Packet*p,uint32_t now){int i,slot=-1,asset=-1;if(p->valid&&p->engine)asset=cachedAsset(p->model,now);EnterCriticalSection(&mixLock);
 for(i=0;i<MR_PEERS;i++){if(voices[i].id==id){slot=i;break;}if(!voices[i].id||now-voices[i].seen>3000)slot=i;}if(slot>=0){Voice*v=&voices[slot];if(v->id!=id||v->state.session!=p->session||v->state.entity!=p->entity||strcmp(v->state.model,p->model)){memset(v,0,sizeof(*v));v->id=id;v->asset=-1;v->xyz[0]=p->x;v->xyz[1]=p->y;v->xyz[2]=p->z;v->pitch=p->pitch;}v->state=*p;v->seen=now;if(asset>=0)v->asset=asset;if(!p->valid)v->state.engine=0;}LeaveCriticalSection(&mixLock);}
static float at(const Sample*s,uint32_t i){const int16_t*p=(const int16_t*)s->samples;uint32_t fade=s->fade;float a=p[i];if(fade&&i<fade){float w=(float)i/fade;a=(1-w)*p[s->count-fade+i]+w*a;}return a;}
static float sample(const Sample*s,double*phase,float pitch){uint32_t length=s->count-s->fade,i,next;double f;float value;if(!s->samples||!length)return 0;while(*phase>=length)*phase-=length;i=(uint32_t)*phase;f=*phase-i;next=i+1==length?0:i+1;value=(float)((1-f)*at(s,i)+f*at(s,next));*phase+=(double)pitch*s->rate/RATE;return value;}
void MixerRender(int16_t*out,uint32_t frames,uint32_t now,const Listener*l){uint32_t n;int i;float gainL[MR_PEERS],gainR[MR_PEERS],target[MR_PEERS],blend[MR_PEERS];memset(out,0,frames*2*sizeof(int16_t));if(!l||!l->valid)return;
 EnterCriticalSection(&mixLock);for(i=0;i<MR_PEERS;i++){Voice*v=&voices[i];float dx,dy,dz,distance,pan=0,atten=0,smoothing=1-(float)exp(-(double)frames/(RATE*0.06));v->xyz[0]+=(v->state.x-v->xyz[0])*smoothing;v->xyz[1]+=(v->state.y-v->xyz[1])*smoothing;v->xyz[2]+=(v->state.z-v->xyz[2])*smoothing;v->pitch+=(v->state.pitch-v->pitch)*smoothing;
 dx=v->xyz[0]-l->position[0];dy=v->xyz[1]-l->position[1];dz=v->xyz[2]-l->position[2];distance=(float)sqrt(dx*dx+dy*dy+dz*dz);if(distance>0.01f)pan=(dx*l->right[0]+dy*l->right[1]+dz*l->right[2])/distance;if(pan>1)pan=1;if(pan<-1)pan=-1;
 if(v->id&&now-v->seen<=3000&&v->state.valid&&v->state.engine&&v->asset>=0&&assets[v->asset].loaded&&distance<200)atten=0.45f/(1+distance*distance/625);target[i]=atten;gainL[i]=(float)sqrt((1-pan)*0.5f);gainR[i]=(float)sqrt((1+pan)*0.5f);blend[i]=(v->pitch-1)*2;if(blend[i]<0)blend[i]=0;if(blend[i]>1)blend[i]=1;}
 for(n=0;n<frames;n++){float left=0,right=0;for(i=0;i<MR_PEERS;i++){Voice*v=&voices[i];float x;if(v->asset<0||!assets[v->asset].loaded)continue;v->gain+=(target[i]-v->gain)*0.00075f;if(v->gain<0.00001f)continue;x=(1-blend[i])*sample(&assets[v->asset].idle,&v->idlePhase,v->pitch)+blend[i]*sample(&assets[v->asset].high,&v->highPhase,v->pitch);left+=x*v->gain*gainL[i];right+=x*v->gain*gainR[i];}if(left>32767)left=32767;if(left<-32768)left=-32768;if(right>32767)right=32767;if(right<-32768)right=-32768;out[n*2]=(int16_t)left;out[n*2+1]=(int16_t)right;}
 LeaveCriticalSection(&mixLock);}
void MixerPump(uint32_t now,const Listener*l){int i;for(i=0;i<BUFFERS;i++)if(!(headers[i].dwFlags&WHDR_INQUEUE)){MixerRender(blocks[i],BLOCK,now,l);if(waveOutWrite(device,&headers[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR)InterlockedIncrement(&outputFailures);else InterlockedIncrement(&blocksWritten);}}
static DWORD WINAPI audioMain(void*unused){while(!audioStop&&!gStop){Listener l={0};CameraGet(&l);MixerPump(GetTickCount(),&l);Sleep(4);}return 0;}
int MixerOpen(void){WAVEFORMATEX format;int i;MMRESULT result;memset(&format,0,sizeof(format));format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=RATE;format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=RATE*4;
 if(!loadWaveApi())return 0;result=waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);if(result!=MMSYSERR_NOERROR){Log("AUDIO output open failed code=%u",result);return 0;}for(i=0;i<BUFFERS;i++){headers[i].lpData=(char*)blocks[i];headers[i].dwBufferLength=sizeof(blocks[i]);result=waveOutPrepareHeader(device,&headers[i],sizeof(WAVEHDR));if(result!=MMSYSERR_NOERROR){Log("AUDIO prepare failed code=%u",result);MixerClose();return 0;}}
 audioStop=0;audioThread=CreateThread(NULL,0,audioMain,NULL,0,NULL);if(!audioThread){MixerClose();return 0;}Log("AUDIO output running 44100 Hz stereo, 6 x 512 frames, cache 64 MiB");return 1;}
void MixerClose(void){int i;audioStop=1;if(audioThread){WaitForSingleObject(audioThread,3000);CloseHandle(audioThread);audioThread=NULL;}if(device){waveOutReset(device);for(i=0;i<BUFFERS;i++)if(headers[i].dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(device,&headers[i],sizeof(WAVEHDR));waveOutClose(device);device=NULL;}}
#ifdef TEST_BUILD
__declspec(dllexport) void TestAudioStats(LONG*written,LONG*failures){*written=blocksWritten;*failures=outputFailures;}
__declspec(dllexport) uint32_t TestCacheBytes(void){return (uint32_t)cachedBytes;}
__declspec(dllexport) void TestClearMixer(void){int i;EnterCriticalSection(&mixLock);for(i=0;i<ASSETS;i++)freeAsset(i);memset(voices,0,sizeof(voices));for(i=0;i<MR_PEERS;i++)voices[i].asset=-1;LeaveCriticalSection(&mixLock);}
#endif

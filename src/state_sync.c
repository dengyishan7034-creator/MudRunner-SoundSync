/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator */
#include "sync.h"
#include <string.h>
#include <math.h>
typedef struct { uint64_t id; uint32_t learned,received,session,seq; int hasState; } Peer;
static Peer peers[MR_PEERS];
static CRITICAL_SECTION stateLock;
static uint32_t session,sequence,lastSent,lastStats;static char lastModel[64];static int lastEngine=-1;
volatile LONG gStop,gSent,gSendFail,gAccepted,gRejected,gGameSend,gGameRead;
static int indexOf(uint64_t id,uint32_t now,int create){int i,freeIndex=-1;if(!id)return -1;for(i=0;i<MR_PEERS;i++){if(peers[i].id==id)return i;if(!peers[i].id||now-peers[i].learned>30000)freeIndex=i;}if(!create||freeIndex<0)return -1;memset(&peers[freeIndex],0,sizeof(Peer));peers[freeIndex].id=id;return freeIndex;}
void SyncInit(void){InitializeCriticalSection(&stateLock);session=GetTickCount()^(GetCurrentProcessId()*2654435761U);if(!session)session=1;}
void SyncLearn(uint64_t id,uint32_t now){int i;EnterCriticalSection(&stateLock);i=indexOf(id,now,1);if(i>=0)peers[i].learned=now;LeaveCriticalSection(&stateLock);}
int PacketValid(const Packet*p,uint32_t bytes){int i;if(!p||bytes!=sizeof(Packet)||p->magic!=MR_MAGIC||p->version!=3||p->valid>1||p->engine>1||!p->session||!memchr(p->model,0,64))return 0;
 if(!isfinite(p->pitch)||p->pitch<0.5f||p->pitch>3||!isfinite(p->x)||!isfinite(p->y)||!isfinite(p->z)||fabs(p->x)>100000||fabs(p->y)>100000||fabs(p->z)>100000)return 0;
 if(p->valid&&!p->model[0])return 0;for(i=0;p->model[i];i++){unsigned char c=p->model[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return 0;}return 1;}
int SyncReceive(uint64_t id,const Packet*p,uint32_t size,uint32_t now){int i,accepted=0;if(!id||!PacketValid(p,size)){InterlockedIncrement(&gRejected);return 0;}EnterCriticalSection(&stateLock);i=indexOf(id,now,1);
 if(i>=0){Peer*q=&peers[i];if(!q->hasState||q->session!=p->session||(int32_t)(p->seq-q->seq)>0){q->learned=q->received=now;q->session=p->session;q->seq=p->seq;q->hasState=1;accepted=1;}}
 LeaveCriticalSection(&stateLock);if(!accepted)return 0;InterlockedIncrement(&gAccepted);MixerUpdate(id,p,now);return 1;}
void SyncStep(uintptr_t game,uint32_t now){void*iface=NULL;NetSend send=NULL;NetRead read=NULL;NetAvailable available=NULL;int i,count=0;uint64_t ids[MR_PEERS];Packet p;NetGet(&iface,&send,&read,&available);
 if(now-lastSent>=100){lastSent=now;GameSnapshot(game,&p);CameraReferenceUpdate(&p,now);p.session=session;p.seq=++sequence;if(strcmp(lastModel,p.model)||lastEngine!=p.engine){strcpy(lastModel,p.model);lastEngine=p.engine;Log("LOCAL valid=%u model=%s engine=%u pitch=%.3f xyz=(%.2f %.2f %.2f)",p.valid,p.model,p.engine,p.pitch,p.x,p.y,p.z);}
 EnterCriticalSection(&stateLock);for(i=0;i<MR_PEERS;i++){ids[i]=(now-peers[i].learned<30000)?peers[i].id:0;if(ids[i])count++;}LeaveCriticalSection(&stateLock);
 if(iface&&send)for(i=0;i<MR_PEERS;i++)if(ids[i]){if(send(iface,ids[i],&p,sizeof(p),0,MR_CHANNEL))InterlockedIncrement(&gSent);else InterlockedIncrement(&gSendFail);}}
 if(iface&&available&&read)for(i=0;i<32;i++){uint32_t n=0,got=0;uint64_t id=0;unsigned char small[512],*buf=small;if(!available(iface,&n,MR_CHANNEL))break;if(n>1024U*1024U){InterlockedIncrement(&gRejected);break;}if(n>sizeof(small)){buf=(unsigned char*)HeapAlloc(GetProcessHeap(),0,n);if(!buf)break;}
 if(!read(iface,buf,n>sizeof(small)?n:sizeof(small),&got,&id,MR_CHANNEL)){if(buf!=small)HeapFree(GetProcessHeap(),0,buf);break;}
 if(got==sizeof(Packet))SyncReceive(id,(Packet*)buf,got,now);else InterlockedIncrement(&gRejected);if(buf!=small)HeapFree(GetProcessHeap(),0,buf);}
 if(now-lastStats>=5000){lastStats=now;Log("NET ready=%d peers=%d camera=%d gameSend=%ld gameRead=%ld tx=%ld failed=%ld accepted=%ld rejected=%ld",iface&&send&&read&&available?1:0,count,CameraNativeFresh(),gGameSend,gGameRead,gSent,gSendFail,gAccepted,gRejected);}}
#ifdef TEST_BUILD
__declspec(dllexport) void TestResetSync(void){EnterCriticalSection(&stateLock);memset(peers,0,sizeof(peers));sequence=lastSent=lastStats=0;gSent=gSendFail=gAccepted=gRejected=gGameSend=gGameRead=0;session=77;LeaveCriticalSection(&stateLock);}
#endif

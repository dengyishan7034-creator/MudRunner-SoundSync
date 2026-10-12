/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator */
#include "sync.h"
#include <string.h>
#include <math.h>
static CRITICAL_SECTION cameraLock;static Listener latest,reference;
void CameraInit(void){InitializeCriticalSection(&cameraLock);}
int CameraMath(const float*m,Listener*out){int i,j;float*axes[3];Listener l={0};for(i=0;i<16;i++)if(!isfinite(m[i]))return 0;
 if(fabs(m[3])>0.01||fabs(m[7])>0.01||fabs(m[11])>0.01||fabs(m[15]-1)>0.01)return 0;
 for(i=0;i<3;i++)l.position[i]=-(m[12]*m[i*4]+m[13]*m[i*4+1]+m[14]*m[i*4+2]);axes[0]=l.right;axes[1]=l.up;axes[2]=l.forward;
 for(j=0;j<3;j++){float length=0;for(i=0;i<3;i++){axes[j][i]=m[i*4+j];length+=axes[j][i]*axes[j][i];}length=(float)sqrt(length);if(length<0.8f||length>1.2f)return 0;for(i=0;i<3;i++)axes[j][i]/=length;}
 if(fabs(l.right[0]*l.up[0]+l.right[1]*l.up[1]+l.right[2]*l.up[2])>0.05f)return 0;for(i=0;i<3;i++)if(!isfinite(l.position[i])||fabs(l.position[i])>100000)return 0;l.valid=1;*out=l;return 1;}
void CameraCapture(const float*view){Listener l;if(!CameraMath(view,&l))return;l.tick=GetTickCount();EnterCriticalSection(&cameraLock);latest=l;LeaveCriticalSection(&cameraLock);}
void CameraReferenceUpdate(const Packet*p,uint32_t now){EnterCriticalSection(&cameraLock);memset(&reference,0,sizeof(reference));reference.valid=p->valid;reference.tick=now;reference.position[0]=p->x;reference.position[1]=p->y;reference.position[2]=p->z;LeaveCriticalSection(&cameraLock);}
int CameraReference(float*position){int valid;EnterCriticalSection(&cameraLock);valid=reference.valid&&GetTickCount()-reference.tick<=1000;memcpy(position,reference.position,sizeof(reference.position));LeaveCriticalSection(&cameraLock);return valid;}
int CameraNativeFresh(void){int valid;EnterCriticalSection(&cameraLock);valid=latest.valid&&GetTickCount()-latest.tick<=1000;LeaveCriticalSection(&cameraLock);return valid;}
int CameraGet(Listener*out){uint32_t now=GetTickCount();EnterCriticalSection(&cameraLock);if(!reference.valid||now-reference.tick>1000){memset(out,0,sizeof(*out));LeaveCriticalSection(&cameraLock);return 0;}if(latest.valid&&now-latest.tick<=1000)*out=latest;else *out=reference;LeaveCriticalSection(&cameraLock);if(!out->valid||now-out->tick>1000){out->valid=0;return 0;}return 1;}

#ifdef TEST_BUILD
__declspec(dllexport) void TestReference(const Packet*p,uint32_t now){CameraReferenceUpdate(p,now);}
__declspec(dllexport) int TestListener(Listener*out){return CameraGet(out);}
#endif

void CameraPublish(const Listener*value){Listener l=*value;l.tick=GetTickCount();EnterCriticalSection(&cameraLock);latest=l;LeaveCriticalSection(&cameraLock);}

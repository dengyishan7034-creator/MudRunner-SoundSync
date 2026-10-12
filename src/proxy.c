/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator */
#include "sync.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#define EXPORT __declspec(dllexport)
wchar_t gRoot[1024];static HMODULE module,systemInput;static HANDLE worker;static CRITICAL_SECTION logLock,proxyLock;static FILE*logFile;
void Log(const char*format,...){va_list args;EnterCriticalSection(&logLock);if(logFile){fprintf(logFile,"[%10u] ",GetTickCount());va_start(args,format);vfprintf(logFile,format,args);va_end(args);fputc('\n',logFile);fflush(logFile);}LeaveCriticalSection(&logLock);}
typedef HRESULT(WINAPI*InputCreateFn)(HINSTANCE,DWORD,const GUID*,void**,void*);
typedef HRESULT(WINAPI*ClassFn)(const GUID*,const GUID*,void**);
static InputCreateFn systemCreate;
static ClassFn systemClass;
/* Resolve the Windows module's own export directory, rather than an overlay's
 * name-based GetProcAddress wrapper that may point back to this proxy. */
static void*systemExport(HMODULE handle,const char*name){uintptr_t base=(uintptr_t)handle;IMAGE_DOS_HEADER*d=(IMAGE_DOS_HEADER*)base;IMAGE_NT_HEADERS64*n;IMAGE_DATA_DIRECTORY dir;IMAGE_EXPORT_DIRECTORY*e;DWORD*names,*functions;WORD*ordinals;DWORD i,rva;
 if(!handle||d->e_magic!=IMAGE_DOS_SIGNATURE)return NULL;n=(IMAGE_NT_HEADERS64*)(base+d->e_lfanew);dir=n->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];if(!dir.VirtualAddress)return NULL;e=(IMAGE_EXPORT_DIRECTORY*)(base+dir.VirtualAddress);names=(DWORD*)(base+e->AddressOfNames);functions=(DWORD*)(base+e->AddressOfFunctions);ordinals=(WORD*)(base+e->AddressOfNameOrdinals);
 for(i=0;i<e->NumberOfNames;i++)if(!strcmp((char*)(base+names[i]),name)){if(ordinals[i]>=e->NumberOfFunctions)return NULL;rva=functions[ordinals[i]];if(rva>=dir.VirtualAddress&&rva<dir.VirtualAddress+dir.Size)return NULL;return(void*)(base+rva);}return NULL;}
static HMODULE realInput(void){wchar_t file[1024];EnterCriticalSection(&proxyLock);if(!systemInput){UINT n=GetSystemDirectoryW(file,1000);if(n&&n<1000){wcscat(file,L"\\dinput8.dll");systemInput=LoadLibraryW(file);if(systemInput){systemCreate=(InputCreateFn)systemExport(systemInput,"DirectInput8Create");systemClass=(ClassFn)systemExport(systemInput,"DllGetClassObject");}}}LeaveCriticalSection(&proxyLock);return systemInput;}
EXPORT HRESULT WINAPI DirectInput8Create(HINSTANCE instance,DWORD version,const GUID*iid,void**out,void*outer){realInput();return systemCreate?systemCreate(instance,version,iid,out,outer):E_FAIL;}
EXPORT HRESULT WINAPI DllGetClassObject(const GUID*cls,const GUID*iid,void**out){realInput();return systemClass?systemClass(cls,iid,out):CLASS_E_CLASSNOTAVAILABLE;}
EXPORT HRESULT WINAPI DllCanUnloadNow(void){/* Hooks remain callable for process lifetime. */return S_FALSE;}
EXPORT HRESULT WINAPI DllRegisterServer(void){return E_NOTIMPL;}
EXPORT HRESULT WINAPI DllUnregisterServer(void){return E_NOTIMPL;}
static DWORD WINAPI runWorker(void*unused){realInput();wchar_t file[1024];uintptr_t game=(uintptr_t)GetModuleHandleW(NULL);uint32_t discover=0,retryAudio=0;int guard=GameGuard(game),audio=0;typedef BOOL(WINAPI*PinFn)(DWORD,LPCWSTR,HMODULE*);PinFn pin=(PinFn)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetModuleHandleExW");HMODULE pinned;if(pin)pin(1|4,(LPCWSTR)(uintptr_t)&runWorker,&pinned);
 if(wcslen(gRoot)+14<1024){wcscpy(file,gRoot);wcscat(file,L"\\mr_sync.log");logFile=_wfopen(file,L"w");}
 Log("PROXY bound Windows input exports directly; overlay resolver bypassed");
 Log("MudRunner SoundSync %s standalone source build; one DLL; gameGuard=%d",MR_VERSION,guard);Log("SOURCE https://github.com/dengyishan7034-creator/MudRunner-SoundSync ; AGPL-3.0-only");
 if(!guard){Log("Unsupported game/process: synchronization and audio remain disabled; DirectInput proxy available");return 0;}GameAudioSetRoot(gRoot);audio=MixerOpen();
 while(!gStop){uint32_t now=GetTickCount();if(now-discover>=1000){discover=now;NetDiscover();}if(!audio&&now-retryAudio>=5000){retryAudio=now;audio=MixerOpen();}{Listener l;if(GameCamera(game,&l)){/* Publish a checked native listener directly. */CameraPublish(&l);}}SyncStep(game,now);Sleep(10);}
 MixerClose();Log("STOP complete");EnterCriticalSection(&logLock);if(logFile){fclose(logFile);logFile=NULL;}LeaveCriticalSection(&logLock);return 0;}
EXPORT void WINAPI SoundSyncStop(void){InterlockedExchange(&gStop,1);if(worker&&WaitForSingleObject(worker,5000)==WAIT_OBJECT_0){CloseHandle(worker);worker=NULL;}}
#ifdef TEST_BUILD
EXPORT void*TestSystemInputExport(void){realInput();return(void*)systemCreate;}
EXPORT int TestSnapshot(uintptr_t base,Packet*out){return GameSnapshot(base,out);}
EXPORT int TestGuard(uintptr_t base){return GameGuard(base);}
EXPORT int TestPacket(const Packet*p,uint32_t size){return PacketValid(p,size);}
EXPORT int TestNativeCamera(uintptr_t base,Listener*out){return GameCamera(base,out);}
EXPORT int TestCamera(const float*matrix,Listener*out){return CameraMath(matrix,out);}
EXPORT void TestRoot(const wchar_t*root){GameAudioSetRoot(root);}
EXPORT int TestResolve(const char*model,char*idle,char*high,uint32_t cap){return ResolveModelSounds(model,idle,high,cap);}
EXPORT int TestWave(const char*path,Sample*out){return GameAudioLoadWave(path,out);}
EXPORT int TestInflate(const unsigned char*data,size_t size,unsigned char*out,size_t cap){return GameAudioInflate(data,size,out,cap);}
EXPORT void TestFree(void*p){GameAudioFree(p);}
EXPORT void TestAttachNet(void*p){NetAttach(p);}
EXPORT void TestStep(uintptr_t game,uint32_t now){SyncStep(game,now);}
EXPORT int TestReceive(uint64_t id,const Packet*p,uint32_t size,uint32_t now){return SyncReceive(id,p,size,now);}
EXPORT void TestMix(int16_t*out,uint32_t frames,uint32_t now,const Listener*listener){MixerRender(out,frames,now,listener);}
EXPORT int TestAudioOpen(void){return MixerOpen();}
EXPORT void TestAudioClose(void){MixerClose();}
#endif
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID reserved){if(reason==DLL_PROCESS_ATTACH){int i;module=h;GetModuleFileNameW(h,gRoot,1024);gRoot[1023]=0;for(i=(int)wcslen(gRoot)-1;i>=0;i--)if(gRoot[i]=='\\'||gRoot[i]=='/'){gRoot[i]=0;break;}InitializeCriticalSection(&logLock);InitializeCriticalSection(&proxyLock);SyncInit();MixerInit();CameraInit();NetInit();DisableThreadLibraryCalls(h);
#ifndef TEST_BUILD
 worker=CreateThread(NULL,0,runWorker,NULL,0,NULL);
#endif
 }else if(reason==DLL_PROCESS_DETACH)InterlockedExchange(&gStop,1);return TRUE;}

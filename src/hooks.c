/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator
 * Atomic pointer replacement for the game-owned networking interface only. */
#include "sync.h"
int PatchPointer(void**slot,void*replacement,void**original){DWORD old,ignored;void*before=NULL;MEMORY_BASIC_INFORMATION info;if(!ReadMemory((uintptr_t)slot,&before,sizeof(before))||!before)return 0;if(before==replacement)return 1;
 if(!VirtualQuery(before,&info,sizeof(info))||!(info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return 0;
 if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return 0;if(original)*original=before;InterlockedExchangePointer((PVOID volatile*)slot,replacement);VirtualProtect(slot,sizeof(void*),old,&ignored);return 1;}
#ifdef TEST_BUILD
__declspec(dllexport) int TestPatch(void**slot,void*replace,void**old){return PatchPointer(slot,replace,old);}
#endif

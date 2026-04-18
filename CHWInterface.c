#include "CHWEngine.h"
#include <stdio.h>
#include <wchar.h>

#define DEFINE_HW_STR_GETTER(hwName, name, member, PluralSuffix) \
DLLIMPORT void Get##hwName##name(int index, LPWSTR buffer, int maxLen) { \
if (buffer == NULL || maxLen <= 0) return; \
if (!g_Status.hwName) { \
Probe##hwName##PluralSuffix(&g_Cache); \
g_Status.hwName = TRUE; \
} \
if (index >= 0 && index < g_Cache.hwName##Count) { \
lstrcpynW(buffer, g_Cache.hwName##s[index].member, maxLen); \
} else { \
buffer[0] = L'\0'; \
} \
}

#define DEFINE_HW_COUNT_GETTER(hwName,PluralSuffix) \
DLLIMPORT int Get##hwName##Count() { \
if (!g_Status.hwName) { \
Probe##hwName##PluralSuffix(&g_Cache); \
g_Status.hwName = TRUE; \
} \
return g_Cache.hwName##Count; \
}

static HW_REPORT g_Cache;

static struct {
	BOOL Cpu;
	BOOL Board;
	BOOL Ram;
} g_Status = { FALSE };


//DLLIMPORT void GetCpuModel(int index, LPWSTR buffer, int maxLen){
//	if (buffer == NULL || maxLen <= 0) return;
//	if (!g_Status.Cpu){
//		ProbeCpu(&g_Cache);
//		g_Status.Cpu = TRUE;
//	}
//	if (index >= 0 && index < g_Cache.CpuCount) {
//		lstrcpynW(buffer, g_Cache.Cpus[index].Model,maxLen);
//	} else {
//		buffer[0] = L'\0';
//	}
//}

//DEFINE_HW_STR_GETTER(Cpu,Model,Model);
//DEFINE_HW_COUNT_GETTER(Cpu);

//DLLIMPORT int GetCpuCount(){
//	if (!g_Status.Cpu) {
//		ProbeCpu(&g_Cache);
//		g_Status.Cpu = TRUE;
//	}
//	return g_Cache.CpuCount;
//}

static void ProbeBoard(HW_REPORT* cache) { ProbeBoardAndRam(cache); }
static void ProbeRams(HW_REPORT* cache)   { ProbeBoardAndRam(cache); }

#define X(hw,name,mem,suff) DEFINE_HW_STR_GETTER(hw,name,mem,suff)
HW_STR_FIELDS
#undef X

#define X(hw,suff) DEFINE_HW_COUNT_GETTER(hw,suff)
HW_COUNT_FIELDS
#undef X

DLLIMPORT int GetCpuCoreCount(int index){
	if (!g_Status.Cpu) {
		ProbeCpus(&g_Cache);
		g_Status.Cpu = TRUE;
	}
	return g_Cache.Cpus[index].CoreCount;
}

DLLIMPORT int GetCpuThreadCount(int index){
	if (!g_Status.Cpu) {
		ProbeCpus(&g_Cache);
		g_Status.Cpu = TRUE;
	}
	return g_Cache.Cpus[index].ThreadCount;
}




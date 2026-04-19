#include "CHWEngine.h"
#include <stdio.h>
#include <wchar.h>

#define DEFINE_HW_STR_GETTER(hwName, name, member) \
DLLIMPORT void Get##hwName##name(int index, LPWSTR buffer, int maxLen) { \
if (buffer == NULL || maxLen <= 0) return; \
if (!g_Status.hwName) { \
Probe##hwName##s(&g_Cache); \
g_Status.hwName = TRUE; \
} \
if (index >= 0 && index < g_Cache.hwName##Count) { \
lstrcpynW(buffer, g_Cache.hwName##s[index].member, maxLen); \
} else { \
buffer[0] = L'\0'; \
} \
}

#define DEFINE_HW_COUNT_GETTER(hwName) \
DLLIMPORT int Get##hwName##Count() { \
if (!g_Status.hwName) { \
Probe##hwName##s(&g_Cache); \
g_Status.hwName = TRUE; \
} \
return g_Cache.hwName##Count; \
}


#define DEFINE_HW_VAL_GETTER(hwName, name, member, type, failVal) \
DLLIMPORT type Get##hwName##name(int index) { \
if (!g_Status.hwName) { \
Probe##hwName##s(&g_Cache); \
g_Status.hwName = TRUE; \
} \
if (index >= 0 && index < g_Cache.hwName##Count) { \
return g_Cache.hwName##s[index].member; \
} \
return (type)failVal; \
}

#define DEFINE_MONITOR_STR_GETTER(name, member, suffix) \
DLLIMPORT void GetMonitor##name##suffix(int index, LPWSTR buffer, int maxLen) { \
if (buffer == NULL || maxLen <= 0) return; \
if (!g_Status.Monitor##suffix) { \
ProbeMonitors##suffix(&g_Cache); \
g_Status.Monitor##suffix = TRUE; \
} \
if (index >= 0 && index < g_Cache.MonitorCount) { \
lstrcpynW(buffer, g_Cache.Monitors[index].member, maxLen); \
} else { \
buffer[0] = L'\0'; \
} \
}

#define DEFINE_MONITOR_VAL_GETTER(name, member, type, failVal, suffix) \
DLLIMPORT type GetMonitor##name##suffix(int index) { \
if (!g_Status.Monitor) { \
ProbeMonitors##suffix(&g_Cache); \
g_Status.Monitor = TRUE; \
} \
if (index >= 0 && index < g_Cache.MonitorCount) { \
return g_Cache.Monitors[index].member; \
} \
return (type)failVal; \
}

static HW_REPORT g_Cache;

static struct {
	BOOL Cpu;
	BOOL Board;
	BOOL Ram;
	BOOL Disk;
	BOOL Gpu;
	BOOL Monitor;
	BOOL MonitorWmi;
	BOOL Audio;
	BOOL Nic;
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

static void ProbeBoards(HW_REPORT* cache) { ProbeBoardsAndRams(cache); }
static void ProbeRams(HW_REPORT* cache)   { ProbeBoardsAndRams(cache); }

#define X(hw,name,mem) DEFINE_HW_STR_GETTER(hw,name,mem)
HW_STR_FIELDS
#undef X

#define X(hw) DEFINE_HW_COUNT_GETTER(hw)
HW_COUNT_FIELDS
#undef X


#define X(hw,name,mem,type) DEFINE_HW_VAL_GETTER(hw,name,mem,type,0)
HW_VAL_FIELDS
#undef X

#define X(name,mem,suffix) DEFINE_MONITOR_STR_GETTER(name,mem,suffix)
MONITOR_STR_FIELDS
#undef X

#define X(name,mem,type,suffix) DEFINE_MONITOR_VAL_GETTER(name,mem,type,0,suffix)
MONITOR_VAL_FIELDS
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




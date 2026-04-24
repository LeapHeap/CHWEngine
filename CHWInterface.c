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

DLLIMPORT void CALLBACK ExportReportToFile(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	double elapsedMilliseconds;
	/* Time count*/
	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;
	
	// 1. Initialize Frequency
	QueryPerformanceFrequency(&frequency);
	
	// 2. Start Timer
	QueryPerformanceCounter(&start);
	
	HW_REPORT report = {0};
	
	ProbeCpus(&report);
	ProbeGpus(&report);
	ProbeBoardsAndRams(&report);
	ProbeNics(&report);
	ProbeDisks(&report);
	ProbeMonitors(&report);
	ProbeAudios(&report);
	
	// 3. End Timer
	QueryPerformanceCounter(&end);
	
	// 4. Calculate Duration
	elapsedMilliseconds = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
	
	wchar_t* buffer = (wchar_t*)malloc(128 * 1024 * sizeof(wchar_t));
	if (!buffer) return;
	
	wchar_t* p = buffer;
	int remaining = 128 * 1024;
	int n;
	
#define APPEND_LINE(fmt, ...) \
	n = swprintf_s(p, remaining, fmt, ##__VA_ARGS__); \
	if (n > 0) { p += n; remaining -= n; }
	
	APPEND_LINE(L"--------------------------------------------------\n");
	APPEND_LINE(L"CHWEngine SDK - Hardware Report\n");
	APPEND_LINE(L"--------------------------------------------------\n");
	
	APPEND_LINE(L"\n[Motherboard] Count: %d\n", report.BoardCount);
	for (int i = 0; i < report.BoardCount; i++) {
		const BOARD_INFO* board = &report.Boards[i];
		APPEND_LINE(L"Board #%d:\n", i);
		APPEND_LINE(L"  Manufacturer: %s\n", board->Manufacturer);
		APPEND_LINE(L"  System Model: %s\n", board->SystemName);
		APPEND_LINE(L"  Board Model:  %s\n", board->Model);
		APPEND_LINE(L"  Chipset:      %s\n", board->ChipsetName);
		APPEND_LINE(L"  BIOS:         %s\n", board->BiosVersion);
	}
	
	APPEND_LINE(L"\n[CPU] Count: %d\n", report.CpuCount);
	for (int i = 0; i < report.CpuCount; i++) {
		const CPU_INFO* cpu = &report.Cpus[i];
		APPEND_LINE(L"Socket #%d: %s | %dC%dT\n", i, cpu->Model, cpu->CoreCount, cpu->ThreadCount);
	}
	
	APPEND_LINE(L"\n[RAM] Count: %d\n", report.RamCount);
	for (int i = 0; i < report.RamCount; i++) {
		const RAM_INFO* ram = &report.Rams[i];
		APPEND_LINE(L"RAM #%d: %s | %s | %u MB | %u MT/s\n", i, ram->Manufacturer, ram->Type, ram->CapacityMb, ram->SpeedMts);
	}
	
	APPEND_LINE(L"\n[GPU] Count: %d\n", report.GpuCount);
	for (int i = 0; i < report.GpuCount; i++) {
		const GPU_INFO* gpu = &report.Gpus[i];
		double totalBytes = (double)gpu->VRamSizeByte;
		if (totalBytes >= 1073741824.0) {
			APPEND_LINE(L"GPU #%d: %s (%.1fGB / %s)\n", i, gpu->Model, totalBytes / 1073741824.0, gpu->SubVendor);
		} else {
			APPEND_LINE(L"GPU #%d: %s (%.0fMB / %s)\n", i, gpu->Model, totalBytes / 1048576.0, gpu->SubVendor);
		}
	}
	
	APPEND_LINE(L"\n[Monitor] Count: %d\n", report.MonitorCount);
	for (int i = 0; i < report.MonitorCount; i++) {
		const MONITOR_INFO* mon = &report.Monitors[i];
		float rounded = mon->Diagonal + 0.05f;
		int dWhole = (int)rounded;
		int dFrac = (int)((rounded - (float)dWhole) * 10.0f);
		APPEND_LINE(L"Monitor #%d: %s %s [%s%s] (%d) | %d x %d | %d.%d Inch\n", 
					i, mon->VendorName, mon->Model[0] ? mon->Model : L"Unknown Model", 
					mon->VendorId, mon->ProductId, mon->Year, mon->PhysWidth, mon->PhysHeight, dWhole, dFrac);
	}
	
	APPEND_LINE(L"\n[Storage] Count: %d\n", report.DiskCount);
	for (int i = 0; i < report.DiskCount; i++) {
		const DISK_INFO* disk = &report.Disks[i];
		APPEND_LINE(L"Disk #%d: %s | %u GB\n", i, disk->Model, (unsigned int)(disk->TotalSizeByte / 1073741824));
	}
	
	APPEND_LINE(L"\n[NIC] Count: %d\n", report.NicCount);
	for (int i = 0; i < report.NicCount; i++) {
		APPEND_LINE(L"NIC #%d: %s\n", i, report.Nics[i].Model);
	}
	
	APPEND_LINE(L"\n[Audio] Count: %d\n", report.AudioCount);
	for (int i = 0; i < report.AudioCount; i++) {
		APPEND_LINE(L"Audio #%d: %s\n", i, report.Audios[i].Model);
	}
	
	WCHAR timeBuf[128];
	int tWhole = (int)elapsedMilliseconds;
	int tFrac = (int)((elapsedMilliseconds - (double)tWhole) * 100.0);
	
	APPEND_LINE(L"\n--------------------------------------------------\n");
	APPEND_LINE(L"Detection completed in: %d.%02d ms\n", tWhole, tFrac);
	APPEND_LINE(L"--------------------------------------------------\n");
	
	const char* filePath = (lpszCmdLine && lpszCmdLine[0] != '\0') ? lpszCmdLine : "HardwareReport.txt";
	FILE* fp = NULL;
	if (fopen_s(&fp, filePath, "w, ccs=UTF-16LE") == 0) {
		fputws(buffer, fp);
		fclose(fp);
	}
	
	free(buffer);
}



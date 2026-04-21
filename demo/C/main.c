#include <windows.h>
#include <stdio.h>
#include "../../CHWEngine.h"

#define PROBMONWMI
//#define PROBMON


double elapsedMilliseconds;


void DemoOutput(HW_REPORT* report){
	
	for (int i=0;i<50;i++) putwchar(L'-');
	wprintf(L"\r\n");
	wprintf(L"CHWEngine SDK - C Demo\r\n");
	for (int i=0;i<50;i++) putwchar(L'-');
	wprintf(L"\r\n");
	
	// Motherboard info
	wprintf(L"\r\n[Motherboard]\r\n");
	for (int i = 0; i < report->BoardCount; i++){
		const BOARD_INFO* board = &report->Boards[i];
		wprintf(L"Make: %ls\r\n",board->Manufacturer);
		wprintf(L"System name: %ls\r\n",board->SystemName);
		wprintf(L"Model: %ls\r\n",board->Model);
		wprintf(L"Chipset: %ls\r\n",board->ChipsetName[0] ? board->ChipsetName : L"Generic / Undetected");
		//wprintf(L"Chipset ID: %ls\r\n",board->ChipsetId);
		wprintf(L"BIOS version: %ls\r\n",board->BiosVersion);
		
	}
	
	// CPU info
	wprintf(L"\r\n[CPU] Count: %d\r\n",report->CpuCount);
	for (int i=0;i<report->CpuCount;i++){
		const CPU_INFO* cpu = &report->Cpus[i];
		wprintf(L"Socket #%d: %ls | %dC%dT\r\n",i,cpu->Model,cpu->CoreCount,cpu->ThreadCount);
	}
	
	// Memory info
	wprintf(L"\r\n[RAM] Count: %d\r\n",report->RamCount);
	for (int i=0;i<report->RamCount;i++){
		const RAM_INFO* ram = &report->Rams[i];
		wprintf(L"Slot #%d: %ls | %ls | %u MB | %u MT/s\r\n",i,ram->Manufacturer,ram->Type[0] ? ram->Type : L"Generic / Undetected",ram->CapacityMb,ram->SpeedMts);
	}
	
	// GPU info
	wprintf(L"\r\n[GPU] Count: %d\r\n",report->GpuCount);
	for (int i = 0; i<report->GpuCount; i++){
		const GPU_INFO* gpu = &report->Gpus[i];
		double totalBytes = (double)gpu->VRamSizeByte;
		if (totalBytes >= 1073741824.0){
			wprintf(L"GPU #%d: %ls (%.1fGB / %ls)\r\n",i,gpu->Model,totalBytes / 1073741824.0,gpu->SubVendor[0] ? gpu->SubVendor : gpu->SubVenId);
		} else {
			wprintf(L"GPU #%d: %ls (%.0fMB / %ls)\r\n",i,gpu->Model,totalBytes / 1048576.0,gpu->SubVendor[0] ? gpu->SubVendor : gpu->SubVenId);
		}
	}
	
	// Monitor info
	wprintf(L"\r\n[Monitor] Count: %d\r\n",report->MonitorCount);
	for (int i=0;i<report->MonitorCount;i++){
		const MONITOR_INFO* mon = &report->Monitors[i];
		int dWhole;
		int dFrac;
		if (mon->Diagonal > 0.1f){
			dWhole = (int)mon->Diagonal;
			dFrac = (int)((mon->Diagonal - (float)dWhole) * 10.0f);
		}
		wprintf(L"Monitor #%d: %ls %ls [%ls%ls] (%d) | %d x %d | %d.%d Inch\r\n",i,mon->VendorName[0] ? mon->VendorName : L"Unknown Vendor",mon->Model[0] ? mon->Model : L"Unknown Model",mon->VendorId,mon->ProductId,mon->Year,mon->CurWidth,mon->CurHeight,dWhole,dFrac);
		
	}
	
	// Storage info
	wprintf(L"\r\n[Storage] Count: %d\r\n",report->DiskCount);
	for (int i=0;i < report->DiskCount; i++){
		const DISK_INFO* disk = &report->Disks[i];
		unsigned int sizeGB = (unsigned int)(disk->TotalSizeByte / (1024 * 1024 * 1024));
		wprintf(L"Disk #%d: %ls | %u GB\r\n",i,disk->Model,sizeGB);
		
	}
	
	// NIC info
	wprintf(L"\r\n[NIC] Count: %d\r\n",report->NicCount);
	for (int i=0;i<report->NicCount;i++){
		const NIC_INFO* nic = &report->Nics[i];
		wprintf(L"NIC #%d: %ls\r\n",i,nic->Model);
		
	}
	
	// Audio info
	wprintf(L"\r\n[Audio] Count: %d\r\n",report->AudioCount);
	for (int i=0;i<report->AudioCount;i++){
		const AUDIO_INFO* audio = &report->Audios[i];
		wprintf(L"Audio #%d: %ls\r\n",i,audio->Model);
	}
	
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HW_REPORT report = {0};
	
	/* Time count*/
	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;
	
	// 1. Initialize Frequency
	QueryPerformanceFrequency(&frequency);
	
	// 2. Start Timer
	QueryPerformanceCounter(&start);
	
	// Call
	ProbeCpus(&report);
	ProbeGpus(&report);
	ProbeBoardsAndRams(&report);
	ProbeNics(&report);
	ProbeDisks(&report);

#ifdef PROBMONWMI
	ProbeMonitorsWmi(&report);
#endif
#ifdef PROBMON
	ProbeMonitors(&report);
#endif
	ProbeAudios(&report);
	
	// 3. End Timer
	QueryPerformanceCounter(&end);
	
	// 4. Calculate Duration
	elapsedMilliseconds = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
	
	DemoOutput(&report);
	
	WCHAR timeBuf[128];
	int tWhole = (int)elapsedMilliseconds;
	int tFrac = (int)((elapsedMilliseconds - (double)tWhole) * 100.0);
	
	wprintf(L"\r\n");
	for (int i=0;i<50;i++) putwchar(L'-');
	wprintf(L"\r\n");
	wprintf(L"Detection completed in: %d.%02d ms\r\n", tWhole, tFrac);
	for (int i=0;i<50;i++) putwchar(L'-');
	wprintf(L"\r\n");
	
	getchar();
	
	return 0;
}


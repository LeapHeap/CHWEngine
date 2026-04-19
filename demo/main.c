#include <windows.h>
#include "../CHWEngine.h"

//#define PROBMONWMI
#define PROBMON


double elapsedMilliseconds;

void SaveReportToFile(HW_REPORT* report, LPCWSTR fileName) {
	
	HANDLE hFile = CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	
	
	WCHAR buf[4096]; 
	int len = 0;
	
	// Write BOM header
	WORD bom = 0xFEFF;
	DWORD written;
	WriteFile(hFile, &bom, sizeof(bom), &written, NULL);
	
	// Board info
	for (int i=0; i< report->BoardCount; i++){
		BOARD_INFO* board = &report->Boards[i];
		len = wsprintfW(buf, L"--- CHWEngine Hardware Report ---\r\n\r\n"
						L"[Motherboard]\r\n"
						L"Manufacturer: %s\r\n"
						L"SystemName: %s\r\n"
						L"Model: %s\r\n"
						L"ChipsetName: %s\r\n"
						L"ChipsetID: %s\r\n"
						L"BIOS: %s\r\n\r\n",
						board->Manufacturer, board->SystemName, board->Model, 
						board->ChipsetName, board->ChipsetId, board->BiosVersion);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	}
	
	
	// CPU info
	len = wsprintfW(buf, L"[Processor Information] Count: %d\r\n", report->CpuCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	for (int i = 0; i < report->CpuCount; i++) {
		CPU_INFO* cpu = &report->Cpus[i];
		
		// Header for each socket
		len = wsprintfW(buf, L"Socket %d: %s", i + 1, cpu->Model);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		
		// Detail line: Cores and Threads
		len = wsprintfW(buf, L" | %d Physical Cores, %d Threads\r\n", 
						cpu->CoreCount, 
						cpu->ThreadCount);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	}
	WriteFile(hFile, L"\r\n", 2 * sizeof(WCHAR), &written, NULL);
	
	
	// Memory info
	len = wsprintfW(buf, L"[Memory] Count: %d\r\n", report->RamCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	for (int i = 0; i < report->RamCount; i++) {
		len = wsprintfW(buf, L"Slot %d: %s | %s | %u MB | %u MT/s\r\n", 
						i+1, report->Rams[i].Manufacturer, report->Rams[i].Type, 
						report->Rams[i].CapacityMb, report->Rams[i].SpeedMts);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	}
	
	// Gfx info
	len = wsprintfW(buf, L"\r\n[GPU] Count: %d\r\n", report->GpuCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	for (int i = 0; i < report->GpuCount; i++) {
		len = wsprintfW(buf, L"GPU %d: %s\r\n"
						L"  VEN: %04X, DEV: %04X\r\n"
						L"SubVendor: %s\r\n"
						L"  SubVendorID: %04X, SubDeviceID: %04X\r\n",
						i + 1, 
						report->Gpus[i].Model, 
						report->Gpus[i].VenId, 
						report->Gpus[i].DevId,
						report->Gpus[i].SubVendor,
						report->Gpus[i].SubVenId, 
						report->Gpus[i].SubDevId  
						);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		if (report->Gpus[i].VRamSizeBytes > 0) {
			// Standard conversion: 1024 * 1024 * 1024 = 1073741824
			unsigned int vramGB = (unsigned int)(report->Gpus[i].VRamSizeBytes / 1073741824);
			
			if (vramGB >= 1) {
				len = wsprintfW(buf, L"  VRAM: %u GB\r\n", vramGB);
			} else {
				unsigned int vramMB = (unsigned int)(report->Gpus[i].VRamSizeBytes / (1024 * 1024));
				len = wsprintfW(buf, L"  VRAM: %u MB\r\n", vramMB);
			}
			WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		}
	}
	
	// Monitor info
	// --- Inside your report generation function ---
	
	len = wsprintfW(buf, L"\r\n[Monitors] Count: %d\r\n", report->MonitorCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	for (int i = 0; i < report->MonitorCount; i++) {
		MONITOR_INFO* m = &report->Monitors[i];
		
		
		// Float to string conversion
		int dWhole = (int)m->Diagonal;
		int dFrac = (int)((m->Diagonal - (float)dWhole) * 10.0f);
		if (dFrac < 0) dFrac = 0;
		
		// Line 1: Combine VendorID and ProductID for the tag
		// Example: Philips 27M2N5810 [PHLC32C] (2024)
		len = wsprintfW(buf, L"Monitor %d: %s %s [%s%s] (%d)\r\n", 
						i + 1, 
						m->VendorName, 
						m->MonitorName[0] ? m->MonitorName : L"Generic", 
						m->VendorId,    // "PHL"
						m->ProductId,   // "C32C"
						m->Year);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		
		// Line 2: Show Resolution (Fallback to CurWidth if PhysWidth is 0)
		if (m->PhysWidth > 0) {
			len = wsprintfW(buf, L"  Resolution: %d x %d (Native: %d x %d)\r\n"
							L"  Physical:   %d.%d Inch\r\n\r\n",
							m->CurWidth, m->CurHeight,
							m->PhysWidth, m->PhysHeight,
							dWhole, dFrac);
		} else {
			// Now handles RDP/Virtual env where CurWidth is known but Phys is not
			len = wsprintfW(buf, L"  Resolution: %d x %d (Virtual/Standard)\r\n", 
							m->CurWidth, m->CurHeight);
		}
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		// Show physical size
		if (m->Diagonal > 0.1f) {
			int dWhole = (int)m->Diagonal;
			int dFrac = (int)((m->Diagonal - (float)dWhole) * 10.0f);
			len = wsprintfW(buf, L"  Physical Size: %d.%d Inch\r\n", dWhole, dFrac);
			WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		} else {
			len = wsprintfW(buf, L"  Physical Size: Unknown (Virtual/RDP)\r\n");
			WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		}
		
		
	}
	
	// Disk info
	len = wsprintfW(buf, L"\r\n[Storage] Count: %d\r\n", report->DiskCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	for (int i = 0; i < report->DiskCount; i++) {
		// Calculate GB (using 1000 or 1024 based on preference, here 1024)
		unsigned int sizeGB = (unsigned int)(report->Disks[i].TotalSizeByte / (1024 * 1024 * 1024));
		
		len = wsprintfW(buf, L"Disk %d: %s\r\n  Capacity: %u GB\r\n  S/N: %s\r\n", 
						i + 1, report->Disks[i].Model, sizeGB, report->Disks[i].SerialNumber);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	}
	
	// Nic info
	len = wsprintfW(buf, L"\r\n[Network] Count: %d\r\n", report->NicCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	for (int i = 0; i < report->NicCount; i++) {
		len = wsprintfW(buf, L"NIC %d: %s\r\n", i+1, report->Nics[i].Model);
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	}
	
	// Audio info
	len = wsprintfW(buf, L"\r\n[Audio Hardware] Count: %d\r\n", report->AudioCount);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	if (report->AudioCount == 0) {
		len = wsprintfW(buf, L"Status: No physical audio hardware detected.\r\n");
		WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	} else {
		for (int i = 0; i < report->AudioCount; i++) {
			// We use the 'Model' field we populated via SetupAPI
			len = wsprintfW(buf, L"Device %d: %s\r\n", 
							i + 1, 
							report->Audios[i].Model);
			WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
		}
	}
	WriteFile(hFile, L"\r\n", 2 * sizeof(WCHAR), &written, NULL);
	
	//time
	WCHAR timeBuf[128];
	int tWhole = (int)elapsedMilliseconds;
	int tFrac = (int)((elapsedMilliseconds - (double)tWhole) * 100.0); // Two decimal places
	
	len = wsprintfW(buf, L"--------------------------------------\r\n");
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	len = wsprintfW(buf, L"Detection completed in: %d.%02d ms\r\n", tWhole, tFrac);
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	len = wsprintfW(buf, L"--------------------------------------\r\n");
	WriteFile(hFile, buf, len * sizeof(WCHAR), &written, NULL);
	
	CloseHandle(hFile);
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
	
	SaveReportToFile(&report, L"Report.txt");
	
	
	MessageBoxW(NULL, L"Hardware detection complete. Report saved to Report.txt", L"CHWBox Demo", MB_OK);
	return 0;
}


#include <windows.h>
#include "CHWEngine.h"
#include "BoardProbe.h"
#include <setupapi.h>
#include <devguid.h>

#define SIG_RSMB 0x52534D42
DWORD signature = SIG_RSMB;

// Define SMBIOS struct header
typedef struct {
	BYTE Type;
	BYTE Length;
	WORD Handle;
} SMBIOS_HEADER;

static void Internal_GetSmbiosString(BYTE* pStructStart, BYTE index, WCHAR* outBuf, DWORD maxLen) {
	if (index == 0 || outBuf == NULL) {
		lstrcpyW(outBuf, L"Unknown");
		return;
	}
	BYTE length = pStructStart[1];
	BYTE* pStrPool = pStructStart + length;
	char* pCurrentStr = (char*)pStrPool;
	for (BYTE i = 1; i < index; i++) {
		pCurrentStr += strlen(pCurrentStr) + 1;
	}
	MultiByteToWideChar(CP_ACP, 0, pCurrentStr, -1, outBuf, maxLen);
}

static void Internal_MapMemoryType(BYTE typeByte, WCHAR* outBuf) {
	switch (typeByte) {
		case 18: lstrcpyW(outBuf, L"DDR"); break;
		case 19: 
		case 20: lstrcpyW(outBuf, L"DDR2"); break;
		case 24: lstrcpyW(outBuf, L"DDR3"); break;
		case 25: lstrcpyW(outBuf, L"FBD2"); break;
		case 26: lstrcpyW(outBuf, L"DDR4"); break;
		case 27: lstrcpyW(outBuf, L"LPDDR"); break;
		case 28: lstrcpyW(outBuf, L"LPDDR2"); break;
		case 29: lstrcpyW(outBuf, L"LPDDR3"); break;
		case 30: lstrcpyW(outBuf, L"LPDDR4"); break;
		case 32: lstrcpyW(outBuf, L"HBM"); break;
		case 33: lstrcpyW(outBuf, L"HBM2"); break;
		case 34: lstrcpyW(outBuf, L"DDR5"); break;
		case 35: lstrcpyW(outBuf, L"LPDDR5"); break;
		default: lstrcpyW(outBuf, L"DDRx"); break;
	}
}

static void Internal_GetPciChipsetId(WCHAR* outId, int maxLen) {
	// Scan all PCI devices
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(NULL, L"PCI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return;
	
	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	
	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
		DWORD busNum = 0;
		DWORD addr = 0;
		
		// Get PCI Bus Number
		SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_BUSNUMBER, NULL, (PBYTE)&busNum, sizeof(busNum), NULL);
		// Get Device/Function Address: (Device << 16) | Function
		SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_ADDRESS, NULL, (PBYTE)&addr, sizeof(addr), NULL);
		
		// Target: Bus 0, Device 31, Function 0 (The Intel PCH / LPC Bridge)
		if (busNum == 0 && addr == 0x001F0000) {
			WCHAR hardwareID[256] = { 0 };
			if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hardwareID, sizeof(hardwareID), NULL)) {
				
				// Double check it's Intel (8086)
				if (wcsstr(hardwareID, L"VEN_8086")) {
					WCHAR* devPtr = wcsstr(hardwareID, L"DEV_");
					if (devPtr) {
						lstrcpynW(outId, devPtr + 4, 5); // Extract the 4 hex digits (e.g., 7A84)
						SetupDiDestroyDeviceInfoList(hDevInfo);
						return;
					}
				}
			}
		}
	}
	
	// Fallback: If Bus 0, Dev 31, Func 0 is hidden/disabled, grab the Host Bridge at 0/0/0
	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
		DWORD busNum = 0, addr = 0;
		SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_BUSNUMBER, NULL, (PBYTE)&busNum, sizeof(busNum), NULL);
		SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_ADDRESS, NULL, (PBYTE)&addr, sizeof(addr), NULL);
		
		if (busNum == 0 && addr == 0x00000000) {
			WCHAR hardwareID[256] = { 0 };
			if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hardwareID, sizeof(hardwareID), NULL)) {
				WCHAR* devPtr = wcsstr(hardwareID, L"DEV_");
				if (devPtr) {
					lstrcpynW(outId, devPtr + 4, 5);
					break;
				}
			}
		}
	}
	
	SetupDiDestroyDeviceInfoList(hDevInfo);
}

void Internal_MapIdNative(const WCHAR* csvName, const WCHAR* searchId, WCHAR* outBuffer, int maxLen) {
	WCHAR csvPath[MAX_PATH];
	
	// Construct path: db\[csvName]
	// Assuming the 'db' folder is in the application root directory
	lstrcpyW(csvPath, L"db\\");
	lstrcatW(csvPath, csvName);
	
	// Default fallback
	lstrcpynW(outBuffer, L"Unknown", maxLen);
	
	// Open file using Win32 API
	HANDLE hFile = CreateFileW(csvPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	
	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize > 0 && fileSize < 1024 * 256) { // 256KB limit for safety
		HANDLE hHeap = GetProcessHeap();
		char* buffer = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, fileSize + 1);
		DWORD bytesRead;
		
		if (buffer && ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
			// Convert searchId to ANSI for comparison against CSV content
			char targetA[8];
			WideCharToMultiByte(CP_ACP, 0, searchId, -1, targetA, 8, NULL, NULL);
			
			char* ptr = buffer;
			char* end = buffer + fileSize;
			
			while (ptr < end) {
				// Check if current line starts with the ID
				BOOL match = TRUE;
				for (int i = 0; i < 4; i++) {
					if (ptr + i >= end || ptr[i] != targetA[i]) {
						match = FALSE;
						break;
					}
				}
				
				if (match && ptr[4] == ',') {
					char* valStart = ptr + 5;
					char* valEnd = valStart;
					
					// Find end of the line
					while (valEnd < end && *valEnd != '\r' && *valEnd != '\n') {
						valEnd++;
					}
					
					int valLen = (int)(valEnd - valStart);
					if (valLen > 0) {
						// Convert value from UTF-8/ANSI back to WCHAR outBuffer
						MultiByteToWideChar(CP_UTF8, 0, valStart, valLen, outBuffer, maxLen);
						outBuffer[min(valLen, maxLen - 1)] = L'\0';
					}
					break;
				}
				
				// Move to next line
				while (ptr < end && *ptr != '\n') ptr++;
				ptr++;
			}
		}
		if (buffer) HeapFree(hHeap, 0, buffer);
	}
	CloseHandle(hFile);
}

BOOL ProbeBoardAndRam(HW_REPORT* report) {
	// Get table size
	DWORD bufSize = GetSystemFirmwareTable(signature, 0, NULL, 0);
	if (bufSize == 0) return FALSE;
	
	BYTE* pBuffer = (BYTE*)HeapAlloc(GetProcessHeap(), 0, bufSize);
	// Populate with fetched data
	if (GetSystemFirmwareTable(signature, 0, pBuffer, bufSize) == 0) {
		HeapFree(GetProcessHeap(), 0, pBuffer);
		return FALSE;
	}
	
	// Skip the 8-bits windows header (Signature, Version, Length, Data...)
	DWORD tableLen = *(DWORD*)(pBuffer + 4);
	BYTE* pData = pBuffer + 8; 
	BYTE* pEnd = pData + tableLen;
	
	report->RamCount = 0;
	
	while (pData < pEnd && pData[0] != 127) { // 127 as the end of table
		BYTE type = pData[0];
		BYTE length = pData[1];
		
		// BIOS info
		if (type == 0) {
			Internal_GetSmbiosString(pData, pData[5], report->Board.BiosVersion, 32);
		}
		// System model info
		else if (type==1){
			Internal_GetSmbiosString(pData, pData[5], report->Board.SystemName, 64);
		}
		// Motherboard info
		else if (type == 2) {
			Internal_GetSmbiosString(pData, pData[4], report->Board.Manufacturer, 64);
			Internal_GetSmbiosString(pData, pData[5], report->Board.Model, 64);
			// Default value for chipset if not fetched
			lstrcpyW(report->Board.ChipsetID, L"Unknown"); 
			Internal_GetPciChipsetId(report->Board.ChipsetID, 16);
			Internal_MapIdNative(L"iChipset.csv", report->Board.ChipsetID, report->Board.ChipsetName, 128);
		}
		// Memory info
		else if (type == 17) {
			WORD rawSize = *(WORD*)(pData + 0x0C);
			// Skip if an empty slot is detected
			if (rawSize != 0 && rawSize != 0xFFFF) {
				
				RAM_INFO* ram = &report->Rams[report->RamCount];
				
				// Parse capacity
				if (rawSize & 0x8000) {
					ram->CapacityMB = (rawSize & 0x7FFF);
				} else {
					ram->CapacityMB = rawSize; 
				}
				if (rawSize == 0x7FFF && length >= 0x20) {
					ram->CapacityMB = *(DWORD*)(pData + 0x1C);
				}
				
				// Identify memory type
				BYTE mType = pData[0x12]; // 0x12 as memory type offset
				Internal_MapMemoryType(mType, ram->Type);
				// Print raw data if not matched
				if (lstrcmpW(ram->Type, L"DDRx") == 0) {
					wsprintfW(ram->Type, L"Unknown(0x%02X)", mType);
				}
				
				// Get memory speed
				WORD speed = *(WORD*)(pData + 0x20);
				if (speed == 0 || speed == 0xFFFF) speed = *(WORD*)(pData + 0x12);
				
				// Mask the flag
				ram->SpeedMts = (DWORD)(speed & 0x7FFF);
				
				// Get manufacturer
				Internal_GetSmbiosString(pData, pData[0x17], ram->Manufacturer, 64);
				
				
				report->RamCount++;
			}
			
			
		}
		
		
		// Move the pointer
		pData += length; // Skip the formatted zone, indicated by length
		// Count until the double \0s in which each SMBIOS structure ends
		while (pData + 1 < pEnd && (*(WORD*)pData != 0)) {
			pData++;
		}
		pData += 2;
	}
	
	HeapFree(GetProcessHeap(), 0, pBuffer);
	return TRUE;
}



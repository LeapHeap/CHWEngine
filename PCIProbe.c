#include <initguid.h>
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <wchar.h>
#include "CHWEngine.h"
#include "PCIProbe.h"
#include "BoardProbe.h" // For the mapper function

#include <dxgi.h>
void Internal_GetVramViaDXGI(GPU_INFO* gpu) {
	IDXGIFactory* pFactory = NULL;
	
	// 1. Create DXGI Factory using the GUID defined in dxgi.h
	if (FAILED(CreateDXGIFactory(&IID_IDXGIFactory, (void**)&pFactory))) {
		return;
	}
	
	IDXGIAdapter* pAdapter = NULL;
	UINT i = 0;
	
	// 2. Enumerate adapters via vtable (lpVtbl)
	while (pFactory->lpVtbl->EnumAdapters(pFactory, i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC desc;
		
		// 3. Get adapter description
		if (SUCCEEDED(pAdapter->lpVtbl->GetDesc(pAdapter, &desc))) {
			
			// 4. Match using PCI Vendor ID and Device ID
			if (desc.VendorId == gpu->VenID && desc.DeviceId == gpu->DevID) {
				// Update the correct member name: VRAMSizeBytes
				gpu->VRAMSizeBytes = desc.DedicatedVideoMemory;
				
				pAdapter->lpVtbl->Release(pAdapter);
				break;
			}
		}
		
		pAdapter->lpVtbl->Release(pAdapter);
		i++;
	}
	
	pFactory->lpVtbl->Release(pFactory);
}


static void Internal_ParsePciId(const WCHAR* hwId, WORD* venId, WORD* devId, WORD* subVenId, WORD* subDevId) {
	const WCHAR* p;
	if ((p = wcsstr(hwId, L"VEN_"))) *venId = (WORD)wcstoul(p + 4, NULL, 16);
	if ((p = wcsstr(hwId, L"DEV_"))) *devId = (WORD)wcstoul(p + 4, NULL, 16);
	
	if ((p = wcsstr(hwId, L"SUBSYS_"))) {
		WCHAR subStr[9] = {0};
		wcsncpy(subStr, p + 7, 8);
		
		DWORD dwFullSub = wcstoul(subStr, NULL, 16);
		
		*subVenId = (WORD)(dwFullSub & 0xFFFF); // Take low 16-bit as Subvendor ID
		*subDevId = (WORD)(dwFullSub >> 16); // Take high 16-bit as Sub-device ID and store as low 16-bit
	}
}

void Internal_IntToHexW(WORD val, WCHAR* outStr) {
	const WCHAR* hexChars = L"0123456789ABCDEF";
	outStr[0] = hexChars[(val >> 12) & 0xF];
	outStr[1] = hexChars[(val >> 8) & 0xF];
	outStr[2] = hexChars[(val >> 4) & 0xF];
	outStr[3] = hexChars[val & 0xF];
	outStr[4] = L'\0';
}

/* * Generic PCI scanner engine
* Handles memory offset calculation for different struct types in HW_REPORT
*/
static int Internal_ScanPciBus(const GUID* classGuid, void* targetArray, int maxCount, size_t structSize, int type) {
	// Get device information set for all present devices in the specified class
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(classGuid, L"PCI", NULL, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return 0;
	
	SP_DEVINFO_DATA devData;
	ZeroMemory(&devData, sizeof(SP_DEVINFO_DATA));
	devData.cbSize = sizeof(SP_DEVINFO_DATA);
	
	int foundCount = 0;
	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devData) && foundCount < maxCount; i++) {
		WCHAR hwIdList[1024] = {0};
		
		// Step 1: Get the Hardware ID for parsing VEN/DEV IDs
		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devData, SPDRP_HARDWAREID, 
											  NULL, (PBYTE)hwIdList, sizeof(hwIdList), NULL)) {
			
			// Calculate memory address for the current array element
			BYTE* currentEntry = (BYTE*)targetArray + (foundCount * structSize);
			
			// Step 2: Extract common ID and Model Name based on hardware type
			if (type == 0) { // GPU_INFO
				GPU_INFO* gpu = (GPU_INFO*)currentEntry;
				Internal_ParsePciId(hwIdList, &gpu->VenID, &gpu->DevID, &gpu->SubVenID, &gpu->SubDevID);
				WCHAR subVenStr[5];
				Internal_IntToHexW(gpu->SubVenID, subVenStr);
				// SubVenID mapper
				Internal_MapIdNative(L"Graphics.csv",subVenStr,gpu->SubVendor,128);
				SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devData, SPDRP_DEVICEDESC, 
												  NULL, (PBYTE)gpu->Model, sizeof(gpu->Model), NULL);
				gpu->VRAMSizeBytes = 0;
				Internal_GetVramViaDXGI(gpu);
			} 
			else if (type == 1) { // AUDIO_INFO
				AUDIO_INFO* audio = (AUDIO_INFO*)currentEntry;
				SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devData, SPDRP_DEVICEDESC, 
												  NULL, (PBYTE)audio->Model, sizeof(audio->Model), NULL);
			}
			else if (type == 2) { // NIC_INFO
				NIC_INFO* nic = (NIC_INFO*)currentEntry;
				SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devData, SPDRP_DEVICEDESC, 
												  NULL, (PBYTE)nic->Model, sizeof(nic->Model), NULL);
			}
			
			foundCount++;
		}
	}
	
	// Clean up handle to avoid memory leaks
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return foundCount;
}

// --- Public APIs ---

BOOL ProbeGpus(HW_REPORT* report) {
	report->GpuCount = Internal_ScanPciBus(&GUID_DEVCLASS_DISPLAY, report->Gpus, 8, sizeof(GPU_INFO), 0);
	return (report->GpuCount > 0);
}

BOOL ProbeAudios(HW_REPORT* report) {
	// Media GUID includes sound cards and some encoders
	report->AudioCount = Internal_ScanPciBus(&GUID_DEVCLASS_MEDIA, report->Audios, 8, sizeof(AUDIO_INFO), 1);
	return (report->AudioCount > 0);
}

BOOL ProbeNics(HW_REPORT* report) {
	// Net GUID for Ethernet, WiFi and Bluetooth adapters
	report->NicCount = Internal_ScanPciBus(&GUID_DEVCLASS_NET, report->Nics, 16, sizeof(NIC_INFO), 2);
	return (report->NicCount > 0);
}


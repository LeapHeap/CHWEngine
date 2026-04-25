#include <initguid.h>
#include <devguid.h>
#include <windows.h>
#include <setupapi.h>
#include <wchar.h>
#include "CHWEngine.h"
#include "PCIProbe.h"
#include "Utils.h"
#include "resource.h"

//#ifndef GUID_DEVINTERFACE_DISPLAY_ADAPTER
//DEFINE_GUID(GUID_DEVINTERFACE_DISPLAY_ADAPTER, 0x5B45201D, 0xF2F2, 0x4F3B, 0x85, 0xBB, 0x30, 0xEF, 0x1F, 0x95, 0x35, 0x99);
//#endif

#ifdef USE_DXGI
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
			
			WCHAR dxgiVenStr[5], dxgiDevStr[5];
			swprintf_s(dxgiVenStr, 5, L"%04X", desc.VendorId);
			swprintf_s(dxgiDevStr, 5, L"%04X", desc.DeviceId);
			
			// 4. Match using PCI Vendor ID and Device ID
			if (_wcsicmp(dxgiVenStr, gpu->VenId) == 0 && _wcsicmp(dxgiDevStr, gpu->DevId) == 0) {
				gpu->VRamSizeByte = desc.DedicatedVideoMemory;
				pAdapter->lpVtbl->Release(pAdapter);
				break;
			}
		}
		
		pAdapter->lpVtbl->Release(pAdapter);
		i++;
	}
	
	pFactory->lpVtbl->Release(pFactory);
}
#endif

UINT64 Internal_GetVramViaRegistry(GPU_INFO* gpu) {
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	HKEY hKey;
	UINT64 vramSize = 0;
	DWORD i = 0;
	WCHAR deviceId[MAX_PATH];
	
	GUID DisplayClassGuid = { 0x4d36e968, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };
	
	hDevInfo = SetupDiGetClassDevsW(&DisplayClassGuid, NULL, NULL, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return 0;
	
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	
	while (SetupDiEnumDeviceInfo(hDevInfo, i++, &devInfoData)) {
		if (SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfoData, deviceId, MAX_PATH, NULL)) {
			
			LPWSTR pVen = wcsstr(deviceId, L"VEN_");
			LPWSTR pDev = wcsstr(deviceId, L"DEV_");
			
			if (pVen && pDev) {
				if (_wcsnicmp(pVen + 4, gpu->VenId, 4) == 0 && 
					_wcsnicmp(pDev + 4, gpu->DevId, 4) == 0) {
					
					hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
					
					if (hKey != (HKEY)INVALID_HANDLE_VALUE) {
						DWORD dwType = 0;
						DWORD dwSize = sizeof(UINT64);
						
						if (RegQueryValueExW(hKey, L"HardwareInformation.qwMemorySize", NULL, &dwType, (LPBYTE)&vramSize, &dwSize) != ERROR_SUCCESS) {
							DWORD vram32 = 0;
							dwSize = sizeof(DWORD);
							if (RegQueryValueExW(hKey, L"HardwareInformation.MemorySize", NULL, &dwType, (LPBYTE)&vram32, &dwSize) == ERROR_SUCCESS) {
								vramSize = (UINT64)vram32;
							} else {
								dwSize = sizeof(DWORD);
								if (RegQueryValueExW(hKey, L"MemorySize", NULL, &dwType, (LPBYTE)&vram32, &dwSize) == ERROR_SUCCESS) {
									vramSize = (UINT64)vram32;
								}
							}
						}
						RegCloseKey(hKey);
					}
					
					gpu->VRamSizeByte = vramSize;
					break; 
				}
			}
		}
	}
	
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return vramSize;
}


static void Internal_ParsePciId(LPCWSTR hwId, GPU_INFO* gpu) {
	LPCWSTR p;
	
	if ((p = wcsstr(hwId, L"VEN_"))) {
		wcsncpy(gpu->VenId, p + 4, 4);
		gpu->VenId[4] = L'\0';
	}
	
	if ((p = wcsstr(hwId, L"DEV_"))) {
		wcsncpy(gpu->DevId, p + 4, 4);
		gpu->DevId[4] = L'\0';
	}
	
	if ((p = wcsstr(hwId, L"SUBSYS_"))) {
		wcsncpy(gpu->SubDevId, p + 7, 4);
		gpu->SubDevId[4] = L'\0';
		
		wcsncpy(gpu->SubVenId, p + 11, 4);
		gpu->SubVenId[4] = L'\0';
	}
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
		
		// Get the Hardware ID for parsing VEN/DEV IDs
		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devData, SPDRP_HARDWAREID, 
											  NULL, (PBYTE)hwIdList, sizeof(hwIdList), NULL)) {
			
			// Calculate memory address for the current array element
			BYTE* currentEntry = (BYTE*)targetArray + (foundCount * structSize);
			
			// Extract common ID and Model Name based on hardware type
			if (type == 0) { // GPU_INFO
				GPU_INFO* gpu = (GPU_INFO*)currentEntry;
				Internal_ParsePciId(hwIdList,gpu);
				WCHAR subVenStr[5];
				// SubVenID mapper
				Internal_MapIdFromResource(IDR_CSV_GRAPHICS,gpu->SubVenId,gpu->SubVendor,128);
				SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devData, SPDRP_DEVICEDESC, 
												  NULL, (PBYTE)gpu->Model, sizeof(gpu->Model), NULL);
				gpu->VRamSizeByte = 0;
				Internal_GetVramViaRegistry(gpu);
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


BOOL ProbeNics(HW_REPORT* report) {
	// Net GUID for Ethernet, WiFi and Bluetooth adapters
	report->NicCount = Internal_ScanPciBus(&GUID_DEVCLASS_NET, report->Nics, 16, sizeof(NIC_INFO), 2);
	return (report->NicCount > 0);
}


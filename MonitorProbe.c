#include "MonitorProbe.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>
#include "CHWEngine.h"
#include "Utils.h"
#include "resource.h"

#ifdef USE_WMI
#define  _WIN32_DCOM
#include <wbemidl.h>
#endif

#include <setupapi.h>
#include <devguid.h>

#ifndef QDC_ONLY_ACTIVE_PATHS
#define QDC_ONLY_ACTIVE_PATHS 0x00000002
#endif

#ifdef USE_WMI
static void WmiArrayToWchar(VARIANT* v, LPWSTR dest, int maxLen) {
	long lBound, uBound;
	SafeArrayGetLBound(v->parray, 1, &lBound);
	SafeArrayGetUBound(v->parray, 1, &uBound);
	long count = uBound - lBound + 1;
	if (count > maxLen - 1) count = maxLen - 1;
	
	long actualIdx = 0;
	for (long i = 0; i < count; i++) {
		unsigned short val;
		long currentIdx = lBound + i; 
		SafeArrayGetElement(v->parray, &currentIdx, &val);
		
		if (val == 0 || val == 0x0A || val == 0x0D) break; 
		
		dest[actualIdx++] = (WCHAR)val;
	}
	
	dest[actualIdx] = L'\0';
}
#endif

static float GetMonitorSizeByInstance(LPCWSTR instanceName) {
	HKEY hKey;
	WCHAR regPath[512];
	
	// WMI InstanceName matches the Registry path under Enum
	// Path: HKLM\SYSTEM\CurrentControlSet\Enum\[InstanceName]\Device Parameters
	swprintf_s(regPath,_countof(regPath),L"SYSTEM\\CurrentControlSet\\Enum\\%s\\Device Parameters", instanceName);
	
	
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		BYTE edid[256];
		DWORD cbEdid = sizeof(edid);
		float diag = 0.0f;
		
		if (RegQueryValueExW(hKey, L"EDID", NULL, NULL, edid, &cbEdid) == ERROR_SUCCESS) {
			if (cbEdid >= 128) {
				// Offsets 21 and 22 are the physical dimensions in cm
				int cmH = edid[21];
				int cmV = edid[22];
				if (cmH > 0 && cmV > 0) {
					diag = (float)(sqrt((double)cmH * cmH + (double)cmV * cmV) / 2.54);
				}
			}
		}
		RegCloseKey(hKey);
		return diag;
	}
	return 0.0f;
}


void Internal_ResolveVendorName(LPCWSTR vendorId, LPWSTR outName, int maxLen) {
	Internal_MapIdFromResource(IDR_CSV_MONITORS, vendorId, outName, maxLen);
}

#ifdef USE_WMI
void ProbeMonitorsWmi(HW_REPORT* report) {
	HRESULT hr;
	IWbemLocator *pLoc = NULL;
	IWbemServices *pSvc = NULL;
	
	// Initialize COM instance
	CoInitializeEx(0, COINIT_MULTITHREADED);
	CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&pLoc);
	
	// Connect to ROOT\WMI
	pLoc->lpVtbl->ConnectServer(pLoc, L"ROOT\\WMI", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
	
	CoSetProxyBlanket((IUnknown*)pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
					  RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	
	// Execute query
	IEnumWbemClassObject* pEnumerator = NULL;
	pSvc->lpVtbl->ExecQuery(pSvc, L"WQL", L"SELECT * FROM WmiMonitorID",
							WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;
	int idx = 0;
	
	while (pEnumerator) {
		hr = pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn || idx >= 4) break;
		
		MONITOR_INFO* mon = &report->Monitors[idx];
		VARIANT vtProp;
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"ManufacturerName", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->VendorId, 16); // e.g. "PHL"
			VariantClear(&vtProp);
			Internal_ResolveVendorName(mon->VendorId,mon->VendorName,64);
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"ProductCodeID", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->ProductId, 16); // e.g. "C32C"
			VariantClear(&vtProp);
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"UserFriendlyName", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->Model, 128); // e.g. "27M2N5810"
			VariantClear(&vtProp);
//			if(!mon->Model[0]) lstrcpynW(mon->Model,L"Unknown Model",_countof(mon->Model));
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"YearOfManufacture", 0, &vtProp, 0, 0))) {
			mon->Year = vtProp.uiVal;
		}
		
		// Now, grab the Current Resolution for this monitor index
		// Note: This assumes WMI index matches the display adapter index
		DISPLAY_DEVICEW dd = { sizeof(dd) };
		if (EnumDisplayDevicesW(NULL, idx, &dd, 0)) {
			DEVMODEW dm = { sizeof(dm) };
			if (EnumDisplaySettingsW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
				mon->CurWidth = dm.dmPelsWidth;
				mon->CurHeight = dm.dmPelsHeight;
			}
		}
		
		VARIANT varInstance;
		VariantInit(&varInstance);
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"InstanceName", 0, &varInstance, NULL, NULL))) {
			// Note: WMI InstanceName sometimes has a suffix like "_0" 
			// we need to strip it if the Registry key doesn't have it.
			WCHAR cleanInstance[256];
			lstrcpynW(cleanInstance,varInstance.bstrVal,_countof(cleanInstance));
			
			WCHAR* pSuffix = wcsrchr(cleanInstance, L'_');
			if (pSuffix && iswdigit(pSuffix[1])) {
				*pSuffix = L'\0'; 
			}
			
			// Assign the diagonal size specifically for THIS monitor instance
			mon->Diagonal = GetMonitorSizeByInstance(cleanInstance);
			
			VariantClear(&varInstance);
		}
		
		
		
		pclsObj->lpVtbl->Release(pclsObj);
		idx++;
	}
	
	pEnumerator->lpVtbl->Release(pEnumerator);
	pSvc->lpVtbl->Release(pSvc);
	pLoc->lpVtbl->Release(pLoc);
	report->MonitorCount = idx;
}
#endif

#ifdef OLD_PROBE
void ProbeMonitors(HW_REPORT* report) {
	DISPLAY_DEVICEW ddAdapter = { sizeof(ddAdapter) };
	int idx = 0;
	
	// 1. Enumerate Display Adapters (Video Cards)
	// Note: We follow the idx logic to match your front-end array
	for (int i = 0; EnumDisplayDevicesW(NULL, i, &ddAdapter, 0) && idx < 4; i++) {
		// Only focus on adapters that are part of the desktop
		if (!(ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) 
			continue;
		
		DISPLAY_DEVICEW ddMon = { sizeof(ddMon) };
		int monIndex = 0;
		
		// 2. Enumerate Monitors attached to this Adapter
		while (EnumDisplayDevicesW(ddAdapter.DeviceName, monIndex, &ddMon, 0)) {
			if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE) {
				MONITOR_INFO* mon = &report->Monitors[idx];
				
				// --- 3. Extract VendorID from DeviceID (e.g., "MONITOR\PHL...") ---
				// Format is usually MONITOR\PNPID\Instance
				WCHAR* pIdStart = wcschr(ddMon.DeviceID, L'\\');
				if (pIdStart) {
					pIdStart++; 
					wcsncpy(mon->VendorId, pIdStart, 3);
					mon->VendorId[3] = L'\0';
					
					// Use the same internal mapping as your WMI version
					Internal_ResolveVendorName(mon->VendorId, mon->VendorName, _countof(mon->VendorName));
				}
				
				// --- 4. Get Monitor Friendly Name ---
				// WMI uses "UserFriendlyName", EnumDisplayDevices uses "DeviceString"
				lstrcpynW(mon->Model, ddMon.DeviceString, _countof(mon->Model));
				
				// --- 5. Grab Current Resolution ---
				// This replaces the EnumDisplaySettingsW call in your WMI version
				DEVMODEW dm = { sizeof(dm) };
				if (EnumDisplaySettingsW(ddAdapter.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
					mon->CurWidth = dm.dmPelsWidth;
					mon->CurHeight = dm.dmPelsHeight;
				}
				
				// --- 6. Set Default Values for HW-only fields ---
				// These fields (Year, Diagonal) require EDID/WMI, so we clear them in Fast Path
				mon->Year = 0;
				mon->Diagonal = 0.0f; 
				mon->ProductId[0] = L'\0'; 
				
				idx++;
				if (idx >= 4) break; // Array boundary check
			}
			monIndex++;
		}
	}
	
	// Assign total count to report to match front-end expectation
	report->MonitorCount = idx;
}
#endif

void ExtractVendorIdFromEdid(const BYTE* edid, WCHAR* outVendorId) {
	// 0x08-0x09 in EDID store three 5-bit char
	char id[4];
	id[0] = ((edid[0x08] & 0x7C) >> 2) + 64;
	id[1] = (((edid[0x08] & 0x03) << 3) | ((edid[0x09] & 0xE0) >> 5)) + 64;
	id[2] = (edid[0x09] & 0x1F) + 64;
	id[3] = '\0';
	
	// Convert to wchar
	swprintf_s(outVendorId, 8, L"%hs", id);
}

void ExtractModelNameFromEdid(const BYTE* edid, WCHAR* outModel, int maxLen) {
	// 4 description bits
	for (int i = 0; i < 4; i++) {
		int offset = 0x36 + (i * 18);
		if (edid[offset] == 0x00 && edid[offset + 1] == 0x00 && edid[offset + 3] == 0xFC) {
			char tempName[14];
			memcpy(tempName, &edid[offset + 5], 13);
			tempName[13] = '\0';
			
			// Clean return (0x0A)
			for(int j=0; j<13; j++) if(tempName[j] == 0x0A) tempName[j] = '\0';
			
			MultiByteToWideChar(CP_ACP, 0, tempName, -1, outModel, maxLen);
			return;
		}
	}
}

void ExtractProductIdFromEdid(const BYTE* edid, WCHAR* outProductId) {
	swprintf_s(outProductId, 16, L"%02X%02X", edid[0x0B], edid[0x0A]);
}

void ExtractPhysicalResolutionFromEdid(const BYTE* edid, int* physWidth, int* physHeight) {
	// Vibed code
	
	int offset = 0x36;
	if (edid[offset] == 0 && edid[offset + 1] == 0) return;
	
	int h_act_lo = edid[offset + 2];
	int h_blank_lo = edid[offset + 3];
	int h_mixed = edid[offset + 4];
	*physWidth = h_act_lo | ((h_mixed & 0xF0) << 4);
	
	// First try: read active pixels
	int v_act_lo = edid[offset + 5];
	int v_mixed = edid[offset + 7]; // Byte 0x3D includes hi-mask consisting height
	
	// V-Active in offset+7 (0x3D)
	
	int v_act_hi = (edid[offset + 4] & 0x0F) << 8;
	int h_raw = v_act_lo | v_act_hi;

	// Physical size in centimeters in EDID 0x15/0x16
	float w_cm = (float)edid[0x15];
	float h_cm = (float)edid[0x16];
	
	if (w_cm > 0 && h_cm > 0) {
		float ratio = w_cm / h_cm;
		float estimated_h = (float)(*physWidth) / ratio;
		
		int best_h = h_raw;
		float min_diff = 999999.0f;
		
		// Go through all possible 4-bit high combinations (0-15)
		for (int bit = 0; bit < 16; bit++) {
			int test_h = v_act_lo | (bit << 8);
			if (test_h == 0) continue;
			
			float diff = (float)fabs((float)test_h - estimated_h);
			if (diff < min_diff) {
				min_diff = diff;
				best_h = test_h;
			}
		}
		
		h_raw = best_h;
	}
	
	*physHeight = h_raw;
}

void ProbeMonitors(HW_REPORT* report) {
	UINT32 pathCount, modeCount;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS) return;
	
	DISPLAYCONFIG_PATH_INFO* paths = (DISPLAYCONFIG_PATH_INFO*)malloc(sizeof(DISPLAYCONFIG_PATH_INFO) * pathCount);
	DISPLAYCONFIG_MODE_INFO* modes = (DISPLAYCONFIG_MODE_INFO*)malloc(sizeof(DISPLAYCONFIG_MODE_INFO) * modeCount);
	if (!paths || !modes) { free(paths); free(modes); return; }
	
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths, &modeCount, modes, NULL) == ERROR_SUCCESS) {
		int actualIdx = 0;
		for (UINT32 i = 0; i < pathCount && actualIdx < 4; i++) {
			MONITOR_INFO* mon = &report->Monitors[actualIdx];
			ZeroMemory(mon,sizeof(MONITOR_INFO));
			
			DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = { 0 };
			targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			targetName.header.size = sizeof(targetName);
			targetName.header.adapterId = paths[i].targetInfo.adapterId;
			targetName.header.id = paths[i].targetInfo.id;
			
			if (DisplayConfigGetDeviceInfo(&targetName.header) == ERROR_SUCCESS) {
				if (targetName.flags.friendlyNameFromEdid && targetName.monitorFriendlyDeviceName[0] != 0) {
					lstrcpynW(mon->Model, targetName.monitorFriendlyDeviceName, _countof(mon->Model));
				} else {
					lstrcpynW(mon->Model, L"Generic Fixed Stack Display", _countof(mon->Model));
				}
				
				// Use bitwise operation to extract edidManufactureId
				mon->Year = (int)targetName.edidManufactureId; 
				swprintf_s(mon->ProductId, _countof(mon->ProductId), L"%04X", targetName.edidProductCodeId);
			}
			
			UINT32 mIdx = paths[i].targetInfo.targetModeInfoIdx;
			if (mIdx < modeCount) {
				mon->CurWidth = modes[mIdx].targetMode.targetVideoSignalInfo.activeSize.cx;
				mon->CurHeight = modes[mIdx].targetMode.targetVideoSignalInfo.activeSize.cy;
			}
			
			// Size
			DISPLAYCONFIG_TARGET_DEVICE_NAME_FLAGS flags = targetName.flags;
			DISPLAYCONFIG_DEVICE_INFO_HEADER devicePathHeader = { 0 };
			DISPLAYCONFIG_TARGET_DEVICE_NAME devicePathName = { 0 };
			devicePathName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			devicePathName.header.size = sizeof(devicePathName);
			devicePathName.header.adapterId = paths[i].targetInfo.adapterId;
			devicePathName.header.id = paths[i].targetInfo.id;
			
			if (DisplayConfigGetDeviceInfo(&devicePathName.header) == ERROR_SUCCESS) {
				HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, NULL, NULL, DIGCF_PRESENT);
				if (devInfo != INVALID_HANDLE_VALUE) {
					SP_DEVINFO_DATA devData = { sizeof(devData) };
					for (DWORD j = 0; SetupDiEnumDeviceInfo(devInfo, j, &devData); j++) {
						HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
						if (hKey != INVALID_HANDLE_VALUE) {
							BYTE edid[256];
							DWORD sz = sizeof(edid);
							if (RegQueryValueExW(hKey, L"EDID", NULL, NULL, edid, &sz) == ERROR_SUCCESS) {
								
//								WCHAR testmsg[128];
//								wsprintfW(testmsg,L"RAW EDID 3A: %02X, 3B: %02X, 38: %02X\n", edid[0x3A], edid[0x3B], edid[0x38]);
//								MessageBoxW(NULL,testmsg,L"edid",MB_OK);
								
								mon->Year = edid[0x11] + 1990;
								
								ExtractVendorIdFromEdid(edid, mon->VendorId);
								Internal_ResolveVendorName(mon->VendorId, mon->VendorName, 64);
								

								WCHAR edidModel[64] = {0};
								ExtractModelNameFromEdid(edid, edidModel, 64);
								if (edidModel[0] != 0) {
									lstrcpynW(mon->Model, edidModel, _countof(mon->Model));
								}
								
								// Diagonal
								float w = (float)edid[0x15];
								float h = (float)edid[0x16];
								if (w > 0 && h > 0) {
									mon->Diagonal = sqrtf(w * w + h * h) / 2.54f;
								}
								
								// Product ID
								ExtractProductIdFromEdid(edid, mon->ProductId);
								
								// Physical resolution
								ExtractPhysicalResolutionFromEdid(edid, &mon->PhysWidth, &mon->PhysHeight);
							}
							RegCloseKey(hKey);
						}
					}
					SetupDiDestroyDeviceInfoList(devInfo);
				}
			}
			actualIdx++;
		}
		report->MonitorCount = actualIdx;
	}
	free(paths); free(modes);
}

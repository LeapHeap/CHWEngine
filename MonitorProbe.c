#include "MonitorProbe.h"
#define  _WIN32_DCOM
#include <windows.h>
#include <wbemidl.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>
#include "CHWEngine.h"
#include "Utils.h"
#include "resource.h"

static void WmiArrayToWchar(VARIANT* v, WCHAR* dest, int maxLen) {
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

static float GetMonitorSizeByInstance(const WCHAR* instanceName) {
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


void Internal_ResolveVendorName(const WCHAR* vendorId, WCHAR* outName, int maxLen) {
	Internal_MapIdFromResource(IDR_CSV_MONITORS, vendorId, outName, maxLen);
}

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
			//lstrcpyW(mon->VendorName,GetVendorFullName(mon->VendorID));
			Internal_ResolveVendorName(mon->VendorId,mon->VendorName,64);
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"ProductCodeID", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->ProductId, 16); // e.g. "C32C"
			VariantClear(&vtProp);
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"UserFriendlyName", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->Model, 128); // e.g. "27M2N5810"
			VariantClear(&vtProp);
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

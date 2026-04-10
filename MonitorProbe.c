#include "MonitorProbe.h"
#define  _WIN32_DCOM
#include <windows.h>
#include <wbemidl.h>
#include <math.h>
#include "CHWEngine.h"

static void WmiArrayToWchar(VARIANT* v, WCHAR* dest, int maxLen) {
	long lBound, uBound;
	SafeArrayGetLBound(v->parray, 1, &lBound);
	SafeArrayGetUBound(v->parray, 1, &uBound);
	long count = uBound - lBound + 1;
	if (count > maxLen - 1) count = maxLen - 1;
	
	for (long i = 0; i < count; i++) {
		unsigned short val;
		SafeArrayGetElement(v->parray, &i, &val);
		if (val == 0) break; 
		dest[i] = (WCHAR)val;
	}
	dest[count] = L'\0';
}

static float GetMonitorSizeByInstance(const WCHAR* instanceName) {
	HKEY hKey;
	WCHAR regPath[512];
	
	// WMI InstanceName matches the Registry path under Enum
	// Path: HKLM\SYSTEM\CurrentControlSet\Enum\[InstanceName]\Device Parameters
	wsprintfW(regPath, L"SYSTEM\\CurrentControlSet\\Enum\\%s\\Device Parameters", instanceName);
	
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


typedef struct {
	const WCHAR* id;
	const WCHAR* name;
} VENDOR_MAP;

static const VENDOR_MAP g_VendorMap[] = {
	{ L"PHL", L"Philips" }, { L"SAM", L"Samsung" }, { L"GSM", L"LG" },
	{ L"DEL", L"Dell" },    { L"AOC", L"AOC" },     { L"BEN", L"BenQ" },
	{ L"ASU", L"ASUS" },    { L"MSI", L"MSI" },     { L"LEN", L"Lenovo" },
	{ L"HPN", L"HP" },      { L"ACI", L"ASUS" },    { L"SEC", L"Samsung" },
	{ L"BOE", L"BOE" },     { L"HKC", L"HKC" },     { L"GIG", L"Gigabyte" }
};

const WCHAR* GetVendorFullName(const WCHAR* pnpId) {
	if (!pnpId || pnpId[0] == L'\0') return L"Unknown";
	for (int i = 0; i < sizeof(g_VendorMap) / sizeof(VENDOR_MAP); i++) {
		if (wcsncmp(pnpId, g_VendorMap[i].id, 3) == 0) return g_VendorMap[i].name;
	}
	return pnpId; // Fallback to raw ID if not in map
}

void ProbeMonitorsWMI(HW_REPORT* report) {
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
			WmiArrayToWchar(&vtProp, mon->VendorID, 16); // e.g. "PHL"
			VariantClear(&vtProp);
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"ProductCodeID", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->ProductID, 16); // e.g. "C32C"
			VariantClear(&vtProp);
		}
		
		if (SUCCEEDED(pclsObj->lpVtbl->Get(pclsObj, L"UserFriendlyName", 0, &vtProp, 0, 0))) {
			WmiArrayToWchar(&vtProp, mon->MonitorName, 128); // e.g. "27M2N5810"
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
			lstrcpyW(cleanInstance, varInstance.bstrVal);
			
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

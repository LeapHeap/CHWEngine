#include "AudioProbe.h"
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include "CHWEngine.h"

void ProbeAudio(HW_REPORT* report) {
	report->AudioCount = 0;
	static const GUID MediaClassGuid = { 0x4d36e96c, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };
	
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(&MediaClassGuid, NULL, NULL, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return;
	
	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	
	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
		WCHAR enumeratorName[64] = { 0 };
		WCHAR friendlyName[128] = { 0 };
		
		// 1. Get the Enumerator Name (The "Provider" of the device node)
		SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_ENUMERATOR_NAME, NULL, (PBYTE)enumeratorName, sizeof(enumeratorName), NULL);
		
		
		// 2. Get the Display Name
		if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL)) {
			if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL)) {
				continue;
			}
		}
		
		
		// Duplicate check
		BOOL isDuplicate = FALSE;
		for (int j = 0; j < report->AudioCount; j++) {
			if (wcscmp(report->Audios[j].Model, friendlyName) == 0) {
				isDuplicate = TRUE;
				break;
			}
		}
		
		if (!isDuplicate && report->AudioCount < 8) {
			lstrcpynW(report->Audios[report->AudioCount].Model, friendlyName, _countof(report->Audios[report->AudioCount].Model));
			report->AudioCount++;
		}
	}
	
	SetupDiDestroyDeviceInfoList(hDevInfo);
}

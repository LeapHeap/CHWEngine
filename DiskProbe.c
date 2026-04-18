#include "DiskProbe.h"

#include <windows.h>


#ifdef HIPRIV
#include <winioctl.h>
#include <ntddstor.h> // For STORAGE_PROPERTY_QUERY
#endif
#include "CHWEngine.h"

#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devguid.h>

#ifdef HIPRIV
void ProbeDisks(HW_REPORT* report) {
	report->DiskCount = 0;
	WCHAR devicePath[64];
	
	for (int i = 0; i < 8 && report->DiskCount < 8; i++) {
		wsprintfW(devicePath, L"\\\\.\\PhysicalDrive%d", i);
		
		// 1. Open handle to the physical drive
		HANDLE hDisk = CreateFileW(devicePath, GENERIC_READ, 
								   FILE_SHARE_READ | FILE_SHARE_WRITE, 
								   NULL, OPEN_EXISTING, 0, NULL);
		
		if (hDisk == INVALID_HANDLE_VALUE) continue;
		
		// 2. Prepare query for device properties
		STORAGE_PROPERTY_QUERY query = { 0 };
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;
		
		BYTE buffer[1024] = { 0 };
		DWORD bytesReturned = 0;
		
		if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), 
							buffer, sizeof(buffer), &bytesReturned, NULL)) {
			
			STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
			DISK_INFO* disk = &report->Disks[report->DiskCount];
			
			// 3. Extract Model (Product ID)
			if (desc->ProductIdOffset != 0) {
				char* pModel = (char*)(buffer + desc->ProductIdOffset);
				MultiByteToWideChar(CP_ACP, 0, pModel, -1, disk->Model, 128);
				
				// Trim trailing spaces from firmware string
				int len = lstrlenW(disk->Model);
				while (len > 0 && disk->Model[len - 1] == L' ') disk->Model[--len] = L'\0';
			}
			
			// 4. Extract Serial Number
			if (desc->SerialNumberOffset != 0) {
				char* pSN = (char*)(buffer + desc->SerialNumberOffset);
				MultiByteToWideChar(CP_ACP, 0, pSN, -1, disk->SerialNumber, 64);
				
				// Trim spaces
				int len = lstrlenW(disk->SerialNumber);
				while (len > 0 && disk->SerialNumber[len - 1] == L' ') disk->SerialNumber[--len] = L'\0';
			}
			
			// 5. Get Capacity using GET_LENGTH_INFORMATION
			GET_LENGTH_INFORMATION gli = { 0 };
			if (DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, 
								&gli, sizeof(gli), &bytesReturned, NULL)) {
				disk->TotalSizeByte = gli.Length.QuadPart;
			}
			
			report->DiskCount++;
		}
		CloseHandle(hDisk);
	}
}
#endif 

void ProbeDisks(HW_REPORT* report) {
	report->DiskCount = 0;
	
	// 1. Get all disk devices using their interface GUID
	// This works in User Mode without Administrator privileges
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE) return;
	
	SP_DEVICE_INTERFACE_DATA interfaceData;
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	
	for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, i, &interfaceData) && report->DiskCount < 8; i++) {
		
		// 2. Get the required size for the interface detail data
		DWORD detailSize = 0;
		SetupDiGetDeviceInterfaceDetailW(hDevInfo, &interfaceData, NULL, 0, &detailSize, NULL);
		
		PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(detailSize);
		if (!detailData) continue;
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		
		// 3. Get the interface detail (the device path)
		if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &interfaceData, detailData, detailSize, NULL, NULL)) {
			
			// 4. Open the device via its interface path (NO ADMIN REQUIRED)
			HANDLE hDisk = CreateFileW(detailData->DevicePath, 0, // No specific access needed for query
									   FILE_SHARE_READ | FILE_SHARE_WRITE, 
									   NULL, OPEN_EXISTING, 0, NULL);
			
			if (hDisk != INVALID_HANDLE_VALUE) {
				DISK_INFO* disk = &report->Disks[report->DiskCount];
				STORAGE_PROPERTY_QUERY query = { 0 };
				query.PropertyId = StorageDeviceProperty;
				query.QueryType = PropertyStandardQuery;
				
				BYTE buffer[1024] = { 0 };
				DWORD bytesReturned = 0;
				
				// 5. Extract Model and properties
				if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), 
									buffer, sizeof(buffer), &bytesReturned, NULL)) {
					
					STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
					
					// Model name (ProductId)
					if (desc->ProductIdOffset != 0) {
						char* pModel = (char*)(buffer + desc->ProductIdOffset);
						MultiByteToWideChar(CP_ACP, 0, pModel, -1, disk->Model, _countof(disk->Model));
						int len = lstrlenW(disk->Model);
						while (len > 0 && disk->Model[len - 1] == L' ') disk->Model[--len] = L'\0';
					}
					
					// Serial Number (Might be empty in LowPriv, but we try)
					if (desc->SerialNumberOffset != 0) {
						char* pSN = (char*)(buffer + desc->SerialNumberOffset);
						MultiByteToWideChar(CP_ACP, 0, pSN, -1, disk->SerialNumber, _countof(disk->SerialNumber));
						int len = lstrlenW(disk->SerialNumber);
						while (len > 0 && disk->SerialNumber[len - 1] == L' ') disk->SerialNumber[--len] = L'\0';
					} else {
						lstrcpyW(disk->SerialNumber, L"Access Denied");
					}
					
					DISK_GEOMETRY_EX dgx;
					if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &dgx, sizeof(dgx), &bytesReturned, NULL)) {
						disk->TotalSizeByte = dgx.DiskSize.QuadPart;
					} else {
						// Fallback: If Geometry fails, try reading the partition table's logic (though less reliable)
						disk->TotalSizeByte = 0; 
					}
					
					report->DiskCount++;
				}
				CloseHandle(hDisk);
			}
		}
		free(detailData);
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
}

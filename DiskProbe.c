#include "DiskProbe.h"

#include <windows.h>
#include <winioctl.h>
#include <ntddstor.h> // For STORAGE_PROPERTY_QUERY
#include "CHWEngine.h"

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

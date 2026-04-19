#ifndef CHWENGINE_H
#define CHWENGINE_H

#if BUILDING_DLL
#define DLLIMPORT __declspec(dllexport)
#else
#define DLLIMPORT __declspec(dllimport)
#endif

#include <windows.h>



typedef struct {
	WCHAR Model[128];
	int   CoreCount;    // Physical Cores (P-cores + E-cores)
	int   ThreadCount;  // Logical Processors
} CPU_INFO;

typedef struct{
	WCHAR Manufacturer[64];
	WCHAR SystemName[64];
	WCHAR Model[64];
	WCHAR ChipsetName[128];
	WCHAR ChipsetId[32];
	WCHAR BiosVersion[32];
} BOARD_INFO;

typedef struct{
	WCHAR Manufacturer[64];
	WCHAR Type[16];
	DWORD CapacityMb;
	DWORD SpeedMts;
} RAM_INFO;

typedef struct {
	WCHAR Vendor[64];   
	WCHAR Model[128];    
	WCHAR SubVendor[128];
	WORD  VenId, DevId, SubVenId, SubDevId;
	UINT64 VRamSizeByte;
} GPU_INFO;

typedef struct {
	WCHAR Model[128];
	UINT64 TotalSizeByte;
	WCHAR SerialNumber[64];
} DISK_INFO;


typedef struct {
	int CurWidth;
	int CurHeight;
	
	WCHAR DeviceName[64];
	WCHAR VendorName[64];
	WCHAR Model[128];  // Friendly name, e.g. "27M2N5810"
	WCHAR VendorId[8];       // VendorID "PHL"
	WCHAR ProductId[16];
	int Year;       
	
	int PhysWidth;           
	int PhysHeight;    
	float Diagonal;   
} MONITOR_INFO;

typedef struct {
	WCHAR Model[128];
} AUDIO_INFO;

typedef struct {
	WCHAR Model[128];
} NIC_INFO;

typedef struct {
	BOARD_INFO Boards[2];
	int BoardCount;
	
	CPU_INFO Cpus[4];
	int CpuCount;
	
	RAM_INFO Rams[32];
	int RamCount;
	
	GPU_INFO Gpus[8];
	int GpuCount;
	
	DISK_INFO Disks[32];
	int DiskCount;
	
	MONITOR_INFO Monitors[8];
	int MonitorCount;
	
	AUDIO_INFO   Audios[8];    
	int          AudioCount;   
	
	NIC_INFO     Nics[16];     
	int          NicCount;     
} HW_REPORT;

#include "AudioProbe.h"
#include "BoardProbe.h"
#include "CpuProbe.h"
#include "DiskProbe.h"
#include "MonitorProbe.h"
#include "PCIProbe.h"


#define HW_STR_FIELDS \
X(Cpu,Model,Model) \
X(Board,Make,Manufacturer) \
X(Board,SystemName,SystemName) \
X(Board,Model,Model) \
X(Board,ChipsetName,ChipsetName) \
X(Board,BiosVersion,BiosVersion) \
X(Ram,Make,Manufacturer) \
X(Ram,Type,Type) \
X(Gpu,Vendor,Vendor) \
X(Gpu,Model,Model) \
X(Gpu,SubVendor,SubVendor) \
X(Disk,Model,Model) \
X(Disk,SerialNumber,SerialNumber) \
X(Audio,Model,Model) \
X(Nic,Model,Model) \

#define HW_COUNT_FIELDS \
X(Cpu) \
X(Board) \
X(Ram) \
X(Gpu) \
X(Disk) \
X(Audio) \
X(Nic) \
X(Monitor) \

#define HW_VAL_FIELDS \
X(Ram,CapacityMb,CapacityMb,DWORD) \
X(Ram,SpeedMts,SpeedMts,DWORD) \
X(Gpu,VRamSizeByte,VRamSizeByte,UINT64) \
X(Disk,TotalSizeByte,TotalSizeByte,UINT64) \

#define MONITOR_STR_FIELDS \
X(VendorName,VendorName,) \
X(VendorName,VendorName,Wmi) \
X(Model,Model,) \
X(Model,Model,Wmi) \
X(VendorId,VendorId,) \
X(VendorId,VendorId,Wmi) \
X(ProductId,ProductId,Wmi) \


#define MONITOR_VAL_FIELDS \
X(CurWidth,CurWidth,int, ) \
X(CurWidth,CurWidth,int,Wmi) \
X(CurHeight,CurHeight,int, ) \
X(CurHeight,CurHeight,int,Wmi) \
X(Diagonal,Diagonal,float,Wmi) \
X(Year,Year,int,Wmi) \


#include "CHWInterface.h"

#endif

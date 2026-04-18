#include <windows.h>
#include <intrin.h>
#include <stdlib.h>
#include "CHWEngine.h"
#include "CpuProbe.h"


void ProbeCpus(HW_REPORT* report) {
	report->CpuCount = 0;
	DWORD bufferSize = 0;
	
	GetLogicalProcessorInformationEx(RelationAll, NULL, &bufferSize);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;
	
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(bufferSize);
	if (!buffer) return;
	
	if (GetLogicalProcessorInformationEx(RelationAll, buffer, &bufferSize)) {
		unsigned char* ptr = (unsigned char*)buffer;
		
		// --- Pass 1: Identify Sockets (Packages) ---
		while (ptr < (unsigned char*)buffer + bufferSize && report->CpuCount < 4) {
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX item = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
			
			if (item->Relationship == RelationProcessorPackage) {
				CPU_INFO* cpu = &report->Cpus[report->CpuCount];
				cpu->CoreCount = 0; // Reset for counting
				
				// 1. Get Brand String via CPUID 
				// Note: For multi-socket, we should technically set thread affinity 
				// to a core in THIS package's mask before calling CPUID.
				GROUP_AFFINITY oldAffinity;
				if (SetThreadGroupAffinity(GetCurrentThread(), &item->Processor.GroupMask[0], &oldAffinity)) {
					int cpuInfo[4];
					char brand[49] = { 0 };
					__cpuid(cpuInfo, 0x80000000);
					if ((unsigned int)cpuInfo[0] >= 0x80000004) {
						__cpuid((int*)(brand + 0),  0x80000002);
						__cpuid((int*)(brand + 16), 0x80000003);
						__cpuid((int*)(brand + 32), 0x80000004);
						MultiByteToWideChar(CP_ACP, 0, brand, -1, cpu->Model, _countof(cpu->Model));
					}
					SetThreadGroupAffinity(GetCurrentThread(), &oldAffinity, NULL);
				}
				
				// 2. Count Logical Threads for this specific package
				for (int g = 0; g < item->Processor.GroupCount; g++) {
					KAFFINITY mask = item->Processor.GroupMask[g].Mask;
					while (mask) { if (mask & 1) cpu->ThreadCount++; mask >>= 1; }
				}
				
				// 3. Count Physical Cores belonging to this package
				unsigned char* corePtr = (unsigned char*)buffer;
				while (corePtr < (unsigned char*)buffer + bufferSize) {
					PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX coreItem = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)corePtr;
					if (coreItem->Relationship == RelationProcessorCore) {
						// Check if this core's mask is contained within the package's mask
						// (Simplified for single-group systems; usually sufficient)
						if (coreItem->Processor.GroupMask[0].Mask & item->Processor.GroupMask[0].Mask) {
							cpu->CoreCount++;
						}
					}
					corePtr += coreItem->Size;
				}
				report->CpuCount++;
			}
			ptr += item->Size;
		}
	}
	free(buffer);
}

#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include "../CHWEngine.h"

int main(){
	
	double elapsedMilliseconds;
	/* Time count*/
	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;
	
	// 1. Initialize Frequency
	QueryPerformanceFrequency(&frequency);
	
	// 2. Start Timer
	QueryPerformanceCounter(&start);
	
	
	
	
	/* Getting starts */
	
	// Get CPU
	int CpuCount = 0;
	WCHAR CpuModel[64];
	CpuCount = GetCpuCount();
	
	wprintf(L"CPU Count:%d\r\n",CpuCount);
	for (int i=0; i < CpuCount; i++){
		GetCpuModel(i,CpuModel,64);
		wprintf(L"CPU %d: %ls | %d Cores, %d Threads\r\n",i,CpuModel,GetCpuCoreCount(i),GetCpuThreadCount(i));
	}
	
	
	
	
	/* Getting ends */
	
	
	
	
	// 3. End Timer
	QueryPerformanceCounter(&end);
	
	// 4. Calculate Duration
	elapsedMilliseconds = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
	
	WCHAR timeBuf[128];
	int tWhole = (int)elapsedMilliseconds;
	int tFrac = (int)((elapsedMilliseconds - (double)tWhole) * 100.0); // Two decimal places
	wprintf(L"Detection completed in: %d.%02d ms\r\n", tWhole, tFrac);

	getchar();
	return 0;
}

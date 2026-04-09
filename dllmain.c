/* Replace "dll.h" with the name of your header */
#include "CHWEngine.h"
#include <windows.h>

#include "AudioProbe.h"
#include "BoardProbe.h"
#include "CpuProbe.h"
#include "DiskProbe.h"
#include "MonitorProbe.h"
#include "PCIProbe.h"


BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
		case DLL_THREAD_ATTACH:
		{
			break;
		}
		case DLL_THREAD_DETACH:
		{
			break;
		}
	}
	
	/* Return TRUE on success, FALSE on failure */
	return TRUE;
}

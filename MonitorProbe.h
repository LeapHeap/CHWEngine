#ifndef MONITORPROBE_H
#define MONITORPROBE_H

#include <windows.h>
#include "CHWEngine.h"

DLLIMPORT const WCHAR* GetVendorFullName(const WCHAR* pnpId);
DLLIMPORT void ProbeMonitorsWMI(HW_REPORT* report);

#endif

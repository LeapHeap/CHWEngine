#ifndef MONITORPROBE_H
#define MONITORPROBE_H

#include <windows.h>
#include "CHWEngine.h"

#ifdef USE_WMI
DLLIMPORT void ProbeMonitorsWmi(HW_REPORT* report);
#endif

DLLIMPORT void ProbeMonitors(HW_REPORT* report);

#endif

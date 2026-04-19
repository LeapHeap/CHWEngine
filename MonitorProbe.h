#ifndef MONITORPROBE_H
#define MONITORPROBE_H

#include <windows.h>
#include "CHWEngine.h"

DLLIMPORT void ProbeMonitorsWmi(HW_REPORT* report);
DLLIMPORT void ProbeMonitors(HW_REPORT* report);

#endif

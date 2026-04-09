#ifndef DISKPROBE_H
#define DISKPROBE_H

#include <windows.h>
#include <winioctl.h>
#include <ntddstor.h> // For STORAGE_PROPERTY_QUERY
#include "CHWEngine.h"

DLLIMPORT void ProbeDisks(HW_REPORT* report);

#endif

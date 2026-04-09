#ifndef PCIPROBE_H
#define PCIPROBE_H

#include "CHWEngine.h"

DLLIMPORT BOOL ProbeGpus(HW_REPORT* report);
DLLIMPORT BOOL ProbeAudios(HW_REPORT* report);
DLLIMPORT BOOL ProbeNics(HW_REPORT* report);


#endif

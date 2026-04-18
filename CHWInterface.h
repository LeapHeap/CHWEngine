#ifndef CHWINTERFACE_H
#define CHWINTERFACE_H

#include "CHWEngine.h"


#define DECLARE_HW_STR_GETTER(hwName, name) \
DLLIMPORT void Get##hwName##name(int index, LPWSTR buffer, int maxLen);

#define DECLARE_HW_COUNT_GETTER(hwName) \
DLLIMPORT int Get##hwName##Count();

#define X(hw,name,mem) DECLARE_HW_STR_GETTER(hw,name)
HW_STR_FIELDS
#undef X

#define X(hw) DECLARE_HW_COUNT_GETTER(hw)
HW_COUNT_FIELDS
#undef X


DLLIMPORT int GetCpuCoreCount(int index);
DLLIMPORT int GetCpuThreadCount(int index);




#endif

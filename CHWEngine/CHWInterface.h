#ifndef CHWINTERFACE_H
#define CHWINTERFACE_H

#include "CHWEngine.h"


#define DECLARE_HW_STR_GETTER(hwName, name) \
DLLIMPORT void Get##hwName##name(int index, LPWSTR buffer, int maxLen);

#define DECLARE_HW_COUNT_GETTER(hwName) \
DLLIMPORT int Get##hwName##Count();

#define DECLARE_HW_VAL_GETTER(hwName, name, type) \
DLLIMPORT type Get##hwName##name(int index);

#define DECLARE_MONITOR_STR_GETTER(name, suffix) \
DLLIMPORT void GetMonitor##name##suffix(int index, LPWSTR buffer, int maxLen);

#define DECLARE_MONITOR_VAL_GETTER(name, type, suffix) \
DLLIMPORT type GetMonitor##name##suffix(int index);


#define X(hw,name,mem) DECLARE_HW_STR_GETTER(hw,name)
HW_STR_FIELDS
#undef X

#define X(hw) DECLARE_HW_COUNT_GETTER(hw)
HW_COUNT_FIELDS
#undef X

#define X(hw,name,mem,type) DECLARE_HW_VAL_GETTER(hw,name,type) \
HW_VAL_FIELDS
#undef X

#define X(name,mem,suffix) DECLARE_MONITOR_STR_GETTER(name,suffix)
MONITOR_STR_FIELDS
#undef X

#define X(name,mem,type,suffix) DECLARE_MONITOR_VAL_GETTER(name,type,suffix) \
MONITOR_VAL_FIELDS
#undef X

DLLIMPORT int GetCpuCoreCount(int index);
DLLIMPORT int GetCpuThreadCount(int index);

DLLIMPORT void CALLBACK ExportReportToFile(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);




#endif

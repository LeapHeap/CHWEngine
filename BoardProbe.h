#ifndef BOARDPROBE_H
#define BOARDPROBE_H

#include <windows.h>
#include "CHWEngine.h"

DLLIMPORT BOOL ProbeBoardAndRam(HW_REPORT* report);
void Internal_MapIdNative(const WCHAR* csvName, const WCHAR* searchId, WCHAR* outBuffer, int maxLen);

#endif

#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
void Internal_MapIdFromResource(int resId, const WCHAR* searchId, WCHAR* outBuffer, int maxLen);
void Internal_IntToHexW(WORD val, WCHAR* outStr);

#endif

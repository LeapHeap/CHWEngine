#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
void Internal_MapIdNative(const WCHAR* csvName, const WCHAR* searchId, WCHAR* outBuffer, int maxLen);
void Internal_IntToHexW(WORD val, WCHAR* outStr);

#endif

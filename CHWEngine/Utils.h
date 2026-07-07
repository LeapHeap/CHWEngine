#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
void Internal_MapIdFromResource(int resId, LPCWSTR searchId, LPWSTR outBuffer, int maxLen);
void Internal_IntToHexW(WORD val, LPWSTR outStr);

#endif

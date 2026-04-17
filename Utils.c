#include <windows.h>
#include "Utils.h"
#include "resource.h"

//void Internal_MapIdNative(const WCHAR* csvName, const WCHAR* searchId, WCHAR* outBuffer, int maxLen) {
//	WCHAR csvPath[MAX_PATH];
//	
//	// Construct path: db\[csvName]
//	// Assuming the 'db' folder is in the application root directory
//	lstrcpyW(csvPath, L"db\\");
//	lstrcatW(csvPath, csvName);
//	
//	// Default fallback
//	lstrcpynW(outBuffer, L"Unknown", maxLen);
//	
//	// Open file using Win32 API
//	HANDLE hFile = CreateFileW(csvPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//	if (hFile == INVALID_HANDLE_VALUE) return;
//	
//	DWORD fileSize = GetFileSize(hFile, NULL);
//	if (fileSize > 0 && fileSize < 1024 * 256) { // 256KB limit for safety
//		HANDLE hHeap = GetProcessHeap();
//		char* buffer = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, fileSize + 1);
//		DWORD bytesRead;
//		
//		if (buffer && ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
//			// Convert searchId to ANSI for comparison against CSV content
//			char targetA[8];
//			WideCharToMultiByte(CP_ACP, 0, searchId, -1, targetA, 8, NULL, NULL);
//			
//			char* ptr = buffer;
//			char* end = buffer + fileSize;
//			
//			while (ptr < end) {
//				// Check if current line starts with the ID
//				BOOL match = TRUE;
//				for (int i = 0; i < 4; i++) {
//					if (ptr + i >= end || ptr[i] != targetA[i]) {
//						match = FALSE;
//						break;
//					}
//				}
//				
//				if (match && ptr[4] == ',') {
//					char* valStart = ptr + 5;
//					char* valEnd = valStart;
//					
//					// Find end of the line
//					while (valEnd < end && *valEnd != '\r' && *valEnd != '\n') {
//						valEnd++;
//					}
//					
//					int valLen = (int)(valEnd - valStart);
//					if (valLen > 0) {
//						// Convert value from UTF-8/ANSI back to WCHAR outBuffer
//						MultiByteToWideChar(CP_UTF8, 0, valStart, valLen, outBuffer, maxLen);
//						outBuffer[min(valLen, maxLen - 1)] = L'\0';
//					}
//					break;
//				}
//				
//				// Move to next line
//				while (ptr < end && *ptr != '\n') ptr++;
//				ptr++;
//			}
//		}
//		if (buffer) HeapFree(hHeap, 0, buffer);
//	}
//	CloseHandle(hFile);
//}

void Internal_MapIdFromResource(int resId, const WCHAR* searchId, WCHAR* outBuffer, int maxLen) {
	HMODULE hMod = GetModuleHandleW(L"CHWEngine.dll");
	
	// Default fallback if not found
	lstrcpynW(outBuffer, L"Unknown", maxLen);
	
	// 1. Locate the resource in the DLL's data segment
	HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resId), (LPCWSTR)RT_RCDATA);
	if (!hRes) return;
	
	DWORD size = SizeofResource(hMod, hRes);
	HGLOBAL hData = LoadResource(hMod, hRes);
	const char* pBase = (const char*)LockResource(hData);
	if (!pBase || size == 0) return;
	
	// 2. Convert searchId (Wide) to ANSI for memory comparison
	char targetA[16];
	WideCharToMultiByte(CP_ACP, 0, searchId, -1, targetA, sizeof(targetA), NULL, NULL);
	int targetLen = lstrlenA(targetA);
	
	// 3. Scan memory buffer
	const char* pPtr = pBase;
	const char* pEnd = pBase + size;
	
	while (pPtr < pEnd) {
		// Find the end of the current line
		const char* lineEnd = pPtr;
		while (lineEnd < pEnd && *lineEnd != '\r' && *lineEnd != '\n') {
			lineEnd++;
		}
		
		int lineLen = (int)(lineEnd - pPtr);
		
		// Check if line starts with targetA followed by a comma
		// CSV Format: 7A84,Intel Z690
		if (lineLen > targetLen && pPtr[targetLen] == ',') {
			BOOL match = TRUE;
			for (int i = 0; i < targetLen; i++) {
				if (pPtr[i] != targetA[i]) {
					match = FALSE;
					break;
				}
			}
			
			if (match) {
				const char* valStart = pPtr + targetLen + 1;
				int valLen = (int)(lineEnd - valStart);
				
				if (valLen > 0) {
					// Convert UTF-8 CSV content to Wide String for the GUI/Report
					int converted = MultiByteToWideChar(CP_UTF8, 0, valStart, valLen, outBuffer, maxLen - 1);
					outBuffer[converted] = L'\0';
				}
				return; // Match found, exit function
			}
		}
		
		// Move to the start of the next line
		pPtr = lineEnd;
		while (pPtr < pEnd && (*pPtr == '\r' || *pPtr == '\n')) {
			pPtr++;
		}
	}
}

void Internal_IntToHexW(WORD val, WCHAR* outStr) {
	const WCHAR* hexChars = L"0123456789ABCDEF";
	outStr[0] = hexChars[(val >> 12) & 0xF];
	outStr[1] = hexChars[(val >> 8) & 0xF];
	outStr[2] = hexChars[(val >> 4) & 0xF];
	outStr[3] = hexChars[val & 0xF];
	outStr[4] = L'\0';
}

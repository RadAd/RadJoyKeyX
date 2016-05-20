#pragma once

DWORD GetModuleFileNameEx2(DWORD pid, LPWSTR lpFileName, DWORD nSize);
HMODULE FindModule(DWORD pid, LPCWSTR lpModuleSpec);
bool RegGetString(HKEY hKey, LPCWSTR lpValue, LPWSTR str, DWORD nLength, LPCWSTR def);
void DebugOut(const TCHAR* format, ...);

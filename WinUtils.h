#pragma once

DWORD GetModuleFileNameEx2(DWORD pid, LPWSTR lpFileName, DWORD nSize);
HMODULE FindModule(DWORD pid, LPCWSTR lpModuleSpec);
void DebugOut(const TCHAR* format, ...);

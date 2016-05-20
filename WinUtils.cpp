#include <stdafx.h>
#include <psapi.h>
#include <Shlwapi.h>

DWORD GetModuleFileNameEx2(DWORD pid, LPWSTR lpFileName, DWORD nSize)
{
    HINSTANCE hInst = NULL;//GetWindowInstance(hWnd);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	DWORD ret = 0;
	if (hProcess != NULL)
	{
		ret = GetModuleFileNameEx(hProcess, hInst, lpFileName, nSize);
		CloseHandle(hProcess);
	}
	return ret;
}

HMODULE FindModule(DWORD pid, LPCWSTR lpModuleSpec)
{
	HMODULE hFoundModule = NULL;

	HANDLE hProcess = pid == ULONG_MAX ? GetCurrentProcess() : OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (hProcess != NULL)
    {
        HMODULE hModules[1024];
        DWORD dwNeeded = 0;
        EnumProcessModules(hProcess, hModules, sizeof(hModules), &dwNeeded);
        DWORD count = min(ARRAYSIZE(hModules), dwNeeded / sizeof(HMODULE));
        for (DWORD i = 0; i < count; ++i)
        {
            TCHAR strModule[1024] = { 0 };
            DWORD r = GetModuleFileNameEx(hProcess, hModules[i], strModule, ARRAYSIZE(strModule));
            if (r != 0 && PathMatchSpec(strModule, lpModuleSpec))
            {
                hFoundModule = hModules[i];
                break;
            }
        }

        CloseHandle(hProcess);
    }
	return hFoundModule;
}

bool RegGetString(HKEY hKey, LPCWSTR lpValue, LPWSTR str, DWORD nLength, LPCWSTR def)
{
    DWORD nSize = nLength * sizeof(TCHAR);
    if (RegGetValue(hKey, NULL, lpValue, RRF_RT_REG_SZ, NULL, str, &nSize) == ERROR_SUCCESS)
    {
        return true;
    }
    else
    {
        _tcscpy_s(str, nLength, def);
        return false;
    }

}

void DebugOut(const TCHAR* format, ...)
{
	TCHAR buffer[2 * 1024];
	va_list args;
	va_start(args, format);
	_vsnwprintf_s(buffer, ARRAYSIZE(buffer), format, args);
	OutputDebugStringW(buffer);
	va_end(args);
}

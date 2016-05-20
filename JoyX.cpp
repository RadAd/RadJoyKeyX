#include "stdafx.h"
#include "JoyX.h"
#include "Utils.h"
#include "WinUtils.h"
#include <Shlwapi.h>

#define KF_SCANKEY	   0x0400

#define MOUSE_SENSITIVITY 3
#define MOUSE_SPEED 5

inline POINTFLOAT Normalize(SHORT x, SHORT y, SHORT deadZone)
{
	return {
		Power(Normalize(x, deadZone), MOUSE_SENSITIVITY),
		Power(Normalize(y, deadZone), MOUSE_SENSITIVITY)
	};
}

bool SendKey(WORD wScan, bool bDown, bool* keyDown)
{
	INPUT ip = { INPUT_KEYBOARD };
	WORD vk = wScan & 0xFF;

    if (wScan & KF_SCANKEY)
    {
        vk = MapVirtualKey(vk, MAPVK_VSC_TO_VK);
        ip.ki.wScan = wScan;
        ip.ki.wVk = vk;
        ip.ki.dwFlags = 0;
        if (!bDown)
            ip.ki.dwFlags |= KEYEVENTF_KEYUP;
        if (wScan & KF_EXTENDED)
            ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        //ip.ki.dwFlags |= KEYEVENTF_SCANCODE;
    }
    else
	{
		switch (vk)
		{
		case VK_LBUTTON:
			ip.type = INPUT_MOUSE;
			ip.mi.dwFlags = bDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
			break;

		case VK_RBUTTON:
			ip.type = INPUT_MOUSE;
			ip.mi.dwFlags = bDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
			break;

		case VK_MBUTTON:
			ip.type = INPUT_MOUSE;
			ip.mi.dwFlags = bDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
			break;

		case VK_XBUTTON1:
			ip.type = INPUT_MOUSE;
			ip.mi.dwFlags = bDown ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
			ip.mi.mouseData = XBUTTON1;
			break;

		case VK_XBUTTON2:
			ip.type = INPUT_MOUSE;
			ip.mi.dwFlags = bDown ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
			ip.mi.mouseData = XBUTTON2;
			break;

		default:
			ip.ki.wVk = vk;
			ip.ki.wScan = MapVirtualKey(ip.ki.wVk & 0xFF, MAPVK_VK_TO_VSC);
			ip.ki.dwFlags = 0;
			if (!bDown)
				ip.ki.dwFlags |= KEYEVENTF_KEYUP;
			//ip.ki.dwFlags |= KEYEVENTF_SCANCODE;
			break;
		}
	}

	bool bSend = true;
	if (!bDown)
		bSend = keyDown[ip.ki.wVk];
	keyDown[ip.ki.wVk] = bDown;

#if 0 //_DEBUG
	if (bSend)
	{
		switch (ip.type)
		{
		case INPUT_KEYBOARD:
			DebugOut(_T("SendKey scan: %X vk: %X %s\n"), ip.ki.wScan, ip.ki.wVk, bDown ? _T("Down") : _T("Up"));
			break;
		case INPUT_MOUSE:
			DebugOut(_T("SendMouse flags: %X md: %X %s\n"), ip.mi.dwFlags, ip.mi.mouseData, bDown ? _T("Down") : _T("Up"));
			break;
		default:
			DebugOut(_T("SendUnknown\n"));
			break;
		}
	}
#endif

	if (bSend)
		SendInput(1, &ip, sizeof(INPUT));
    return bSend;
}

bool SendMouse(LONG dx, LONG dy)
{
	if (dx != 0 || dy != 0)
	{
        //DebugOut(_T("SendMouse x: %d y: %d\n"), dx, dy);
		INPUT ip = { INPUT_MOUSE };
		ip.mi.dx = dx;
		ip.mi.dy = dy;
		ip.mi.dwFlags = MOUSEEVENTF_MOVE;
		SendInput(1, &ip, sizeof(INPUT));
		return true;
	}
	else
		return false;
}

bool SendScroll(LONG dx, LONG dy)
{
	if (dx != 0 || dy != 0)
	{
		POINT pt;
		GetCursorPos(&pt);
		HWND hWnd = WindowFromPoint(pt);
		if (IsWindow(hWnd))
		{
			if (dx != 0)
			{
				//SendMessage(hWnd, WM_HSCROLL, dx > 0 ? SB_LINERIGHT : SB_LINELEFT, 0);
				SendMessage(hWnd, WM_MOUSEHWHEEL, MAKEWPARAM(0, dx), POINTTOPOINTS(pt)); // TODO Check key combos
			}
			if (dy != 0)
			{
				//SendMessage(hWnd, WM_VSCROLL, dy > 0 ? SB_LINEDOWN : SB_LINEUP, 0);
				SendMessage(hWnd, WM_MOUSEWHEEL, MAKEWPARAM(0, dy), POINTTOPOINTS(pt)); // TODO Check key combos
			}
		}
		return true;
	}
	else
		return false;
}

inline SHORT GetThumbX(const XINPUT_STATE& state, JoyThumb t)
{
    switch (t)
    {
    case JMT_LEFT:
        return state.Gamepad.sThumbLX;
    case JMT_RIGHT:
        return state.Gamepad.sThumbRX;
    default:
        return 0;
    }
}

inline SHORT GetThumbY(const XINPUT_STATE& state, JoyThumb t)
{
    switch (t)
    {
    case JMT_LEFT:
        return state.Gamepad.sThumbLY;
    case JMT_RIGHT:
        return state.Gamepad.sThumbRY;
    default:
        return 0;
    }
}

inline bool IsOnButtonDown(const XINPUT_STATE& state, const XINPUT_STATE& stateold, WORD button)
{
	return (!(stateold.Gamepad.wButtons & button)
		&& (state.Gamepad.wButtons & button));
}

inline bool IsOnButtonUp(const XINPUT_STATE& state, const XINPUT_STATE& stateold, WORD button)
{
	return ((stateold.Gamepad.wButtons & button)
		&& !(state.Gamepad.wButtons & button));
}

bool DoButtonDown(const XINPUT_STATE& state, const XINPUT_STATE& stateold, WORD button, const WORD* const keys, bool* keyDown)
{
	if (IsOnButtonDown(state, stateold, button))
	{
		int k = 0;
		while (keys[k] != 0)
		{
			SendKey(keys[k], true, keyDown);
			++k;
		}
		return true;
	}
	return false;
}

bool DoButtonUp(const XINPUT_STATE& state, const XINPUT_STATE& stateold, WORD button, const WORD* const keys, bool* keyDown)
{
	if (IsOnButtonUp(state, stateold, button))
	{
		int k = 0;
		while (keys[k] != 0)
		{
			SendKey(keys[k], false, keyDown);
			++k;
		}
		return true;
	}
	return false;
}

bool IsWnd(const WndInfo& wndInfo, const TCHAR* strModule, const TCHAR* strClass, const TCHAR* strText)
{
    bool match = true;
    match &= PathMatchSpec(wndInfo.strModule, strModule) != FALSE;
    match &= PathMatchSpec(wndInfo.strWndClass, strClass) != FALSE;
    match &= PathMatchSpec(wndInfo.strWndText, strText) != FALSE;
    return match;
}

bool Update(WndInfo& wndInfo)
{
	DWORD r;
	HWND hWndFG = GetForegroundWindow();
	if (hWndFG != NULL && hWndFG != wndInfo.hWndFG)
	{
		GetWindowThreadProcessId(hWndFG, &wndInfo.pid);

		r = GetModuleFileNameEx2(wndInfo.pid, wndInfo.strModule, ARRAYSIZE(wndInfo.strModule));
		if (r == 0)
            _tcscpy_s(wndInfo.strModule, L"[Unknown]");
		r = GetClassName(hWndFG, wndInfo.strWndClass, ARRAYSIZE(wndInfo.strWndClass));
		if (r == 0)
            _tcscpy_s(wndInfo.strWndClass, L"");
		r = GetWindowText(hWndFG, wndInfo.strWndText, ARRAYSIZE(wndInfo.strWndText));
		if (r == 0)
            _tcscpy_s(wndInfo.strWndText, L"");
		wndInfo.bUsesXinput = FindModule(wndInfo.pid, _T("*\\xinput*.dll")) != NULL;

#if 0
		TCHAR* exe = _tcsrchr(wndInfo.strModule, _T('\\'));
		exe = exe == nullptr ? wndInfo.strModule : exe + 1;
		DebugOut(_T("Module: 0x%x %s\n"), (UINT)hWndFG, exe);
		DebugOut(_T("  XInput: %s\n"), _(wndInfo.bUsesXinput));
		DebugOut(_T("  Class: %s\n"), wndInfo.strWndClass);
		DebugOut(_T("  Text: %s\n"), wndInfo.strWndText);
#endif

		wndInfo.hWndFG = hWndFG;
		return true;
	}
    else if (hWndFG == NULL)
    {
        wndInfo.pid = 0;
        _tcscpy_s(wndInfo.strModule, L"[Unknown]");
        _tcscpy_s(wndInfo.strWndClass, L"");
        _tcscpy_s(wndInfo.strWndText, L"");
        wndInfo.bUsesXinput = false;
    }
	return false;
}

bool Update(QUERY_USER_NOTIFICATION_STATE& notifyStateOld)
{
	QUERY_USER_NOTIFICATION_STATE notifyState;
	DWORD r = SHQueryUserNotificationState(&notifyState);
	if (notifyState != notifyStateOld)
	{
		//DebugOut(_T("-Notify State: %d\n"), notifyState);
		notifyStateOld = notifyState;
		return true;
	}
	return false;
}

const JoyMapping& GetWndJoyMapping(const JoyX& joyx)
{
    //DebugOut(_T("\n  Find: %s\n"), joyx.wndInfoFG.strModule);
    for (auto it = joyx.joyMappingOther.begin(); it != joyx.joyMappingOther.end(); ++it)
    {
        const JoyMapping& thisJoyMapping = it->second;
        //DebugOut(_T("    Search: %s\n"), thisJoyMapping.strModule);
        if (IsWnd(joyx.wndInfoFG, thisJoyMapping.strModule, thisJoyMapping.strWndClass, thisJoyMapping.strWndText))
        {
            //DebugOut(_T("    Found: %s\n"), thisJoyMapping.strModule);
            return thisJoyMapping;
        }
    }
    return joyx.joyMapping;
}

JoystickRet DoJoystick(JoyX& joyx)
{
    JoystickRet ret;
	//printf("time: %d %d\n", ticksleep, tick);
	DWORD r;

	bool bWindowChanged = false;
    bWindowChanged = Update(joyx.wndInfoFG) || bWindowChanged;
    bWindowChanged = Update(joyx.notifyState) || bWindowChanged;

    const JoyMapping& wndJoyMapping = GetWndJoyMapping(joyx);
    
	{
		bool bEnabled = !joyx.wndInfoFG.bUsesXinput || joyx.notifyState >= QUNS_ACCEPTS_NOTIFICATIONS || &wndJoyMapping != &joyx.joyMapping;
		// TODO Enable if Steam
		if (bEnabled != joyx.bEnabled)
		{
			//MessageBeep(bEnabled ? MB_ICONINFORMATION : MB_ICONWARNING);
			//DebugOut(_T("Enabled: %s\n"), _(bEnabled));
			//DebugOut(_T("  Uses X Input: %s\n"), _(joyx.wndInfoFG.bUsesXinput));
			//DebugOut(_T("  Notify State: %d\n"), joyx.notifyState);
			joyx.bEnabled = bEnabled;
            bWindowChanged = true;
		}
	}

	if (bWindowChanged)
	{
		TCHAR* exe = _tcsrchr(joyx.wndInfoFG.strModule, _T('\\'));
		exe = exe == nullptr ? joyx.wndInfoFG.strModule : exe + 1;
		//DebugOut(_T("Module: 0x%x %s\n"), (UINT)hWndFG, exe);
		DebugOut(_T("%c %s \"%s\"\n"), (joyx.bEnabled ? '+' : '-'), exe, joyx.wndInfoFG.strWndText);
	}

    for (int j = 0; j < XUSER_MAX_COUNT; ++j)
	{
		XINPUT_BATTERY_INFORMATION joyBattery = {};
		r = XInputGetBatteryInformation(j, BATTERY_DEVTYPE_GAMEPAD, &joyBattery);
		if (r == ERROR_SUCCESS && !Equal(joyBattery, joyx.joyBattery[j]))
		{
            joyx.joyBattery[j] = joyBattery;
            ret.bBatteryChanged = true;
		}

		XINPUT_CAPABILITIES joyCapabilities = {};
		r = XInputGetCapabilities(j, XINPUT_FLAG_GAMEPAD, &joyCapabilities);
		if (r != ERROR_SUCCESS)
		{
			continue;
		}

		XINPUT_STATE joyState = {};
		r = XInputGetState(j, &joyState);
		if (r == ERROR_SUCCESS)
		{
            const JoyMapping& joyMapping = joyState.Gamepad.wButtons & joyx.altKey ? joyx.joyMappingAlt : wndJoyMapping;

            if (joyState.dwPacketNumber != joyx.joyState[j].dwPacketNumber)
			{
#if 0
				for (int button = XINPUT_GAMEPAD_DPAD_UP; button <= XINPUT_GAMEPAD_Y; button <<= 1)
				{
					if (capabilities.Gamepad.wButtons & button)
					{
						if ((joyState.Gamepad.wButtons & button) != (joyx.joyState[j].Gamepad.wButtons & button))
						{
							DebugOut(_T("button: 0x%X\n"), button);
						}
					}
				}
				SHORT* axes = &joyState.Gamepad.sThumbLX;
				SHORT* axesold = &joyx.joyState[j].Gamepad.sThumbLX;
				for (DWORD i = 0; i < 4; ++i)
				{
					if (axes[j] != axesold[j])
					{
						DebugOut(_T("axis: %d %d\n"), i, axes[j]);
					}
				}
#endif
				for (int b = 0; b < XINPUT_MAX_BUTTONS; ++b)
				{
					const JoyMappingButton& joyMappingButton = joyMapping.joyMappingButton[b];
					const WORD mask = 1 << b;
					switch (joyMappingButton.type)
					{
					case JMBT_KEYS:
						if (joyx.bEnabled && (joyCapabilities.Gamepad.wButtons & mask))
							if (DoButtonDown(joyState, joyx.joyState[j], mask, joyMappingButton.keys, joyx.keyDown))
								joyx.joyLast = JML_KEYBOARD;
						DoButtonUp(joyState, joyx.joyState[j], mask, joyMappingButton.keys, joyx.keyDown);
						break;

					case JMBT_COMMAND:
                        if (joyx.bEnabled && IsOnButtonDown(joyState, joyx.joyState[j], mask))
                        {
                            switch (joyMappingButton.command)
                            {
                            case JMC_TURN_OFF:
                                if (joyx.XInputPowerOffController != nullptr)
                                    joyx.XInputPowerOffController(j);
                                break;

                            case JMC_BUTTON:
                                if (joyx.joyLast == JML_MOUSE)
                                {
                                    SendKey(VK_LBUTTON, true, joyx.keyDown);
                                }
                                else if (joyx.joyLast == JML_KEYBOARD)
                                {
                                    SendKey(VK_RETURN, true, joyx.keyDown);
                                }
                                break;
                            }
                        }

						if (IsOnButtonUp(joyState, joyx.joyState[j], mask))
						{
							switch (joyMappingButton.command)
							{
							case JMC_TURN_OFF:
								break;

							case JMC_BUTTON:
								if (joyx.joyLast == JML_MOUSE)
								{
									SendKey(VK_LBUTTON, false, joyx.keyDown);
								}
								else if (joyx.joyLast == JML_KEYBOARD)
								{
									SendKey(VK_RETURN, false, joyx.keyDown);
								}
								break;
							}
						}
					}
				}

				// TODO User override joyx.bEnabled
            }

			if (joyx.bEnabled)
			{
                for (int i = 0; i < JMT_MAX; ++i)
                {
                    JoyThumb t = static_cast<JoyThumb>(i);
                    POINTFLOAT pf = Normalize(GetThumbX(joyState, t), GetThumbY(joyState, t), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                    switch (joyMapping.joyMappingThumb[i])
                    {
                    case JMTT_MOUSE:
                        if (SendMouse((LONG) ((pf.x * MOUSE_SPEED) + 0.5), (LONG) ((pf.y * -MOUSE_SPEED) + 0.5)))
                            joyx.joyLast = JML_MOUSE;
                        break;

                    case JMTT_SCROLL:
                        if (SendScroll((LONG) ((pf.x * WHEEL_DELTA) + 0.5), (LONG) ((pf.y * WHEEL_DELTA) + 0.5)))
                            joyx.joyLast = JML_MOUSE;
                        break;

                    case JMTT_WASD:
                        {
                            bool bSend = false;
                            if (pf.x < -0.1)
                                bSend |= SendKey('A', true, joyx.keyDown);
                            else if (pf.x > 0.1)
                                bSend |= SendKey('D', true, joyx.keyDown);
                            else
                            {
                                bSend |= SendKey('A', false, joyx.keyDown);
                                bSend |= SendKey('D', false, joyx.keyDown);
                            }

                            if (pf.y < -0.1)
                                bSend |= SendKey('S', true, joyx.keyDown);
                            else if (pf.y > 0.1)
                                bSend |= SendKey('W', true, joyx.keyDown);
                            else
                            {
                                bSend |= SendKey('S', false, joyx.keyDown);
                                bSend |= SendKey('W', false, joyx.keyDown);
                            }
                            if (bSend)
                                joyx.joyLast = JML_KEYBOARD;
                        }
                        break;
                    }
                }
			}

            joyx.joyState[j] = joyState;
        }
	}

    return ret;
}

LPCWSTR GetButtonName(int b)
{
    switch (b)
    {
    case XINPUT_GAMEPAD_DPAD_UP: return TEXT("DPadUp");
    case XINPUT_GAMEPAD_DPAD_DOWN: return TEXT("DPadDown");
    case XINPUT_GAMEPAD_DPAD_LEFT: return TEXT("DPadLeft");
    case XINPUT_GAMEPAD_DPAD_RIGHT: return TEXT("DPadRight");
    case XINPUT_GAMEPAD_START: return TEXT("Start");
    case XINPUT_GAMEPAD_BACK: return TEXT("Back");
    case XINPUT_GAMEPAD_LEFT_THUMB: return TEXT("ThumbButtonLeft");
    case XINPUT_GAMEPAD_RIGHT_THUMB: return TEXT("ThumbButtonRight");
    case XINPUT_GAMEPAD_LEFT_SHOULDER: return TEXT("ShoulderLeft");
    case XINPUT_GAMEPAD_RIGHT_SHOULDER: return TEXT("ShoulderRight");
    case XINPUT_GAMEPAD_A: return TEXT("GamepadA");
    case XINPUT_GAMEPAD_B: return TEXT("GamepadB");
    case XINPUT_GAMEPAD_X: return TEXT("GamepadX");
    case XINPUT_GAMEPAD_Y: return TEXT("GamepadY");
    default: return nullptr;
    }
}

LPCWSTR GetThumbName(JoyThumb t)
{
    switch (t)
    {
    case JMT_LEFT: return TEXT("ThumbLeft");
    case JMT_RIGHT: return TEXT("ThumbRight");
    default: return nullptr;
    }
}

bool LoadRegString(HKEY hKey, LPCWSTR lpValue, LPWSTR str, DWORD nLength, LPCWSTR def)
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

bool is(LPCWSTR start, LPCWSTR end, LPCWSTR compare)
{
    size_t l = _tcsclen(compare);
    return (end - start) == l && _tcsncicmp(start, compare, l) == 0;
}

WORD GetButton(LPCWSTR start, LPCWSTR end)
{
         if (is(start, end, TEXT("LBUTTON")))   return VK_LBUTTON;
    else if (is(start, end, TEXT("RBUTTON")))   return VK_RBUTTON;
    else if (is(start, end, TEXT("MBUTTON")))   return VK_MBUTTON;
    else if (is(start, end, TEXT("RETURN")))    return VK_RETURN;
    else if (is(start, end, TEXT("SHIFT")))     return VK_SHIFT;
    else if (is(start, end, TEXT("CONTROL")))   return VK_CONTROL;
    else if (is(start, end, TEXT("ALT")))       return VK_MENU;
    else if (is(start, end, TEXT("ESCAPE")))    return VK_ESCAPE;
    else if (is(start, end, TEXT("SPACE")))     return VK_SPACE;
    else if (is(start, end, TEXT("END")))       return VK_END;
    else if (is(start, end, TEXT("HOME")))      return VK_HOME;
    else if (is(start, end, TEXT("LEFT")))      return VK_LEFT;
    else if (is(start, end, TEXT("UP")))        return VK_UP;
    else if (is(start, end, TEXT("RIGHT")))     return VK_RIGHT;
    else if (is(start, end, TEXT("DOWN")))      return VK_DOWN;
    else if (is(start, end, TEXT("LWIN")))      return VK_LWIN;
    else if (is(start, end, TEXT("RWIN")))      return VK_RWIN;
    else if (is(start, end, TEXT("APPS")))      return VK_APPS;
    else if (is(start, end, TEXT("BACK")))      return VK_BACK;
    else if (is(start, end, TEXT("TAB")))       return VK_TAB;
    // TODO add VK_BROWSER_*, VK_VOLUME_*, VK_MEDIA_*
    else return 0;
}

JoyMappingThumbType GetThumbType(LPCWSTR s)
{
         if (_tcsicmp(s, TEXT("Mouse")) == 0)    return JMTT_MOUSE;
    else if (_tcsicmp(s, TEXT("Scroll")) == 0)   return JMTT_SCROLL;
    else if (_tcsicmp(s, TEXT("WASD")) == 0)     return JMTT_WASD;
    else return JMTT_NONE;
}

JoyMapping::JoyMapping()
{
    _tcscpy_s(strModule, _T(""));
    _tcscpy_s(strWndClass, _T(""));
    _tcscpy_s(strWndText, _T(""));

    for (int b = 0; b < XINPUT_MAX_BUTTONS; ++b)
    {
        JoyMappingButton& button = joyMappingButton[b];
        button.type = JMBT_NONE;
        button.keys[0] = 0;
    }

    for (int t = 0; t < JMT_MAX; ++t)
    {
        JoyMappingThumbType& thumb = joyMappingThumb[t];
        thumb = JMTT_NONE;
    }
}

bool LoadFromRegistry(HKEY hParent, LPCWSTR lpSubKey, JoyMapping& joyMapping)
{
    HKEY hKey = NULL;
    if (RegOpenKey(hParent, lpSubKey, &hKey) == ERROR_SUCCESS)
    {
        LoadRegString(hKey, L"Module", joyMapping.strModule, ARRAYSIZE(joyMapping.strModule), _T(""));
        LoadRegString(hKey, L"Class", joyMapping.strWndClass, ARRAYSIZE(joyMapping.strWndClass), _T("*"));
        LoadRegString(hKey, L"Title", joyMapping.strWndText, ARRAYSIZE(joyMapping.strWndText), _T("*"));

        TCHAR strTemp[ARRAYSIZE(JoyMappingButton::keys)] = L"";
        for (int b = 0; b < XINPUT_MAX_BUTTONS; ++b)
        {
            LPCWSTR n = GetButtonName(1 << b);
            JoyMappingButton& button = joyMapping.joyMappingButton[b];
            if (LoadRegString(hKey, n, strTemp, ARRAYSIZE(strTemp), _T("")))
            {
                if (_tcscmp(strTemp, TEXT("")) == 0)
                {
                    button.type = JMBT_NONE;
                    button.keys[0] = 0;
                }
                else
                {
                    button.type = JMBT_KEYS;
                    const TCHAR* strParseIn = strTemp;
                    WORD* strParseOut = button.keys;
                    while (*strParseIn != TEXT('\0'))
                    {
                        const TCHAR* end = nullptr;
                        if (*strParseIn == TEXT('[') && (end = _tcsrchr(strParseIn + 1, _T(']'))) != nullptr)
                        {
                            *strParseOut = GetButton(strParseIn + 1, end);
                            strParseIn = end;
                        }
                        else
                            *strParseOut = *strParseIn;
                        ++strParseOut;
                        ++strParseIn;
                    }
                    *strParseOut = 0;
                }
            }
        }

        for (int t = 0; t < JMT_MAX; ++t)
        {
            LPCWSTR n = GetThumbName(static_cast<JoyThumb>(t));
            if (LoadRegString(hKey, n, strTemp, ARRAYSIZE(strTemp), _T("")))
            {
                JoyMappingThumbType& thumb = joyMapping.joyMappingThumb[t];
                thumb = GetThumbType(strTemp);
            }
        }

        RegCloseKey(hKey);
        return true;
    }
    return false;
}

void Init(JoyX& joyx)
{
    HKEY hMappingKey = NULL;
    RegOpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\RadSoft\\RadJoyKeyX\\Mapping", &hMappingKey);

    joyx.joyMapping.joyMappingThumb[JMT_LEFT] = JMTT_MOUSE;
    joyx.joyMapping.joyMappingThumb[JMT_RIGHT] = JMTT_SCROLL;
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =					{ JMBT_COMMAND, JMC_BUTTON };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =					{ JMBT_KEYS, { VK_ESCAPE, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_X)] =					{ JMBT_KEYS, { VK_RBUTTON, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =					  { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_UP)] =				{ JMBT_KEYS, { VK_UP, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_DOWN)] =			{ JMBT_KEYS, { VK_DOWN, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_LEFT)] =			{ JMBT_KEYS, { VK_LEFT, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_RIGHT)] =			{ JMBT_KEYS, { VK_RIGHT, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =				{ JMBT_KEYS, { VK_LWIN, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_BACK)] =				  { JMBT_KEYS, { VK_CONTROL, VK_MENU, VK_TAB, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =		  { JMBT_KEYS, { VK_MEDIA_PREV_TRACK, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] =      { JMBT_KEYS, { VK_MEDIA_NEXT_TRACK, 0 } };

	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =				  { JMBT_COMMAND, JMC_BUTTON };
	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =				  { JMBT_KEYS, { VK_ESCAPE, 0 } };
	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_X)] =				  { JMBT_KEYS, { VK_RBUTTON, 0 } };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =				{ JMBT_KEYS, { VK_MEDIA_STOP, 0 } };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =			{ JMBT_COMMAND, JMC_TURN_OFF };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =	{ JMBT_KEYS, { VK_VOLUME_MUTE, 0 } };
	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] =	  { JMBT_COMMAND, JMC_SHOW_WINDOW };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_UP)] =			{ JMBT_KEYS, { VK_VOLUME_UP, 0 } };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_DOWN)] =		{ JMBT_KEYS, { VK_VOLUME_DOWN, 0 } };
    
	{
		JoyMapping& joyMappingMedia = joyx.joyMappingOther[L"MediaPlayer"];
        joyMappingMedia = joyx.joyMapping;
		_tcscpy_s(joyMappingMedia.strModule, _T("*\\mpc-hc64.exe"));
		_tcscpy_s(joyMappingMedia.strWndClass, _T("*"));
		_tcscpy_s(joyMappingMedia.strWndText, _T("*"));
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =              { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE, 0 } };
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =              { JMBT_KEYS, { VK_MENU, VK_RETURN, 0 } };
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =  { JMBT_KEYS, { VK_MEDIA_PREV_TRACK, 0 } };
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] = { JMBT_KEYS, { VK_MEDIA_NEXT_TRACK, 0 } };
	}

	{
		JoyMapping& joyMappingBrowser = joyx.joyMappingOther[L"Browser"];
        joyMappingBrowser = joyx.joyMapping;
		_tcscpy_s(joyMappingBrowser.strModule, _T("*\\ApplicationFrameHost.exe"));
		_tcscpy_s(joyMappingBrowser.strWndClass, _T("ApplicationFrameWindow"));
		_tcscpy_s(joyMappingBrowser.strWndText, _T("*- Microsoft Edge"));
		joyMappingBrowser.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =				 { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE, 0 } };
		joyMappingBrowser.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =  { JMBT_KEYS, { VK_BROWSER_BACK, 0 } };
		joyMappingBrowser.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] = { JMBT_KEYS, { VK_BROWSER_FORWARD, 0 } };
	}

    int i = 0;
    while (true)
    {
        TCHAR n[100];
        DWORD l = ARRAYSIZE(n);
        if (RegEnumKeyEx(hMappingKey, i, n, &l, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        JoyMapping& joyMapping = joyx.joyMappingOther[n];
        LoadFromRegistry(hMappingKey, n, joyMapping);
        ++i;
    }

    joyx.altKey = XINPUT_GAMEPAD_BACK;

	joyx.notifyState = QUNS_ACCEPTS_NOTIFICATIONS;

	HMODULE hXInput = FindModule(-1, _T("*\\xinput*.dll"));
	joyx.XInputPowerOffController = reinterpret_cast<XInputPowerOffController_t>(GetProcAddress(hXInput, (LPCSTR)103));

    RegCloseKey(hMappingKey);
}

void AppendBattInfo(TCHAR* s, int length, const XINPUT_BATTERY_INFORMATION* joyBattery)
{
	for (int j = 0; j < XUSER_MAX_COUNT; ++j)
	{
		TCHAR str[1024];
		switch (joyBattery[j].BatteryType)
		{
		case BATTERY_TYPE_DISCONNECTED:
			//_stprintf_s(str, _T("\n%d: disconnected"), (j + 1));
			//_tcscat_s(s, length, str);
			break;
		case BATTERY_TYPE_WIRED:
			_stprintf_s(str, _T("\n%d: wired"), (j + 1));
			_tcscat_s(s, length, str);
			break;
		case BATTERY_TYPE_ALKALINE:
		case BATTERY_TYPE_NIMH:
			_stprintf_s(str, _T("\n%d: batt: %.*s%.*s"), (j + 1), joyBattery[j].BatteryLevel, _T("###"), (BATTERY_LEVEL_FULL - joyBattery[j].BatteryLevel), _T("---"));
			_tcscat_s(s, length, str);
			break;
		default:
			_stprintf_s(str, _T("\n%d: unknown"), (j + 1));
			_tcscat_s(s, length, str);
			break;
		}
	}
}

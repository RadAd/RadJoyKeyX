#include "stdafx.h"
#include "JoyX.h"
#include "Utils.h"
#include "WinUtils.h"
#include <Shlwapi.h>
#include <math.h>

#define KF_SCANKEY      0x0400
#define VK_BUTTON       0x88 // Currently unassigned

#define MOUSE_SENSITIVITY 2
#define MOUSE_SPEED 5

#define DEFAULT_MAPPING L"Default"
#define ALT_MAPPING L"Alt"

#define XINPUT_GAMEPAD_GUIDE      0x0400

inline POINTFLOAT Normalize(SHORT x, SHORT y, SHORT deadZone)
{
	return {
		std::copysignf(Power(std::fabs(Normalize(x, deadZone)), MOUSE_SENSITIVITY), x),
        std::copysignf(Power(std::fabs(Normalize(y, deadZone)), MOUSE_SENSITIVITY), y)
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

#if 0
	bool bSend = true;
	if (!bDown)
		bSend = keyDown[ip.ki.wVk];
#else
    bool bSend = bDown != keyDown[ip.ki.wVk];
#endif
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
    bool ret = false;

    if (dx != 0)
    {
        //DebugOut(_T("SendMouse x: %d y: %d\n"), dx, dy);
        INPUT ip = { INPUT_MOUSE };
        ip.mi.mouseData = dx;
        ip.mi.dwFlags = MOUSEEVENTF_HWHEEL;
        SendInput(1, &ip, sizeof(INPUT));
        ret = true;
    }

    if (dy != 0)
    {
        //DebugOut(_T("SendMouse x: %d y: %d\n"), dx, dy);
        INPUT ip = { INPUT_MOUSE };
        ip.mi.mouseData = dy;
        ip.mi.dwFlags = MOUSEEVENTF_WHEEL;
        SendInput(1, &ip, sizeof(INPUT));
        ret = true;
    }

    return ret;
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

inline bool GetTriggerDown(const XINPUT_STATE& state, JoyThumb t)
{
    switch (t)
    {
    case JMT_LEFT:
        return state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    case JMT_RIGHT:
        return state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    default:
        return false;
    }
}

inline bool IsOnButtonDown(const XINPUT_STATE& state, const XINPUT_STATE& stateold, WORD button)
{
	return !(stateold.Gamepad.wButtons & button)
		&& (state.Gamepad.wButtons & button);
}

inline bool IsOnButtonUp(const XINPUT_STATE& state, const XINPUT_STATE& stateold, WORD button)
{
	return (stateold.Gamepad.wButtons & button)
		&& !(state.Gamepad.wButtons & button);
}

inline bool IsOnTriggerDown(const XINPUT_STATE& state, const XINPUT_STATE& stateold, JoyThumb t)
{
    return !GetTriggerDown(stateold, t)
        && GetTriggerDown(state, t);
}

inline bool IsOnTriggerUp(const XINPUT_STATE& state, const XINPUT_STATE& stateold, JoyThumb t)
{
    return GetTriggerDown(stateold, t)
        && !GetTriggerDown(state, t);
}

WORD Translate(WORD key, JoyMappingLast& joyLast)
{
    switch (key)
    {
    case VK_BUTTON:
        switch (joyLast)
        {
        case JML_MOUSE: return VK_LBUTTON;
        case JML_KEYBOARD: return  VK_RETURN;
        default: return key;
        }
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
        joyLast = JML_MOUSE;
        return key;
    default:
        joyLast = JML_KEYBOARD;
        return key;
    }
}

bool DoButtonDown(bool bIsOnDown, const WORD* const keys, bool* keyDown, JoyMappingLast& joyLast)
{
	if (bIsOnDown)
	{
		int k = 0;
		while (keys[k] != 0)
		{
            WORD key = Translate(keys[k], joyLast);
            SendKey(key, true, keyDown);
			++k;
		}
		return true;
	}
	return false;
}

bool DoButtonUp(bool bIsOnUp, const WORD* const keys, bool* keyDown, JoyMappingLast joyLast)
{
	if (bIsOnUp)
	{
		int k = 0;
		while (keys[k] != 0)
		{
            WORD key = Translate(keys[k], joyLast);
            SendKey(key, false, keyDown);
			++k;
		}
		return true;
	}
	return false;
}

bool IsWnd(const WndSpec& wnd, const WndSpec& spec)
{
    bool match = true;
    match &= spec.strModule[0] != L'\0' && PathMatchSpec(wnd.strModule, spec.strModule) != FALSE;
    match &= spec.strWndClass[0] != L'\0' && PathMatchSpec(wnd.strWndClass, spec.strWndClass) != FALSE;
    match &= spec.strWndText[0] != L'\0' && PathMatchSpec(wnd.strWndText, spec.strWndText) != FALSE;
    return match;
}

bool Update(WndInfo& wndInfo)
{
	DWORD r;
	HWND hWndFG = GetForegroundWindow();
	if (hWndFG != NULL && hWndFG != wndInfo.hWndFG)
	{
		GetWindowThreadProcessId(hWndFG, &wndInfo.pid);

		r = GetModuleFileNameEx2(wndInfo.pid, wndInfo.spec.strModule, ARRAYSIZE(wndInfo.spec.strModule));
		if (r == 0)
            _tcscpy_s(wndInfo.spec.strModule, L"[Unknown]");
		r = GetClassName(hWndFG, wndInfo.spec.strWndClass, ARRAYSIZE(wndInfo.spec.strWndClass));
		if (r == 0)
            _tcscpy_s(wndInfo.spec.strWndClass, L"");
		r = GetWindowText(hWndFG, wndInfo.spec.strWndText, ARRAYSIZE(wndInfo.spec.strWndText));
		if (r == 0)
            _tcscpy_s(wndInfo.spec.strWndText, L"");
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
        _tcscpy_s(wndInfo.spec.strModule, L"[Unknown]");
        _tcscpy_s(wndInfo.spec.strWndClass, L"");
        _tcscpy_s(wndInfo.spec.strWndText, L"");
        wndInfo.bUsesXinput = false;
        wndInfo.hWndFG = hWndFG;
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

const JoyMapping& GetWndJoyMapping(const JoyX& joyx, std::wstring& name)
{
    //DebugOut(_T("\n  Find: %s\n"), joyx.wndInfoFG.spec.strModule);
    for (auto it = joyx.joyMapping.begin(); it != joyx.joyMapping.end(); ++it)
    {
        const JoyMapping& thisJoyMapping = it->second;
        //DebugOut(_T("    Search: %s %s\n"), it->first.c_str(), thisJoyMapping.spec.strModule);
        if (IsWnd(joyx.wndInfoFG.spec, thisJoyMapping.spec))
        {
            name = it->first;
            //DebugOut(_T("    Found: %s %s\n"), it->first.c_str(), thisJoyMapping.spec.strModule);
            return thisJoyMapping;
        }
    }
    name = DEFAULT_MAPPING;
    auto it = joyx.joyMapping.find(name);
    //ASSERT(it != joyx.joyMapping.end());
    return it->second;
}

JoystickRet DoJoystick(JoyX& joyx)
{
    JoystickRet ret;
	//printf("time: %d %d\n", ticksleep, tick);
	DWORD r;

	bool bWindowChanged = false;
    bWindowChanged = Update(joyx.wndInfoFG) || bWindowChanged;
    bWindowChanged = Update(joyx.notifyState) || bWindowChanged;

    std::wstring nameMapping;
    const JoyMapping& wndJoyMapping = GetWndJoyMapping(joyx, nameMapping);
    
	{
		bool bEnabled = !joyx.wndInfoFG.bUsesXinput || joyx.notifyState >= QUNS_ACCEPTS_NOTIFICATIONS || &wndJoyMapping != &joyx.joyMapping[DEFAULT_MAPPING];
		// TODO Enable if Steam
        if (bEnabled != joyx.bEnabled)
        {
            //MessageBeep(bEnabled ? MB_ICONINFORMATION : MB_ICONWARNING);
            joyx.bEnabled = bEnabled;
            bWindowChanged = true;
        }
	}

	if (bWindowChanged)
	{
		TCHAR* exe = _tcsrchr(joyx.wndInfoFG.spec.strModule, _T('\\'));
		exe = exe == nullptr ? joyx.wndInfoFG.spec.strModule : exe + 1;
		//DebugOut(_T("Module: 0x%x %s\n"), (UINT)hWndFG, exe);
		DebugOut(_T("%c %s %s \"%s\"\n"), (joyx.bEnabled ? '+' : '-'), nameMapping.c_str(), exe, joyx.wndInfoFG.spec.strWndText);
        //DebugOut(_T("  Uses X Input: %s\n"), _(joyx.wndInfoFG.bUsesXinput));
        //DebugOut(_T("  Notify State: %d\n"), joyx.notifyState);
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
		r = joyx.XInputGetState(j, &joyState);
		if (r == ERROR_SUCCESS)
		{
            const JoyMapping* joyMappingAlt = nullptr;
            for (int b = 0; b < XINPUT_MAX_BUTTONS; ++b)
            {
                const JoyMappingButton& joyMappingButton = wndJoyMapping.joyMappingButton[b];
                const WORD mask = 1 << b;
                if (joyMappingButton.type == JMBT_ALT)
                {
                    if (joyx.bEnabled && (joyState.Gamepad.wButtons & mask))
                    {
                        joyMappingAlt = &joyx.joyMapping[joyMappingButton.strMapping];
                    }
                }
            }

            const JoyMapping& joyMapping = joyMappingAlt != nullptr ? *joyMappingAlt : wndJoyMapping;

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
                    // if (joyCapabilities.Gamepad.wButtons & mask)
					switch (joyMappingButton.type)
					{
					case JMBT_KEYS:
                        if (joyx.bEnabled)
                            DoButtonDown(IsOnButtonDown(joyState, joyx.joyState[j], mask), joyMappingButton.keys, joyx.keyDown, joyx.joyLast);
						DoButtonUp(IsOnButtonUp(joyState, joyx.joyState[j], mask), joyMappingButton.keys, joyx.keyDown, joyx.joyLast);
						break;

					case JMBT_TURN_OFF:
                        if (joyx.bEnabled && IsOnButtonDown(joyState, joyx.joyState[j], mask))
                        {
                            if (joyx.XInputPowerOffController != nullptr)
                                joyx.XInputPowerOffController(j);
                        }
                        break;
                    }
				}

                for (int i = 0; i < JMT_MAX; ++i)
                {
                    JoyThumb t = static_cast<JoyThumb>(i);
                    const JoyMappingButton& joyMappingButton = joyMapping.joyMappingTrigger[i];
                    if (joyx.bEnabled)
                        DoButtonDown(IsOnTriggerDown(joyState, joyx.joyState[j], t), joyMappingButton.keys, joyx.keyDown, joyx.joyLast);
                    DoButtonUp(IsOnTriggerUp(joyState, joyx.joyState[j], t), joyMappingButton.keys, joyx.keyDown, joyx.joyLast);
                }

                // TODO User override joyx.bEnabled
            }

			if (joyx.bEnabled)
			{
                for (int i = 0; i < JMT_MAX; ++i)
                {
                    JoyThumb t = static_cast<JoyThumb>(i);
                    POINTFLOAT pf = Normalize(GetThumbX(joyState, t), GetThumbY(joyState, t), XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                    //if (t == JMT_RIGHT)
                        //DebugOut(L"Thumb %f %f\n", pf.x, pf.y);

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
    case XINPUT_GAMEPAD_GUIDE: return TEXT("Guide");
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

LPCWSTR GetTriggerName(JoyThumb t)
{
    switch (t)
    {
    case JMT_LEFT: return TEXT("TriggerLeft");
    case JMT_RIGHT: return TEXT("TriggerRight");
    default: return nullptr;
    }
}

bool is(LPCWSTR start, LPCWSTR end, LPCWSTR compare)
{
    size_t l = _tcsclen(compare);
    return (end - start) == l && _tcsncicmp(start, compare, l) == 0;
}

WORD GetKey(LPCWSTR start, LPCWSTR end)
{
         if (is(start, end, TEXT("LBUTTON")))   return VK_LBUTTON;
    else if (is(start, end, TEXT("RBUTTON")))   return VK_RBUTTON;
    else if (is(start, end, TEXT("MBUTTON")))   return VK_MBUTTON;
    else if (is(start, end, TEXT("RETURN")))    return VK_RETURN;
    else if (is(start, end, TEXT("SHIFT")))     return VK_SHIFT;
    else if (is(start, end, TEXT("CONTROL")))   return VK_CONTROL;
    else if (is(start, end, TEXT("CTRL")))      return VK_CONTROL;
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
    else if (is(start, end, TEXT("F1")))        return VK_F1;
    else if (is(start, end, TEXT("F2")))        return VK_F2;
    else if (is(start, end, TEXT("F3")))        return VK_F3;
    else if (is(start, end, TEXT("F4")))        return VK_F4;
    else if (is(start, end, TEXT("F5")))        return VK_F5;
    else if (is(start, end, TEXT("F6")))        return VK_F6;
    else if (is(start, end, TEXT("F7")))        return VK_F7;
    else if (is(start, end, TEXT("F8")))        return VK_F8;
    else if (is(start, end, TEXT("F9")))        return VK_F9;
    else if (is(start, end, TEXT("F10")))       return VK_F10;
    else if (is(start, end, TEXT("F11")))       return VK_F11;
    else if (is(start, end, TEXT("F12")))       return VK_F12;
    else if (is(start, end, TEXT("BROWSER_BACK")))      return VK_BROWSER_BACK;
    else if (is(start, end, TEXT("BROWSER_FORWARD")))   return VK_BROWSER_FORWARD;
    else if (is(start, end, TEXT("BROWSER_REFRESH")))   return VK_BROWSER_REFRESH;
    else if (is(start, end, TEXT("BROWSER_STOP")))      return VK_BROWSER_STOP;
    else if (is(start, end, TEXT("BROWSER_SEARCH")))    return VK_BROWSER_SEARCH;
    else if (is(start, end, TEXT("BROWSER_FAVORITES"))) return VK_BROWSER_FAVORITES;
    else if (is(start, end, TEXT("BROWSER_HOME")))      return VK_BROWSER_HOME;
    else if (is(start, end, TEXT("VOLUME_MUTE")))       return VK_VOLUME_MUTE;
    else if (is(start, end, TEXT("VOLUME_DOWN")))       return VK_VOLUME_DOWN;
    else if (is(start, end, TEXT("VOLUME_UP")))         return VK_VOLUME_UP;
    else if (is(start, end, TEXT("MEDIA_NEXT_TRACK")))  return VK_MEDIA_NEXT_TRACK;
    else if (is(start, end, TEXT("MEDIA_PREV_TRACK")))  return VK_MEDIA_PREV_TRACK;
    else if (is(start, end, TEXT("MEDIA_STOP")))        return VK_MEDIA_STOP;
    else if (is(start, end, TEXT("MEDIA_PLAY_PAUSE")))  return VK_MEDIA_PLAY_PAUSE;
    else if (is(start, end, TEXT("BUTTON")))  return VK_BUTTON;
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
    _tcscpy_s(spec.strModule, _T(""));
    _tcscpy_s(spec.strWndClass, _T(""));
    _tcscpy_s(spec.strWndText, _T(""));

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

void ParseButton(JoyMappingButton& button, LPCWSTR strTemp)
{
    if (_tcscmp(strTemp, TEXT("")) == 0)
    {
        button.type = JMBT_NONE;
        button.keys[0] = 0;
    }
    else if (_tcscmp(strTemp, TEXT("[TURN_OFF]")) == 0)
    {
        button.type = JMBT_TURN_OFF;
        button.keys[0] = 0;
    }
    else if (strTemp[0] == L'!')
    {
        button.type = JMBT_ALT;
        _tcscpy_s(button.strMapping, strTemp + 1);
    }
    else
    {
        button.type = JMBT_KEYS;
        const TCHAR* strParseIn = strTemp;
        WORD* strParseOut = button.keys;
        while (*strParseIn != TEXT('\0'))
        {
            const TCHAR* end = nullptr;
            if (*strParseIn == TEXT('[') && (end = _tcschr(strParseIn + 1, _T(']'))) != nullptr)
            {
                *strParseOut = GetKey(strParseIn + 1, end);
                if (*strParseOut == 0)
                    DebugOut(L"Error: can't find key %s\n", std::wstring(strParseIn + 1, end).c_str());
                strParseIn = end;
            }
            else
            {
                switch (*strParseIn)
                {
                case L';': case L':':    *strParseOut = VK_OEM_1; break;
                case L'+': case L'=':    *strParseOut = VK_OEM_PLUS; break;
                case L',': case L'<':    *strParseOut = VK_OEM_COMMA; break;
                case L'-': case L'_':    *strParseOut = VK_OEM_MINUS; break;
                case L'.': case L'>':    *strParseOut = VK_OEM_PERIOD; break;
                case L'/': case L'?':    *strParseOut = VK_OEM_2; break;
                case L'`': case L'~':    *strParseOut = VK_OEM_3; break;
                case L'[': case L'{':    *strParseOut = VK_OEM_4; break;
                case L'\\': case L'|':   *strParseOut = VK_OEM_5; break;
                case L']': case L'}':    *strParseOut = VK_OEM_6; break;
                case L'\'': case L'\"':  *strParseOut = VK_OEM_7; break;
                default:                 *strParseOut = *strParseIn; break;
                }
            }
            ++strParseOut;
            ++strParseIn;
        }
        *strParseOut = 0;
    }
}

bool LoadFromRegistry(HKEY hParent, LPCWSTR lpSubKey, JoyMapping& joyMapping)
{
    DebugOut(L"LoadFromRegistry: %s\n", lpSubKey);
    HKEY hKey = NULL;
    if (RegOpenKey(hParent, lpSubKey, &hKey) == ERROR_SUCCESS)
    {
        TCHAR strTemp[1024] = L"";
        if (RegGetString(hKey, L"Base", strTemp, ARRAYSIZE(strTemp), _T("")))
        {
            DebugOut(L"   Base: %s\n", strTemp);
            LoadFromRegistry(hParent, strTemp, joyMapping);
        }

        RegGetString(hKey, L"Module", joyMapping.spec.strModule, ARRAYSIZE(joyMapping.spec.strModule), _T(""));
        RegGetString(hKey, L"Class", joyMapping.spec.strWndClass, ARRAYSIZE(joyMapping.spec.strWndClass), _T("*"));
        RegGetString(hKey, L"Title", joyMapping.spec.strWndText, ARRAYSIZE(joyMapping.spec.strWndText), _T("*"));

        for (int b = 0; b < XINPUT_MAX_BUTTONS; ++b)
        {
            LPCWSTR n = GetButtonName(1 << b);
            if (n != nullptr && RegGetString(hKey, n, strTemp, ARRAYSIZE(strTemp), _T("")))
            {
                JoyMappingButton& button = joyMapping.joyMappingButton[b];
                DebugOut(L"   Button: %s %s\n", n, strTemp);
                ParseButton(button, strTemp);
            }
        }

        for (int t = 0; t < JMT_MAX; ++t)
        {
            LPCWSTR n = GetThumbName(static_cast<JoyThumb>(t));
            if (RegGetString(hKey, n, strTemp, ARRAYSIZE(strTemp), _T("")))
            {
                DebugOut(L"   Thumb: %s %s\n", n, strTemp);
                JoyMappingThumbType& thumb = joyMapping.joyMappingThumb[t];
                thumb = GetThumbType(strTemp);
                if (thumb == JMTT_NONE)
                    DebugOut(L"Error: can't find thumb %s\n", strTemp);
            }
        }

        for (int t = 0; t < JMT_MAX; ++t)
        {
            LPCWSTR n = GetTriggerName(static_cast<JoyThumb>(t));
            if (RegGetString(hKey, n, strTemp, ARRAYSIZE(strTemp), _T("")))
            {
                JoyMappingButton& button = joyMapping.joyMappingTrigger[t];
                DebugOut(L"   Trigger: %s %s\n", n, strTemp);
                ParseButton(button, strTemp);
            }
        }

        RegCloseKey(hKey);
        return true;
    }
    else if (_tcsicmp(lpSubKey, DEFAULT_MAPPING) == 0)
    {
        //_tcscpy_s(joyMapping.strModule, _T("*"));

        joyMapping.joyMappingThumb[JMT_LEFT] = JMTT_MOUSE;
        joyMapping.joyMappingThumb[JMT_RIGHT] = JMTT_SCROLL;
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =          { JMBT_KEYS, { VK_BUTTON, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =          { JMBT_KEYS, { VK_ESCAPE, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_X)] =          { JMBT_KEYS, { VK_RBUTTON, 0 } };
        //joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =          { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_UP)] =    { JMBT_KEYS, { VK_UP, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_DOWN)] =  { JMBT_KEYS, { VK_DOWN, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_LEFT)] =  { JMBT_KEYS, { VK_LEFT, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_RIGHT)] = { JMBT_KEYS, { VK_RIGHT, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =      { JMBT_KEYS, { VK_LWIN, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_BACK)] =       { JMBT_ALT };
        _tcscpy_s(joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_BACK)].strMapping, ALT_MAPPING);
        //joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =    { JMBT_KEYS, { VK_MEDIA_PREV_TRACK, 0 } };
        //joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] =   { JMBT_KEYS, { VK_MEDIA_NEXT_TRACK, 0 } };
    }
    else if (_tcsicmp(lpSubKey, ALT_MAPPING) == 0)
    {
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =                { JMBT_KEYS, { VK_MEDIA_STOP, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =            { JMBT_TURN_OFF };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =    { JMBT_KEYS, { VK_VOLUME_MUTE, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_UP)] =          { JMBT_KEYS, { VK_VOLUME_UP, 0 } };
        joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_DOWN)] =        { JMBT_KEYS, { VK_VOLUME_DOWN, 0 } };
    }
    else
    {
        DebugOut(L"Error: cant't find mapping %s\n", lpSubKey);
    }
    return false;
}

void LoadMapping(JoyX& joyx)
{
    HKEY hMappingKey = NULL;
    RegOpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\RadSoft\\RadJoyKeyX\\Mapping", &hMappingKey);

    joyx.joyMapping.clear();
    LoadFromRegistry(hMappingKey, DEFAULT_MAPPING, joyx.joyMapping[DEFAULT_MAPPING]);
    LoadFromRegistry(hMappingKey, ALT_MAPPING, joyx.joyMapping[ALT_MAPPING]);

    int i = 0;
    while (true)
    {
        TCHAR n[100];
        DWORD l = ARRAYSIZE(n);
        if (RegEnumKeyEx(hMappingKey, i, n, &l, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        auto it = joyx.joyMapping.find(n);
        if (it == joyx.joyMapping.end())
        {
            JoyMapping& joyMapping = joyx.joyMapping[n];
            LoadFromRegistry(hMappingKey, n, joyMapping);
        }
        ++i;
    }

    RegCloseKey(hMappingKey);
}

void Init(JoyX& joyx)
{
    LoadMapping(joyx);

	joyx.notifyState = QUNS_ACCEPTS_NOTIFICATIONS;

    HMODULE hXInput = FindModule(-1, _T("*\\xinput*.dll"));
    joyx.XInputPowerOffController = reinterpret_cast<XInputPowerOffController_t>(GetProcAddress(hXInput, (LPCSTR) 103));
    joyx.XInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(hXInput, (LPCSTR) 100));
    if (joyx.XInputGetState == nullptr)
        joyx.XInputGetState = XInputGetState;
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

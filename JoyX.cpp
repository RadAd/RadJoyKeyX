#include "stdafx.h"
#include "JoyX.h"
#include "Utils.h"
#include "WinUtils.h"
#include <Shlwapi.h>

#define KF_VKEY	   0x0400

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
	if (wScan & KF_VKEY)
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
	else
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

	bool bSend = true;
	if (!bDown)
		bSend = keyDown[ip.ki.wVk];
	keyDown[ip.ki.wVk] = bDown;

#if _DEBUG
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
    for (int i = 0; i < joyx.joyMappingOtherCount; ++i)
    {
        const JoyMapping& thisJoyMapping = joyx.joyMappingOther[i];
        //DebugOut(_T("    Search: %s\n"), thisJoyMapping.strModule);
        if (IsWnd(joyx.wndInfoFG, thisJoyMapping.strModule, thisJoyMapping.strWndClass, thisJoyMapping.strWndText))
        {
            DebugOut(_T("    Found: %s\n"), thisJoyMapping.strModule);
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
                                    SendKey(VK_LBUTTON | KF_VKEY, true, joyx.keyDown);
                                }
                                else if (joyx.joyLast == JML_KEYBOARD)
                                {
                                    SendKey(VK_RETURN | KF_VKEY, true, joyx.keyDown);
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
									SendKey(VK_LBUTTON | KF_VKEY, false, joyx.keyDown);
								}
								else if (joyx.joyLast == JML_KEYBOARD)
								{
									SendKey(VK_RETURN | KF_VKEY, false, joyx.keyDown);
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
                            if (pf.x < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                                bSend |= SendKey('A' | KF_VKEY, true, joyx.keyDown);
                            else if (pf.x > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                                bSend |= SendKey('D' | KF_VKEY, true, joyx.keyDown);
                            else
                            {
                                bSend |= SendKey('A' | KF_VKEY, false, joyx.keyDown);
                                bSend |= SendKey('D' | KF_VKEY, false, joyx.keyDown);
                            }

                            if (pf.y < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                                bSend |= SendKey('W' | KF_VKEY, true, joyx.keyDown);
                            else if (pf.y > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                                bSend |= SendKey('S' | KF_VKEY, true, joyx.keyDown);
                            else
                            {
                                bSend |= SendKey('W' | KF_VKEY, false, joyx.keyDown);
                                bSend |= SendKey('S' | KF_VKEY, false, joyx.keyDown);
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

void Init(JoyX& joyx)
{
    joyx.joyMapping.joyMappingThumb[JMT_LEFT] = JMTT_MOUSE;
    joyx.joyMapping.joyMappingThumb[JMT_RIGHT] = JMTT_SCROLL;
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =					{ JMBT_COMMAND, JMC_BUTTON };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =					{ JMBT_KEYS, { VK_ESCAPE | KF_VKEY, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_X)] =					{ JMBT_KEYS, { VK_RBUTTON | KF_VKEY, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =					  { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE | KF_VKEY, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_UP)] =				{ JMBT_KEYS, { VK_UP | KF_VKEY, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_DOWN)] =			{ JMBT_KEYS, { VK_DOWN | KF_VKEY, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_LEFT)] =			{ JMBT_KEYS, { VK_LEFT | KF_VKEY, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_RIGHT)] =			{ JMBT_KEYS, { VK_RIGHT | KF_VKEY, 0 } };
	joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =				{ JMBT_KEYS, { VK_LWIN | KF_VKEY, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_BACK)] =				  { JMBT_KEYS, { VK_CONTROL | KF_VKEY, VK_MENU | KF_VKEY, VK_TAB | KF_VKEY, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =		  { JMBT_KEYS, { VK_MEDIA_PREV_TRACK | KF_VKEY, 0 } };
	//joyx.joyMapping.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] =      { JMBT_KEYS, { VK_MEDIA_NEXT_TRACK | KF_VKEY, 0 } };

	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =				  { JMBT_COMMAND, JMC_BUTTON };
	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =				  { JMBT_KEYS, { VK_ESCAPE | KF_VKEY, 0 } };
	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_X)] =				  { JMBT_KEYS, { VK_RBUTTON | KF_VKEY, 0 } };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =				{ JMBT_KEYS, { VK_MEDIA_STOP | KF_VKEY, 0 } };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =			{ JMBT_COMMAND, JMC_TURN_OFF };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =	{ JMBT_KEYS, { VK_VOLUME_MUTE | KF_VKEY, 0 } };
	//joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] =	  { JMBT_COMMAND, JMC_SHOW_WINDOW };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_UP)] =			{ JMBT_KEYS, { VK_VOLUME_UP | KF_VKEY, 0 } };
	joyx.joyMappingAlt.joyMappingButton[LBS(XINPUT_GAMEPAD_DPAD_DOWN)] =		{ JMBT_KEYS, { VK_VOLUME_DOWN | KF_VKEY, 0 } };
    
	{
		JoyMapping& joyMappingMedia = joyx.joyMappingOther[joyx.joyMappingOtherCount++];
        joyMappingMedia = joyx.joyMapping;
		_tcscpy_s(joyMappingMedia.strModule, _T("*\\mpc-hc64.exe"));
		_tcscpy_s(joyMappingMedia.strWndClass, _T("*"));
		_tcscpy_s(joyMappingMedia.strWndText, _T("*"));
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =              { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE | KF_VKEY, 0 } };
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =              { JMBT_KEYS, { VK_MENU | KF_VKEY, VK_RETURN | KF_VKEY, 0 } };
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =  { JMBT_KEYS, { VK_MEDIA_PREV_TRACK | KF_VKEY, 0 } };
		joyMappingMedia.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] = { JMBT_KEYS, { VK_MEDIA_NEXT_TRACK | KF_VKEY, 0 } };
	}

    if (false)
    {
        JoyMapping& joyMappingMedia = joyx.joyMappingOther[joyx.joyMappingOtherCount++];
        joyMappingMedia = joyx.joyMapping;
        _tcscpy_s(joyMappingMedia.strModule, _T("*\\devenv.exe"));
    }

	{
		JoyMapping& joyMappingBrowser = joyx.joyMappingOther[joyx.joyMappingOtherCount++];
        joyMappingBrowser = joyx.joyMapping;
		_tcscpy_s(joyMappingBrowser.strModule, _T("*\\ApplicationFrameHost.exe"));
		_tcscpy_s(joyMappingBrowser.strWndClass, _T("ApplicationFrameWindow"));
		_tcscpy_s(joyMappingBrowser.strWndText, _T("*- Microsoft Edge"));
		joyMappingBrowser.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =				 { JMBT_KEYS, { VK_MEDIA_PLAY_PAUSE | KF_VKEY, 0 } };
		joyMappingBrowser.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =  { JMBT_KEYS, { VK_BROWSER_BACK | KF_VKEY, 0 } };
		joyMappingBrowser.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] = { JMBT_KEYS, { VK_BROWSER_FORWARD | KF_VKEY, 0 } };
	}

	{
		JoyMapping& joyMappingInvisibleInc = joyx.joyMappingOther[joyx.joyMappingOtherCount++];
		_tcscpy_s(joyMappingInvisibleInc.strModule, _T("*\\invisibleinc.exe"));
		_tcscpy_s(joyMappingInvisibleInc.strWndClass, _T("*"));
		_tcscpy_s(joyMappingInvisibleInc.strWndText, _T("*"));
        joyMappingInvisibleInc.joyMappingThumb[JMT_LEFT] = JMTT_MOUSE;
		joyMappingInvisibleInc.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =              { JMBT_KEYS,{ VK_LBUTTON | KF_VKEY, 0 } };
		joyMappingInvisibleInc.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =			  { JMBT_KEYS, { VK_SPACE| KF_VKEY, 0 } };
		joyMappingInvisibleInc.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =  { JMBT_KEYS,{ VK_MENU | KF_VKEY, 0 } };
		joyMappingInvisibleInc.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] = { JMBT_KEYS,{ VK_TAB | KF_VKEY, 0 } };
		joyMappingInvisibleInc.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_THUMB)] =     { JMBT_KEYS,{ VK_RETURN | KF_VKEY, 0 } };
	}

    {
        JoyMapping& joyMappingWitcher= joyx.joyMappingOther[joyx.joyMappingOtherCount++];
        _tcscpy_s(joyMappingWitcher.strModule, _T("*\\witcher.exe"));
        _tcscpy_s(joyMappingWitcher.strWndClass, _T("*"));
        _tcscpy_s(joyMappingWitcher.strWndText, _T("*"));
        //_tcscpy_s(joyMappingWitcher.strWndText, _T("The Witcher (*"));
        joyMappingWitcher.joyMappingThumb[JMT_LEFT] = JMTT_WASD;
        joyMappingWitcher.joyMappingThumb[JMT_RIGHT] = JMTT_MOUSE;
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_A)] =              { JMBT_KEYS, { VK_LBUTTON | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_B)] =              { JMBT_KEYS, { VK_RBUTTON | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_Y)] =              { JMBT_KEYS, { VK_TAB | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_SHOULDER)] =  { JMBT_KEYS, { 'Q' | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_RIGHT_SHOULDER)] = { JMBT_KEYS, { 'E' | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_LEFT_THUMB)] =     { JMBT_KEYS, { 'F' | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_START)] =          { JMBT_KEYS, { VK_ESCAPE | KF_VKEY, 0 } };
        joyMappingWitcher.joyMappingButton[LBS(XINPUT_GAMEPAD_BACK)] =           { JMBT_KEYS, { VK_SPACE | KF_VKEY, 0 } };
    }

    joyx.altKey = XINPUT_GAMEPAD_BACK;

	joyx.notifyState = QUNS_ACCEPTS_NOTIFICATIONS;

	HMODULE hXInput = FindModule(-1, _T("*\\xinput*.dll"));
	joyx.XInputPowerOffController = reinterpret_cast<XInputPowerOffController_t>(GetProcAddress(hXInput, (LPCSTR)103));
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

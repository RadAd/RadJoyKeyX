#pragma once

#include <map>
#include <string>

#define XINPUT_MAX_BUTTONS	   16

enum JoyMappingButtonType { JMBT_NONE, JMBT_KEYS, JMBT_COMMAND };
enum JoyMappingThumbType { JMTT_NONE, JMTT_MOUSE, JMTT_SCROLL, JMTT_WASD };
enum JoyMappingCommand { JMC_TURN_OFF, JMC_BUTTON, JMC_ALT };
enum JoyMappingLast { JML_MOUSE, JML_KEYBOARD };
enum JoyThumb { JMT_LEFT, JMT_RIGHT, JMT_MAX };

typedef DWORD(WINAPI* XInputPowerOffController_t)(DWORD i);

struct WndSpec
{
    TCHAR strModule[MAX_PATH];
    TCHAR strWndClass[MAX_PATH];
    TCHAR strWndText[MAX_PATH];
};

struct WndInfo
{
	HWND hWndFG;
	DWORD pid;
    WndSpec spec;
	bool bUsesXinput;
};

struct JoyMappingButton
{
	JoyMappingButtonType type;

	union
	{
		WORD keys[10];
		JoyMappingCommand command;
	};
};

struct JoyMapping
{
    JoyMapping();

    WndSpec spec;
	JoyMappingButton joyMappingButton[XINPUT_MAX_BUTTONS];
    JoyMappingThumbType joyMappingThumb[JMT_MAX];
};

struct JoyX
{
	XInputPowerOffController_t XInputPowerOffController;

	WndInfo wndInfoFG;
	QUERY_USER_NOTIFICATION_STATE notifyState;

	bool bEnabled;

	XINPUT_STATE joyState[XUSER_MAX_COUNT];
	XINPUT_BATTERY_INFORMATION joyBattery[XUSER_MAX_COUNT];

	std::map<std::wstring, JoyMapping> joyMapping;

	JoyMappingLast joyLast;
	bool keyDown[0xFF];
};

struct JoystickRet
{
    JoystickRet()
        : bBatteryChanged(false)
    { }

    bool bBatteryChanged : 1;
};

void Init(JoyX& joyx);
void LoadMapping(JoyX& joyx);
JoystickRet DoJoystick(JoyX& joyx);
void AppendBattInfo(TCHAR* s, int length, const XINPUT_BATTERY_INFORMATION* joyBattery);

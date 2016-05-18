#pragma once

#define XINPUT_MAX_BUTTONS	   16

#define J_BATTERY_CHANGED 1

enum JoyMappingButtonType { JMBT_NONE, JMBT_KEYS, JMBT_COMMAND };
enum JoyMappingThumbType { JMTT_NONE, JMTT_MOUSE, JMTT_SCROLL};
enum JoyMappingCommand { JMC_TURN_OFF, JMC_BUTTON };
enum JoyMappingLast { JML_MOUSE, JML_KEYBOARD };
enum JoyThumb { JMT_LEFT, JMT_RIGHT, JMT_MAX };

typedef DWORD(WINAPI* XInputPowerOffController_t)(DWORD i);

struct WndInfo
{
	HWND hWndFG;
	DWORD pid;
	TCHAR strModule[MAX_PATH];
	TCHAR strWndClass[MAX_PATH];
	TCHAR strWndText[MAX_PATH];
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
	TCHAR strModule[MAX_PATH];
	TCHAR strWndClass[MAX_PATH];
	TCHAR strWndText[MAX_PATH];
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

	WORD altKey;
	JoyMapping joyMapping;
	JoyMapping joyMappingAlt;
	JoyMapping joyMappingOther[10];
	int joyMappingOtherCount;
	JoyMappingLast joyLast;
	bool keyDown[0xFF];
};

void Init(JoyX& joyx);
DWORD DoJoystick(JoyX& joyx);
void AppendBattInfo(TCHAR* s, int length, const XINPUT_BATTERY_INFORMATION* joyBattery);

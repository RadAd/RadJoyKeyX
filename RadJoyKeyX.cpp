#include "stdafx.h"
#include "RadJoyKeyX.h"
#include "JoyX.h"
#include "resource.h"

#define MAX_LOADSTRING 100
#define WM_TRAY (WM_USER + 30)
#define ID_TRAY 100
#define TIMER_JOY 10

// Global Variables:
HINSTANCE g_hInst;                                // current instance
WCHAR g_szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR g_szWindowClass[MAX_LOADSTRING];            // the main window class name
JoyX g_JoyX;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void UpdateTrayMsg(HWND hWnd)
{
    NOTIFYICONDATA nid = { NOTIFYICONDATA_V2_SIZE };
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY;
    nid.uVersion = NOTIFYICON_VERSION;
    GetWindowText(hWnd, nid.szTip, ARRAYSIZE(nid.szTip));
    AppendBattInfo(nid.szTip, ARRAYSIZE(nid.szTip), g_JoyX.joyBattery);
    nid.uFlags = NIF_TIP;

    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RADJOYKEYX));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RADJOYKEYX));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RADJOYKEYX);
    wcex.lpszClassName  = g_szWindowClass;
    //wcex.hIconSm      = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	wcex.hIconSm		= wcex.hIcon;

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_hInst = hInstance; // Store instance handle in our global variable

   LoadStringW(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
   LoadStringW(hInstance, IDC_RADJOYKEYX, g_szWindowClass, MAX_LOADSTRING);
   MyRegisterClass(hInstance);
   Init(g_JoyX);

   HWND hWnd = CreateWindowW(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		{
			NOTIFYICONDATA nid = { NOTIFYICONDATA_V2_SIZE };
			nid.hWnd = hWnd;
			nid.uID = ID_TRAY;
			nid.uVersion = NOTIFYICON_VERSION;
			nid.uCallbackMessage = WM_TRAY;
			nid.hIcon = (HICON) GetClassLongPtr(hWnd, GCLP_HICONSM);
			GetWindowText(hWnd, nid.szTip, ARRAYSIZE(nid.szTip));
			AppendBattInfo(nid.szTip, ARRAYSIZE(nid.szTip), g_JoyX.joyBattery);
			nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;

			Shell_NotifyIcon(NIM_ADD, &nid);

			SetTimer(hWnd, TIMER_JOY, 10, NULL);
		}
		break;

	case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDM_ABOUT:
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case ID_FILE_RELOAD:
                LoadMapping(g_JoyX);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

	case WM_CONTEXTMENU:
		{
			SetFocus(hWnd);
			HMENU hMenu = GetSubMenu(GetMenu(hWnd), 0);
			//SetMenuDefaultItem(hMenu, IDM_SELECT, FALSE);
			TrackPopupMenu(hMenu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
			PostMessage(hWnd, WM_NULL, 0, 0);
		}
		break;

	case WM_TRAY:
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONUP:
			if (IsWindowEnabled(hWnd))
			{
				NOTIFYICONIDENTIFIER nii = { sizeof(NOTIFYICONIDENTIFIER) };
				nii.hWnd = hWnd;
				nii.uID = ID_TRAY;
				RECT r = { };
				Shell_NotifyIconGetRect(&nii, &r);

				SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELPARAM((r.right + r.left)/2, (r.bottom + r.top) / 2));
			}
			else
			{
				SetForegroundWindow(hWnd);
			}
			break;

		case WM_LBUTTONUP:
			if (!IsWindowEnabled(hWnd))
			{
				SetForegroundWindow(hWnd);
			}
			break;
		}
		break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_JOY:
			if (IsWindowEnabled(hWnd))
			{
                JoystickRet ret = DoJoystick(g_JoyX);
                if (ret.bBatteryChanged)
                    UpdateTrayMsg(hWnd);
			}
			break;
		}
		break;


	case WM_DESTROY:
		{
			NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
			nid.hWnd = hWnd;
			nid.uID = ID_TRAY;
			nid.uVersion = NOTIFYICON_VERSION_4;
			Shell_NotifyIcon(NIM_DELETE, &nid);

			PostQuitMessage(0);
		}
		break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

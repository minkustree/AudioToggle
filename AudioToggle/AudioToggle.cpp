// AudioToggle.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AudioToggle.h"
#include "PlaybackDeviceToggle.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND g_hWnd;									// Handle to the main window
HICON speakerIcon;
HICON headphoneIcon;
BOOL isHeadphones;

static const UINT NOTIFY_ICON_ID = 1;			// identifies a specific notification icon
const UINT WMAPP_NOTIFY_EVENT = WM_APP + 1;		// called when something happens in the tray icon


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL				ShowNotificationIcon(HWND hWnd);
BOOL				RemoveNotificationIcon(HWND hWnd);
HRESULT				SetIconAndTooltip(NOTIFYICONDATA * nid);
BOOL				LoadIcons(HINSTANCE hInstance);
BOOL				ShowContextMenu(HWND hwnd, POINT pt);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

 
	InitCOM();

	// Determine if we're headphones or not
	LPWSTR deviceId;
	GetDefaultAudioPlaybackDevice(&deviceId);
	isHeadphones = (wcscmp(deviceId, szHeadphoneDeviceId) == 0);
	CoTaskMemFree(deviceId);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AUDIOTOGGLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	// Pre-load icons
	LoadIcons(hInstance);
	
    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AUDIOTOGGLE));

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
	
	CleanupCOM();
    return (int) msg.wParam;
}

BOOL ShowNotificationIcon(HWND hWnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uID = NOTIFY_ICON_ID;
	nid.uFlags |= NIF_MESSAGE;
	nid.uCallbackMessage = WMAPP_NOTIFY_EVENT;
	nid.uVersion = NOTIFYICON_VERSION_4;

	if FAILED(	SetIconAndTooltip(&nid)					) return FALSE;
	if		 (!	Shell_NotifyIcon(NIM_ADD, &nid)			) return FALSE;
	if       (!	Shell_NotifyIcon(NIM_SETVERSION, &nid)	) return FALSE;
	return TRUE;
}

HRESULT SetIconAndTooltip(NOTIFYICONDATA *pNid)
{
	HRESULT hr;

	LPCWSTR szDeviceId = isHeadphones ? szHeadphoneDeviceId : szSpeakerDeviceId;
	pNid->uFlags |= NIF_TIP | NIF_SHOWTIP | NIF_ICON;
	pNid->hIcon = isHeadphones ? headphoneIcon : speakerIcon;

	LPWSTR friendlyName;
	hr = GetFriendlyName(szDeviceId, &friendlyName);
	if SUCCEEDED(hr) {
		hr = StringCchCopy(pNid->szTip, ARRAYSIZE(pNid->szTip), friendlyName);
		CoTaskMemFree(friendlyName);
	} 
	else
	{
		hr = StringCchCopy(pNid->szTip, ARRAYSIZE(pNid->szTip), L"Unknown device");
	}
	return hr;
}

BOOL RemoveNotificationIcon(HWND hWnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uID = NOTIFY_ICON_ID;
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

HRESULT ToggleDefaultDevice()
{
	HRESULT hr;

	// device to switch to: if we're headphones, we switch to speakers
	LPCWSTR szNextDeviceId = isHeadphones ? szSpeakerDeviceId : szHeadphoneDeviceId;
	hr = SetDefaultAudioPlaybackDevice(szNextDeviceId);
	if SUCCEEDED(hr)
	{
		// toggle the boolean which tracks default device
		isHeadphones = !isHeadphones;
	}
	return hr;
}

HRESULT UpdateNotificationIcon() {
	HRESULT hr;
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = g_hWnd;
	nid.uID = NOTIFY_ICON_ID;
	hr = SetIconAndTooltip(&nid);
	if SUCCEEDED(hr)
	{
		hr = Shell_NotifyIcon(NIM_MODIFY, &nid) ? S_OK : E_FAIL;
	}
	return hr;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AUDIOTOGGLE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AUDIOTOGGLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
   g_hWnd = hWnd; // store window handle to identify our notification icon
   if (!hWnd)
   {
      return FALSE;
   }

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		// add the notification icon
		ShowNotificationIcon(hWnd);
		break;
    case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_CONTEXT_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
        break;
    case WM_DESTROY:
		RemoveNotificationIcon(hWnd);
		PostQuitMessage(0);
		break;
	case WMAPP_NOTIFY_EVENT:
		switch (LOWORD(lParam))
		{
		case NIN_SELECT:
			ToggleDefaultDevice();
			// We update icon in response to callback from Windows on default device change
			break;
		case WM_CONTEXTMENU:
		{
			POINT pt = { GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam) };
			ShowContextMenu(hWnd, pt);
			break;
		}
		}
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

BOOL ShowContextMenu(HWND hwnd, POINT pt)
{
	HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXTMENU));
	if (hMenu)
	{
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
			SetForegroundWindow(hwnd);

			// respect menu drop alignment
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			{
				uFlags |= TPM_RIGHTALIGN;
			}
			else
			{
				uFlags |= TPM_LEFTALIGN;
			}

			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
		}
		else
		{
			return FALSE;
		}
		DestroyMenu(hMenu);
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

BOOL LoadIcons(HINSTANCE hInstance)
{
	HMODULE hmodMmres = LoadLibrary(TEXT("mmres"));
	if (hmodMmres)
	{
		speakerIcon = static_cast<HICON>(LoadImage(hmodMmres,
			MAKEINTRESOURCE(3010),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_SHARED));
		headphoneIcon = static_cast<HICON>(LoadImage(hmodMmres,
			MAKEINTRESOURCE(3015),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_SHARED));
		FreeLibrary(hmodMmres);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


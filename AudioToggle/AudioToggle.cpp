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
HICON speakerIcon;
HICON headphoneIcon;
BOOL isHeadphones;
HMENU g_contextMenu;

static const UINT NOTIFY_ICON_ID = 1;			// identifies a specific notification icon
const UINT WMAPP_NOTIFY_EVENT = WM_APP + 1;		// called when something happens in the tray icon

LPCWSTR szSpeakerDeviceId = L"{0.0.0.00000000}.{20fc265e-1269-47e7-8718-377317faa951}";
LPCWSTR szHeadphoneDeviceId = L"{0.0.0.00000000}.{6764b0e9-deec-4b03-9c33-dc163bc41f15}";

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HRESULT				ShowNotificationIcon(HWND hWnd);
HRESULT				RemoveNotificationIcon(HWND hWnd);
void				LoadIcons(HINSTANCE hInstance);
void				FreeIcons();
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

    return (int) msg.wParam;
}

HRESULT ShowNotificationIcon(HWND hWnd)
{
	HRESULT hr;
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uID = NOTIFY_ICON_ID;
	nid.uFlags |= NIF_MESSAGE;
	nid.uCallbackMessage = WMAPP_NOTIFY_EVENT;
	nid.uVersion = NOTIFYICON_VERSION_4;
	
	hr = SetIconAndTooltip(&nid);
	if SUCCEEDED(hr)
	{
		hr = Shell_NotifyIcon(NIM_ADD, &nid);
	}
	if SUCCEEDED(hr) 
	{
		hr = Shell_NotifyIcon(NIM_SETVERSION, &nid);
	}
	return hr;
}

HRESULT SetIconAndTooltip(NOTIFYICONDATA *pNid)
{
	HRESULT hr;
	pNid->uFlags |= NIF_TIP | NIF_SHOWTIP | NIF_ICON;
	if (isHeadphones)
	{
		hr = SetDefaultAudioPlaybackDevice(szSpeakerDeviceId);
		if FAILED(hr) return hr;
		StringCchCopy(pNid->szTip, ARRAYSIZE(pNid->szTip), TEXT("Speakers"));
		pNid->hIcon = speakerIcon;
	}
	else
	{
		hr = SetDefaultAudioPlaybackDevice(szHeadphoneDeviceId);
		if (FAILED(hr)) { return hr; }
		StringCchCopy(pNid->szTip, ARRAYSIZE(pNid->szTip), TEXT("Headphones"));
		pNid->hIcon = headphoneIcon;
	}
}

HRESULT RemoveNotificationIcon(HWND hWnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uID = NOTIFY_ICON_ID;
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

HRESULT ModifyNotificationText(HWND hWnd)
{
	HRESULT hr;
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uID = NOTIFY_ICON_ID;
	SetIconAndTooltip(&nid);
	isHeadphones = !isHeadphones;
	return Shell_NotifyIcon(NIM_MODIFY, &nid);
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

   if (!hWnd)
   {
      return FALSE;
   }

   return TRUE;
}

void InitContextMenu()
{
	

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
			case IDM_EXIT:
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
		DestroyMenu(g_contextMenu);
		CleanupCOM();
		PostQuitMessage(0);
		break;
	case WMAPP_NOTIFY_EVENT:
		switch (LOWORD(lParam))
		{
		case NIN_SELECT:
			ModifyNotificationText(hWnd);
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

void LoadIcons(HINSTANCE hInstance)
{
	HMODULE mmres = LoadLibrary(TEXT("mmres"));
	speakerIcon = static_cast<HICON>(LoadImage(mmres, MAKEINTRESOURCE(3010), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
	headphoneIcon = static_cast<HICON>(LoadImage(mmres, MAKEINTRESOURCE(3015), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
	// not sure if I have to free this libary here or not, or whether it renders the icons invalid?
	FreeLibrary(mmres);
}

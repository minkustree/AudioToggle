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
BOOL isAltIcon;

static const UINT NOTIFY_ICON_ID = 1;			// identifies a specific notification icon
const UINT WMAPP_NOTIFY_EVENT = WM_APP + 1;		// called when something happens in the tray icon

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HRESULT				ShowNotificationIcon(HWND hWnd);
HRESULT				RemoveNotificationIcon(HWND hWnd);
void				LoadIcons(HINSTANCE hInstance);
void				FreeIcons();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	InitCOM();
	LPWSTR deviceId;
	GetDefaultAudioPlaybackDevice(&deviceId);
	CoTaskMemFree(deviceId);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AUDIOTOGGLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	// Pre-load icons
	LoadIcons(hInstance);
	isAltIcon = FALSE;

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

	NOTIFYICONDATA nid = { sizeof(nid) };
		
	nid.hWnd = hWnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP |  NIF_MESSAGE;
	nid.uID = NOTIFY_ICON_ID;
	nid.hIcon = speakerIcon;
	nid.uCallbackMessage = WMAPP_NOTIFY_EVENT;
	StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), TEXT("Andy's Tooltip"));
	Shell_NotifyIcon(NIM_ADD, &nid);

	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
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
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_ICON;
	nid.hWnd = hWnd;
	nid.uID = NOTIFY_ICON_ID;
	
	if (isAltIcon)
	{
		StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), TEXT("Speakers"));
		nid.hIcon = speakerIcon;
	}
	else
	{
		StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), TEXT("Headphones"));
		nid.hIcon = headphoneIcon;
	}
	isAltIcon = !isAltIcon;
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

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

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
		{
			// add the notification icon
			ShowNotificationIcon(hWnd);
			break;
		}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
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
    case WM_DESTROY:
		{
			RemoveNotificationIcon(hWnd);
			FreeIcons();
			CleanupCOM();
			PostQuitMessage(0);
		}
		break;
	case WMAPP_NOTIFY_EVENT:
		{
		switch (LOWORD(lParam)) {
		case NIN_SELECT:
			{
				ModifyNotificationText(hWnd);
				break;
			}
		}
		break;
	}
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void LoadIcons(HINSTANCE hInstance)
{
	HMODULE mmres = LoadLibrary(TEXT("mmres"));
	speakerIcon = static_cast<HICON>(LoadImage(mmres, MAKEINTRESOURCE(3010), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
	headphoneIcon = static_cast<HICON>(LoadImage(mmres, MAKEINTRESOURCE(3015), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
	// not sure if I have to free this libary here or not, or whether it renders the icons invalid?
	FreeLibrary(mmres);
}

void FreeIcons() 
{
	//if (speakerIcon) DestroyIcon(speakerIcon);
	//if (headphoneIcon) DestroyIcon(headphoneIcon);
}
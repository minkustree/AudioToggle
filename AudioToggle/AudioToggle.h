#pragma once

#include "resource.h"
#include <map>


struct AudioDeviceInfo
{
	UINT    uSequence;
	LPWSTR	pszId;
	LPWSTR	pszFriendlyName;
	HICON	hIcon;
	HBITMAP hBitmap;
};

extern std::map<unsigned int, AudioDeviceInfo> g_vDeviceInfo;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL				ShowNotificationIcon(HWND hWnd);
BOOL				RemoveNotificationIcon(HWND hWnd);
HRESULT				UpdateNotificationIcon();

HRESULT				SetIconAndTooltip(NOTIFYICONDATA * nid);

BOOL				ShowContextMenu(HWND hwnd, POINT pt);
void				FreeDeviceInfo();

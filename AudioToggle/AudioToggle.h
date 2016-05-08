#pragma once

#include "resource.h"
#include <map>

// Opaque audio device IDs for the two devices I'm interested in: Speakers and Optical Out (Headphones)

static LPCWSTR szSpeakerDeviceId = L"{0.0.0.00000000}.{20fc265e-1269-47e7-8718-377317faa951}";
static LPCWSTR szHeadphoneDeviceId = L"{0.0.0.00000000}.{6764b0e9-deec-4b03-9c33-dc163bc41f15}";

extern BOOL isHeadphones;

struct AudioDeviceInfo
{
	UINT    uSequence;
	LPWSTR	pszId;
	LPWSTR	pszFriendlyName;
	HICON	hIcon;
	HBITMAP hBitmap;
};


extern std::map<unsigned int, AudioDeviceInfo> g_vDeviceInfo;


HRESULT UpdateNotificationIcon();
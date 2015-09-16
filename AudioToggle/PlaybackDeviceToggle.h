#pragma once
#include <Windows.h>

HRESULT InitCOM();
HRESULT SetDefaultAudioPlaybackDevice(_In_ LPCWSTR devId);
HRESULT GetDefaultAudioPlaybackDevice(_Outptr_ LPWSTR *ppstrId);
void	CleanupCOM();
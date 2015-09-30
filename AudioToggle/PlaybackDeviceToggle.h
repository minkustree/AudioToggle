#pragma once
#include <Windows.h>
#include <Mmdeviceapi.h>
#include "AudioToggle.h"



HRESULT InitCOM();
HRESULT SetDefaultAudioPlaybackDevice(_In_ LPCWSTR devId);
HRESULT GetDefaultAudioPlaybackDevice(_Outptr_ LPWSTR *ppstrId);
HRESULT GetFriendlyName(_In_ LPCWSTR devId, _Out_ LPWSTR *pwszFriendlyName);
HRESULT GetDeviceInfo(IMMDevice *pDevice, AudioDeviceInfo *info);
HRESULT EnumerateDevices();
void	LoadDeviceIcon(_Inout_ LPWSTR pszIconPath, _Out_ HICON *phIcon);
void	CleanupCOM();
#pragma once
#include <Windows.h>
#include <Mmdeviceapi.h>

HRESULT InitCOM();
HRESULT SetDefaultAudioPlaybackDevice(_In_ LPCWSTR devId);
HRESULT GetDefaultAudioPlaybackDevice(_Outptr_ LPWSTR *ppstrId);
HRESULT GetFriendlyName(_In_ LPCWSTR devId, _Out_ LPWSTR *pwszFriendlyName);
HRESULT GetDeviceInfo(IMMDevice *pDevice);
HRESULT EnumerateDevices();
HICON   LoadDeviceIcon(_Inout_ LPWSTR pszIconPath);
void	CleanupCOM();
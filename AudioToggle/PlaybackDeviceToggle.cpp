#include "stdafx.h"
#include "PlaybackDeviceToggle.h"
#include "AudioToggle.h"
#include "IPolicyConfig.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <vector>


// helps us release COM objects cleanly
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

class CMMNotificationClient : public IMMNotificationClient
{
	LONG _cRef;

public:
	CMMNotificationClient() : _cRef(1)
	{
	}

	~CMMNotificationClient()
	{
	}

	// IUnknown methods

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		ULONG ulRef = InterlockedDecrement(&_cRef);
		if (ulRef == 0)
		{
			delete this;
		}
		return ulRef;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface)
	{
		if (IID_IUnknown == riid)
		{
			AddRef();
			*ppvInterface = (IUnknown*)this;
		}
		else if (__uuidof(IMMNotificationClient) == riid) {
			AddRef();
			*ppvInterface = (IMMNotificationClient*)this;
		}
		else
		{
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	// Callback methods for device-event notifications

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
		_In_ EDataFlow flow,
		_In_ ERole     role,
		_In_ LPCWSTR   pwstrDefaultDevice
		) {
		if (eRender == flow && eConsole == role) {
			// is the new device our headphone device?
			isHeadphones = (wcscmp(pwstrDefaultDevice, szHeadphoneDeviceId) == 0);
			// update the notification icon
			UpdateNotificationIcon();
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) {
		return S_OK;
	}


};

// global variables
IMMDeviceEnumerator *g_pDeviceEnumerator;
CMMNotificationClient *g_pNotificationClient;

// Set up COM
HRESULT InitCOM()
{
	HRESULT hr;
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if SUCCEEDED(hr) 
	{
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&g_pDeviceEnumerator));
		g_pNotificationClient = new CMMNotificationClient;
	}
	if SUCCEEDED(hr) {
		hr = g_pDeviceEnumerator->RegisterEndpointNotificationCallback(g_pNotificationClient);
	}
	return hr;
}

HRESULT SetDefaultAudioPlaybackDevice(_In_ LPCWSTR devId)
{
	IPolicyConfigVista *pPolicyConfig;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pPolicyConfig));
	if (SUCCEEDED(hr)) 
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devId, eConsole);
	}
	if SUCCEEDED(hr)
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devId, eMultimedia);
	}
	if SUCCEEDED(hr)
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devId, eCommunications);
	}
	SafeRelease(&pPolicyConfig);

	return hr;
}


HRESULT EnumerateDevices()
{
	HRESULT hr;
	IMMDeviceCollection *pDevices;
	UINT deviceCount;
	hr = g_pDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
	if SUCCEEDED(hr)
	{
		hr = pDevices->GetCount(&deviceCount);
	}
	if (SUCCEEDED(hr))
	{
		g_vDeviceInfo.clear();
		for (UINT i = 0; i < deviceCount; i++) 
		{
			IMMDevice *pDevice;
			hr = pDevices->Item(i, &pDevice);
			AudioDeviceInfo info;
			info.uSequence = 3004 + i;
			GetDeviceInfo(pDevice, &info);
			g_vDeviceInfo.emplace(3004 + i, info); // creates a copy
			SafeRelease(&pDevice);
		}
		
	}
	SafeRelease(&pDevices);
	return hr;
}



HRESULT GetDeviceInfo(IMMDevice *pDevice, AudioDeviceInfo *info)
{
	HRESULT hr;
	IPropertyStore *pProperties = nullptr;
	PROPVARIANT propVar;

	PropVariantInit(&propVar);

	// id
	hr = pDevice->GetId(&(info->pszId)); // free with CoTaskMemFree
	if SUCCEEDED(hr)
	{
		hr = pDevice->OpenPropertyStore(STGM_READ, &pProperties);
	}

	// friendly name
	if SUCCEEDED(hr)
	{
		
		hr = pProperties->GetValue(PKEY_Device_FriendlyName, &propVar);
	}
	if SUCCEEDED(hr)
	{
		hr = PropVariantToStringAlloc(propVar, &(info->pszFriendlyName)); // free with CoTaskMemFree
	}
	
	// icon
	if SUCCEEDED(hr)
	{
		PropVariantClear(&propVar);
		hr = pProperties->GetValue(PKEY_DeviceClass_IconPath, &propVar);
		LoadDeviceIcon(propVar.pwszVal, &info->hIcon); // free with DestroyIcon
		IconToBitmap(info->hIcon, &(info->hBitmap)); // free with DeleteObject
	}

	PropVariantClear(&propVar);
	SafeRelease(&pProperties);
	return hr;
}

// Supplied icon path will be truncated after calling. Icon must be destroyed after use with DestroyIcon
void LoadDeviceIcon(_Inout_ LPWSTR pszIconPath, _Out_ HICON *phIcon) 
{
	// Destructive split of pszIconPath into filename,resourceId components
	int resourceId = PathParseIconLocation(pszIconPath);
	UINT cExtracted = ExtractIconEx(pszIconPath, resourceId, NULL, phIcon, 1);
}

// Creates a HBITMAP - free with DeleteObject after use
void IconToBitmap(_In_ HICON hIcon, _Inout_ HBITMAP *phBitmap) 
{
	// convert the icon to a bitmap
	// 1. Create a bitmap with the same format as the screen
	HDC screenDC = GetDC(NULL);
	int x_sm = GetSystemMetrics(SM_CXSMICON);
	int y_sm = GetSystemMetrics(SM_CYSMICON);
	*phBitmap = CreateCompatibleBitmap(screenDC, x_sm, y_sm);
	HDC drawingDC = CreateCompatibleDC(screenDC); // for drawing into the bitmap
	

	// Draw the icon into the dc (which is backed by the bitmap)
	HBITMAP originalBitmap = (HBITMAP)SelectObject(drawingDC, *phBitmap);
	DrawIconEx(drawingDC, 0, 0, hIcon, x_sm, y_sm, 0, GetSysColorBrush(COLOR_MENU), DI_NORMAL); 

	// we're supposed to replace the bitmap in the DC with the original when we're done
	SelectObject(drawingDC, originalBitmap);

	// clean up
	DeleteDC(drawingDC);
	ReleaseDC(NULL, screenDC);
}

HRESULT GetDeviceIcon(_In_ LPCWSTR devId, _Out_ HICON *phIcon)
{
	HRESULT hr;

	IMMDevice *pDevice = nullptr;
	IPropertyStore *pPropStore = nullptr;

	hr = g_pDeviceEnumerator->GetDevice(devId, &pDevice);
	if (SUCCEEDED(hr))
	{
		hr = pDevice->OpenPropertyStore(STGM_READ, &pPropStore);
	}
	if (SUCCEEDED(hr))
	{
		PROPVARIANT varIcon;
		PropVariantInit(&varIcon);
		hr = pPropStore->GetValue(PKEY_DeviceClass_IconPath, &varIcon);
		if (SUCCEEDED(hr))
		{
			LoadDeviceIcon(varIcon.pwszVal, phIcon); // free with DestroyIcon
		}
		PropVariantClear(&varIcon);
	}

	SafeRelease(&pPropStore);
	SafeRelease(&pDevice);
	return hr;
}

// Caller must free pwszFriendlyName with CoTaskMemFree when done
HRESULT GetFriendlyName(_In_ LPCWSTR devId, _Out_ LPWSTR * pwszFriendlyName)
{
	HRESULT hr;
	
	IMMDevice *pDevice = nullptr;
	IPropertyStore *pPropStore = nullptr;

	hr = g_pDeviceEnumerator->GetDevice(devId, &pDevice);
	if (SUCCEEDED(hr))
	{
		hr = pDevice->OpenPropertyStore(STGM_READ, &pPropStore);
	}
	if (SUCCEEDED(hr))
	{
		PROPVARIANT varName;
		PropVariantInit(&varName);
		hr = pPropStore->GetValue(PKEY_Device_FriendlyName, &varName);
		if (SUCCEEDED(hr)) 
		{
			hr = PropVariantToStringAlloc(varName, pwszFriendlyName);
		}
		PropVariantClear(&varName);
	}
	
	SafeRelease(&pPropStore);
	SafeRelease(&pDevice);
	return hr;
}


// ppstrId must be freed by CoTaskMemFree
HRESULT GetDefaultAudioPlaybackDevice(_Outptr_ LPWSTR *ppstrId)
{
	HRESULT hr;
	IMMDeviceEnumerator *pDeviceEnumerator = nullptr;
	IMMDevice *pDevice = nullptr;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDeviceEnumerator));
	if (SUCCEEDED(hr))
	{
		hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	}
	if (SUCCEEDED(hr))
	{
		hr = pDevice->GetId(ppstrId);
	}
	SafeRelease(&pDevice);
	SafeRelease(&pDeviceEnumerator);
	return hr;
}

void CleanupCOM()
{
	if (g_pNotificationClient && g_pDeviceEnumerator)
	{
		g_pDeviceEnumerator->UnregisterEndpointNotificationCallback(g_pNotificationClient);
	}
	SafeRelease(&g_pNotificationClient);
	SafeRelease(&g_pDeviceEnumerator);
	CoUninitialize();
}


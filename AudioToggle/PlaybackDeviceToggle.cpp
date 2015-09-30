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

HRESULT EnumerateDevices(/*std::vector<deviceMenuInfo> vDevices*/)
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
		for (UINT i = 0; i < deviceCount; i++) 
		{
			IMMDevice *pDevice;
			hr = pDevices->Item(i, &pDevice);
			GetDeviceInfo(pDevice);
			SafeRelease(&pDevice);
		}
		
	}
	SafeRelease(&pDevices);
	return hr;
}

HRESULT GetDeviceInfo(IMMDevice *pDevice)
{
	HRESULT hr;
	IPropertyStore *pProperties = nullptr;
	PROPVARIANT propVar;

	LPWSTR pstrId;
	LPWSTR pstrFriendlyName;
	HICON hIcon;

	hr = pDevice->GetId(&pstrId); // free with CoTaskMemFree
	if SUCCEEDED(hr)
	{
		hr = pDevice->OpenPropertyStore(STGM_READ, &pProperties);
	}

	// friendly name
	if SUCCEEDED(hr)
	{
		PropVariantInit(&propVar);
		hr = pProperties->GetValue(PKEY_Device_DeviceDesc, &propVar);
	}
	if SUCCEEDED(hr)
	{
		hr = PropVariantToStringAlloc(propVar, &pstrFriendlyName);
	}
	
	// icon
	if SUCCEEDED(hr)
	{
		PropVariantClear(&propVar);
		hr = pProperties->GetValue(PKEY_DeviceClass_IconPath, &propVar);
		hIcon = LoadDeviceIcon(propVar.pwszVal);
	}

	PropVariantClear(&propVar);
	SafeRelease(&pProperties);
	return hr;
}

// Supplied icon path will be truncated after calling
HICON LoadDeviceIcon(_Inout_ LPWSTR pszIconPath) 
{
	// TODO: Split pszIconPath into filename,resourceId components
	HICON result = NULL;
	int resource = PathParseIconLocation(pszIconPath);
	HMODULE hMod = LoadLibraryEx(pszIconPath, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (hMod)
	{
		result = (HICON)LoadImage(hMod, MAKEINTRESOURCE(resource), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
		FreeLibrary(hMod);
	} 
	else
	{
		DWORD errorId = GetLastError();
		if (errorId == 0)
		{
			printf("Error %d", errorId);
		}
	}
	return result;
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
		hr = pPropStore->GetValue(PKEY_Device_DeviceDesc, &varName);
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

//
//
//void PrintPropertyStore(IPropertyStore *pPropertyStore)
//{
//	
//	DWORD cProps;
//	pPropertyStore->GetCount(&cProps);
//
//
//	for (DWORD i = 0; i < cProps; i++) {
//		PROPERTYKEY key;
//		pPropertyStore->GetAt(i, &key);
//
//		IPropertyDescription *propDescription = NULL;
//		HRESULT hr = PSGetPropertyDescription(key, IID_PPV_ARGS(&propDescription));
//		
//		if (SUCCEEDED(hr)) 
//		{
//			LPWSTR propertyName = NULL;
//			if (SUCCEEDED(propDescription->GetDisplayName(&propertyName)))
//			{
//				std::wcout << "  " << propertyName << std::endl;
//				CoTaskMemFree(propertyName);
//			}
//			propDescription->Release();
//		} 
//		else
//		{
//			switch (hr)
//			{
//			case E_INVALIDARG: {
//				std::cout << "The ppv parameter is NULL" << std::endl;
//				break;
//			}
//			case TYPE_E_ELEMENTNOTFOUND: {
//				std::cout << "The PROPERTYKEY does not exist in the schema subsystem cache" << std::endl;
//				break;
//			}
//			default: {
//				std::cout << "Unknown result code: " << hr << std::endl;
//			}
//			}
//		}
//	}
//}
//




//HRESULT PrintDeviceInfo(IMMDevice *pDevice) 
//{
//	HRESULT hr = S_OK;
//	LPWSTR pszId = NULL;
//	IPropertyStore *pPropertyStore = NULL;
//
//	hr = pDevice->GetId(&pszId);
//	if FAILED(hr) goto done;
//
//	hr = pDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
//	if FAILED(hr) goto done;
//
//	PROPVARIANT friendlyName;
//	PropVariantInit(&friendlyName);
//	hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
//	if FAILED(hr) goto done;
//
//	PROPVARIANT deviceFriendlyName;
//	PropVariantInit(&deviceFriendlyName);
//	hr = pPropertyStore->GetValue(PKEY_DeviceInterface_FriendlyName, &deviceFriendlyName);
//	if FAILED(hr) goto done;
//
//	PROPVARIANT formFactor;
//	PropVariantInit(&formFactor);
//	hr = pPropertyStore->GetValue(PKEY_AudioEndpoint_FormFactor, &formFactor);
//	if FAILED(hr) goto done;
//
//	std::wcout << "Device " << pszId << ": " << friendlyName.pwszVal << " on " << deviceFriendlyName.pwszVal << ". Form factor: " << formFactor.uintVal << std::endl;
//
//	done:
//		PropVariantClear(&formFactor);
//		PropVariantClear(&deviceFriendlyName);
//		PropVariantClear(&friendlyName);
//		if (pszId != NULL) CoTaskMemFree(pszId);
//		SafeRelease(&pPropertyStore);
//		return hr;
//}

//int main()
//{
//	HRESULT hr;
//
//	if FAILED(InitCOM()) { CleanupAndExit(1); }
//	if FAILED(PrintDefaultDevice()) { CleanupAndExit(2); }
//
//	int rv = 0;
//	IPolicyConfig *pPolicyConfig = NULL;
//	
//	IMMDeviceCollection *pDeviceCollection = NULL;
//	
//	if (FAILED(pDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDeviceCollection))) {
//		std::cout << "Error enumerating audio endpoints" << std::endl;
//		rv = 3;
//		goto cleanup;
//	}
//	
//	UINT count;
//	pDeviceCollection->GetCount(&count);
//	for (UINT i = 0; i < count; i++) {
//		IMMDevice *pDevice = NULL;
//		if (FAILED(pDeviceCollection->Item(i, &pDevice))) {
//			std::cout << "Failed to get details for device " << i << ". Skipping." << std::endl;
//			continue;
//		}
//		
//		PrintDeviceInfo(pDevice);
//		SafeRelease(&pDevice);
//		
//	}
//
//	std::cout << "Success" << std::endl;
//	
//	cleanup:
//		std::cout << "Press any key to exit" << std::endl;
//		std::cin.ignore();
//		if (pDeviceCollection != NULL) { pDeviceCollection->Release(); }
//		if (pDeviceEnumerator != NULL) { pDeviceEnumerator->Release(); }
//		exit(rv);
//}


#include "stdafx.h"
#include "PlaybackDeviceToggle.h"
#include "IPolicyConfig.h"

// helps us release COM objects cleanly
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

// Set up COM
HRESULT InitCOM()
{
	HRESULT hr;
	hr = CoInitialize(nullptr);
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
	SafeRelease(&pPolicyConfig);

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


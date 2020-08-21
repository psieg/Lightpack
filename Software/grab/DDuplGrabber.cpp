/*
* DDGrabber.cpp
*
*	Created on: 21.12.2018
*		Project: Lightpack
*
*	Copyright (c) 2014 Patrick Siegler
*
*	Lightpack a USB content-driving ambient lighting system
*
*	Lightpack is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 2 of the License, or
*	(at your option) any later version.
*
*	Lightpack is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.	If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "DDuplGrabber.hpp"
#include "SystemSession.hpp"
#include "debug.h"

#ifdef DDUPL_GRAB_SUPPORT
#include <comdef.h>
#include <dxgi1_2.h>
#include <d3d11.h>
_COM_SMARTPTR_TYPEDEF(IDXGIFactory1, __uuidof(IDXGIFactory1));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput, __uuidof(IDXGIOutput));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput1, __uuidof(IDXGIOutput1));
_COM_SMARTPTR_TYPEDEF(IDXGIOutputDuplication, __uuidof(IDXGIOutputDuplication));
_COM_SMARTPTR_TYPEDEF(IDXGIResource, __uuidof(IDXGIResource));
_COM_SMARTPTR_TYPEDEF(IDXGISurface1, __uuidof(IDXGISurface1));
_COM_SMARTPTR_TYPEDEF(ID3D11Device, __uuidof(ID3D11Device));
_COM_SMARTPTR_TYPEDEF(ID3D11DeviceContext, __uuidof(ID3D11DeviceContext));
_COM_SMARTPTR_TYPEDEF(ID3D11Texture2D, __uuidof(ID3D11Texture2D));
_COM_SMARTPTR_TYPEDEF(ID3D11ShaderResourceView, __uuidof(ID3D11ShaderResourceView));


typedef HRESULT(WINAPI *CreateDXGIFactory1Func)(REFIID riid, _Out_ void **ppFactory);
typedef HRESULT(WINAPI *D3D11CreateDeviceFunc)(
	_In_opt_ IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	_In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	_Out_opt_ ID3D11Device** ppDevice,
	_Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
	_Out_opt_ ID3D11DeviceContext** ppImmediateContext);

#define ACQUIRE_TIMEOUT_INTERVAL 0 // timing is done via the timer freqency, so we don't wait again
#define ACCESSDENIED_DESKTOP_RETRY_INTERVAL 1000
#define ACCESSDENIED_DUPLICATION_RETRY_INTERVAL 5000
#define THREAD_DESTRUCTION_WAIT_TIMEOUT 3000

#define DDUPL_THREAD_EVENT_NAME L"Lightpack.DDuplGrabber.Event.Thread"
#define DDUPL_THREADRETURN_EVENT_NAME L"Lightpack.DDuplGrabber.Event.ThreadReturn"

namespace {
	// 0 = /1 (no scaling)
	// 1 = /2
	// 2 = /4
	// 3 = /8 (best value for now)
	// 4+ seems to be counter-productive
	static const uint8_t DownscaleMipLevel = 3;
}

struct DDuplScreenData
{
	DDuplScreenData(IDXGIOutputPtr _output, IDXGIOutputDuplicationPtr _duplication, ID3D11DevicePtr _device, ID3D11DeviceContextPtr _context)
		: output(_output), duplication(_duplication), device(_device), context(_context)
	{}

	IDXGIOutputPtr output{nullptr};
	IDXGIOutputDuplicationPtr duplication{nullptr};
	ID3D11DevicePtr device{nullptr};
	ID3D11DeviceContextPtr context{nullptr};

	ID3D11Texture2DPtr textureCopy{nullptr};
	DXGI_MAPPED_RECT surfaceMap;
};

DDuplGrabber::DDuplGrabber(QObject * parent, GrabberContext *context)
	: GrabberBase(parent, context),
	m_state(Uninitialized),
	m_accessDeniedLastCheck(0),
	m_dxgiDll(NULL),
	m_d3d11Dll(NULL),
	m_createDXGIFactory1Func(NULL),
	m_D3D11CreateDeviceFunc(NULL),
	m_thread(NULL),
	m_threadEvent(NULL),
	m_threadReturnEvent(NULL),
	m_threadReallocateArg(),
	m_threadReallocateResult(false),
	m_isSessionLocked(false)
{
}

DDuplGrabber::~DDuplGrabber()
{
	freeScreens();

	if (m_thread) {
		m_threadCommand = Exit;
		runThreadCommand(THREAD_DESTRUCTION_WAIT_TIMEOUT);
	}

	// release adapters before unloading libraries
	m_adapters.clear();

	if (m_threadEvent)
		CloseHandle(m_threadEvent);
	if (m_threadReturnEvent)
		CloseHandle(m_threadReturnEvent);

	if (m_dxgiDll)
		FreeLibrary(m_dxgiDll);
	if (m_d3d11Dll)
		FreeLibrary(m_d3d11Dll);
}

bool DDuplGrabber::init()
{
	m_state = Unavailable;

	m_dxgiDll = LoadLibrary(L"dxgi.dll");
	m_d3d11Dll = LoadLibrary(L"d3d11.dll");
	if (!m_dxgiDll || !m_d3d11Dll)
		return false;

	m_createDXGIFactory1Func = GetProcAddress(m_dxgiDll, "CreateDXGIFactory1");
	m_D3D11CreateDeviceFunc = GetProcAddress(m_d3d11Dll, "D3D11CreateDevice");
	if (!m_createDXGIFactory1Func || !m_D3D11CreateDeviceFunc)
		return false;

	if (!recreateAdapters()) {
		return false;
	}

	if (NULL == (m_threadEvent = CreateEventW(NULL, false, false, DDUPL_THREAD_EVENT_NAME))) {
		qCritical(Q_FUNC_INFO " unable to create threadEvent");
		return false;
	}

	if (NULL == (m_threadReturnEvent = CreateEventW(NULL, false, false, DDUPL_THREADRETURN_EVENT_NAME))) {
		qCritical(Q_FUNC_INFO " unable to create threadReturnEvent");
		return false;
	}

	if (NULL == (m_thread = CreateThread(NULL, 0, DDuplGrabberThreadProc, this, 0, NULL))) {
		qCritical(Q_FUNC_INFO " unable to create thread");
		return false;
	}

	m_state = Ready;
	return true;
}

bool DDuplGrabber::recreateAdapters() {
	m_adapters.clear();

	IDXGIFactory1Ptr factory;
	HRESULT hr = ((CreateDXGIFactory1Func)m_createDXGIFactory1Func)(__uuidof(IDXGIFactory1), (void**)&factory);
	if (FAILED(hr))
	{
		qCritical(Q_FUNC_INFO " Failed to CreateDXGIFactory1: 0x%X", hr);
		return false;
	}

	IDXGIAdapter1Ptr adapter;
	for (int i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Found Adapter: " << QString::fromWCharArray(desc.Description);
		m_adapters.push_back(adapter);
	}

	return true;
}

DWORD WINAPI DDuplGrabberThreadProc(LPVOID arg) {
	DDuplGrabber* _this = (DDuplGrabber*)arg;
	DWORD errorcode;

	while (true) {
		if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(_this->m_threadEvent, INFINITE))) {
			switch (_this->m_threadCommand) {
			case Exit:
				SetEvent(_this->m_threadReturnEvent);
				return 0;
			case Reallocate:
				_this->m_threadReallocateResult = _this->_reallocate(_this->m_threadReallocateArg);
				break;
			}
			SetEvent(_this->m_threadReturnEvent);
		}
	}
}

bool DDuplGrabber::runThreadCommand(DWORD timeout) {
	DWORD errorcode;
	SetEvent(m_threadEvent);

	if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(m_threadReturnEvent, timeout))) {
		return true;
	}
	else {
		qWarning(Q_FUNC_INFO " couldn't execute thread command: %x %x", m_threadCommand, errorcode);
		return false;
	}
}

bool anyWidgetOnThisMonitor(HMONITOR monitor, const QList<GrabWidget *> &grabWidgets)
{
	for (GrabWidget* widget : grabWidgets)
	{
		HMONITOR widgetMonitor = MonitorFromWindow(reinterpret_cast<HWND>(widget->winId()), MONITOR_DEFAULTTONULL);
		if (widgetMonitor == monitor)
		{
			return true;
		}
	}

	return false;
}

QList< ScreenInfo > * DDuplGrabber::screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets)
{
	return __screensWithWidgets(result, grabWidgets);
}

QList< ScreenInfo > * DDuplGrabber::__screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets, bool noRecursion)
{
	result->clear();

	if (m_state == Uninitialized)
	{
		if (!init())
			return result;
	}

	for (IDXGIAdapter1Ptr adapter : m_adapters)
	{
		IDXGIOutputPtr output;
		for (int i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			output->GetDesc(&outputDesc);
			if (outputDesc.Monitor == NULL) {
				if (!noRecursion) {
					qWarning() << Q_FUNC_INFO << "Found a monitor with NULL handle. Recreating adapters";
					recreateAdapters();
					return __screensWithWidgets(result, grabWidgets, true);
				} else {
					qWarning() << Q_FUNC_INFO << "Found a monitor with NULL handle (after recreation)";
					continue;
				}
			}

			if (anyWidgetOnThisMonitor(outputDesc.Monitor, grabWidgets))
			{
				ScreenInfo screenInfo;
				screenInfo.rect = QRect(
					outputDesc.DesktopCoordinates.left,
					outputDesc.DesktopCoordinates.top,
					outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left,
					outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top);
				screenInfo.handle = outputDesc.Monitor;

				result->append(screenInfo);
			}
		}
	}
	return result;
}

void DDuplGrabber::onSessionChange(int change)
{
	if (change == SystemSession::Status::Locking)
	{
		m_isSessionLocked = true;
	}
	else if (change == SystemSession::Status::Unlocking)
	{
		m_isSessionLocked = false;
	}
}

bool DDuplGrabber::isReallocationNeeded(const QList< ScreenInfo > &grabScreens) const
{
	if (m_state != Allocated)
	{
		if (m_state == AccessDeniedDesktop)
		{
			// retry allocation every ACCESSDENIED_DESKTOP_RETRY_INTERVAL ms in case the user left the secure desktop
			return GetTickCount() - m_accessDeniedLastCheck > ACCESSDENIED_DESKTOP_RETRY_INTERVAL;
		}
		else if (m_state == AccessDeniedDuplication)
		{
			// retry allocation every ACCESSDENIED_DUPLICATION_RETRY_INTERVAL ms in case the 3D app closed
			return GetTickCount() - m_accessDeniedLastCheck > ACCESSDENIED_DUPLICATION_RETRY_INTERVAL;
		}
		else if (m_state == Unavailable)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		return GrabberBase::isReallocationNeeded(grabScreens);
	}
}

void DDuplGrabber::freeScreens()
{
	for (GrabbedScreen& screen : _screensWithWidgets)
	{
		if (screen.imgData != NULL)
		{
			if (((DDuplScreenData*)screen.associatedData)->textureCopy == nullptr)
				free((void*)screen.imgData);// if we don't have a texture we have a manually allocated black frame
			screen.imgData = NULL;
			screen.imgDataSize = 0;
		}

		if (screen.associatedData != NULL)
		{
			delete (DDuplScreenData*)screen.associatedData;
			screen.associatedData = NULL;
		}

		screen.scale = 1.0;
		screen.bytesPerRow = 0;
	}

	_screensWithWidgets.clear();
}

bool DDuplGrabber::reallocate(const QList< ScreenInfo > &grabScreens)
{
	// Reallocate on the dedicated thread to be able to SetThreadDesktop to the currently active input desktop
	// once the duplication is created, it seems like it can be used from the normal thread.
	m_threadCommand = Reallocate;
	m_threadReallocateArg = grabScreens;
	if (runThreadCommand(INFINITE)) {
		return m_threadReallocateResult;
	} else {
		return false;
	}
}

// Must be called from DDuplGrabberThreadProc !
bool DDuplGrabber::_reallocate(const QList< ScreenInfo > &grabScreens, bool noRecursion)
{
	if (m_state == Uninitialized)
	{
		if (!init())
			return false;
	}

	freeScreens();

	HDESK screensaverDesk = OpenInputDesktop(0, true, DESKTOP_SWITCHDESKTOP);
	if (!screensaverDesk) {
		DWORD err = GetLastError();

		if (err == ERROR_ACCESS_DENIED) {
			// fake success, see grabScreens
			m_state = AccessDeniedDesktop;
			m_accessDeniedLastCheck = GetTickCount();
			qWarning(Q_FUNC_INFO " Access to input desktop denied, retry later");
			return true;
		} else {
			qCritical(Q_FUNC_INFO " Failed to open input desktop: %x", GetLastError());
			return true;
		}
	}

	BOOL success = SetThreadDesktop(screensaverDesk);
	CloseHandle(screensaverDesk);
	if (!success) {
		qCritical(Q_FUNC_INFO " Failed to set grab desktop: %x", GetLastError());
		return false;
	}

	for (IDXGIAdapter1Ptr adapter : m_adapters)
	{
		ID3D11DevicePtr device;
		ID3D11DeviceContextPtr context;
		D3D_FEATURE_LEVEL featureLevel;
		HRESULT hr = ((D3D11CreateDeviceFunc)m_D3D11CreateDeviceFunc)(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context);
		if (FAILED(hr))
		{
			qCritical(Q_FUNC_INFO " Failed to create D3D11 device: 0x%X", hr);
			return false;
		}

		IDXGIOutputPtr output;
		for (int i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; i++)
		{
			IDXGIOutput1Ptr output1;
			hr = output->QueryInterface(IID_IDXGIOutput1, (void**)&output1);
			if (FAILED(hr))
			{
				qCritical(Q_FUNC_INFO " Failed to cast output to IDXGIOutput1: 0x%X", hr);
				return false;
			}

			DXGI_OUTPUT_DESC outputDesc;
			output->GetDesc(&outputDesc);
			for (const ScreenInfo& screenInfo : grabScreens)
			{
				if (screenInfo.handle == outputDesc.Monitor)
				{
					IDXGIOutputDuplicationPtr duplication;
					hr = output1->DuplicateOutput(device, &duplication);
					if (hr == E_ACCESSDENIED)
					{
						// fake success, see grabScreens
						m_state = AccessDeniedDuplication;
						m_accessDeniedLastCheck = GetTickCount();
						qWarning(Q_FUNC_INFO " Desktop Duplication not available, access denied, retry later");
						return true;
					}
					else if (hr == E_NOTIMPL || hr == DXGI_ERROR_UNSUPPORTED)
					{
						m_state = Unavailable;
						qCritical(Q_FUNC_INFO " Desktop Duplication not available on this system / in this configuration (desktop 0x%X, 0x%X)", screenInfo.handle, hr);
						return false;
					}
					else if (hr == DXGI_ERROR_MODE_CHANGE_IN_PROGRESS) {
						if (!noRecursion) {
							qWarning(Q_FUNC_INFO " DXGI mode change in progress. Recreating adapters");
							if (!recreateAdapters()) {
								return false;
							}
							return _reallocate(grabScreens, true);
						} else {
							qCritical(Q_FUNC_INFO " Failed to reallocate: DXGI mode change in progress (after recreation)");
							return false;
						}
					}
					else if (FAILED(hr))
					{
						qCritical(Q_FUNC_INFO " Failed to duplicate output: 0x%X", hr);
						return false;
					}

					GrabbedScreen grabScreen;
					grabScreen.imgData = (unsigned char *)NULL;
					grabScreen.imgFormat = BufferFormatArgb;
					grabScreen.screenInfo = screenInfo;
					grabScreen.scale = 1.0;
					switch (outputDesc.Rotation) {
					case DXGI_MODE_ROTATION_ROTATE90:
						grabScreen.rotation = 3; // If the screen is rotated 90, in the screen coordinates the image is rotated 270
						break;
					case DXGI_MODE_ROTATION_ROTATE180:
						grabScreen.rotation = 2;
						break;
					case DXGI_MODE_ROTATION_ROTATE270:
						grabScreen.rotation = 1; // If the screen is rotated 270, in the screen coordinates the image is rotated 90
						break;
					default:
						grabScreen.rotation = 0;
					}
					grabScreen.bytesPerRow = 0;
					grabScreen.associatedData = new DDuplScreenData(output, duplication, device, context);

					_screensWithWidgets.append(grabScreen);

					break;
				}
			}

		}
	}


	for (const ScreenInfo& screenInfo : grabScreens) {
		bool found = false;
		for (const GrabbedScreen& foundScreen : _screensWithWidgets) {
			if (foundScreen.screenInfo == screenInfo) {
				found = true;
				break;
			}
		}
		if (!found) {
			qWarning() << Q_FUNC_INFO << "No matching output was found for screenInfo at" << screenInfo.rect;
		}
	}

	m_state = Allocated;

	return true;
}

BufferFormat mapDXGIFormatToBufferFormat(DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			return BufferFormatArgb;
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			return BufferFormatAbgr;
		default:
			return BufferFormatUnknown;
	}
}

GrabResult DDuplGrabber::returnBlackBuffer()
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO;
	for (GrabbedScreen& screen : _screensWithWidgets)
	{
		DDuplScreenData* screenData = (DDuplScreenData*)screen.associatedData;
		if (screenData->textureCopy) {
			// if we have a texture, imgData points to surfaceMap.pBits
			// so here we only need to free the texture and null everything
			screenData->textureCopy = nullptr;
			screen.imgData = NULL;
			screen.imgDataSize = 0;
		}
		screen.scale = 1.0 / (1 << DownscaleMipLevel);
		screen.bytesPerRow = screen.screenInfo.rect.width() >> DownscaleMipLevel; // doesn't need to be perfectly padded
		const size_t sizeNeeded = (screen.screenInfo.rect.height() >> DownscaleMipLevel) * screen.bytesPerRow;
		if (screen.imgData == NULL)
			screen.imgData = (unsigned char*)malloc(sizeNeeded);
		else if (screen.imgDataSize != sizeNeeded)
		{
			qWarning(Q_FUNC_INFO " Unexpected buffer size %d where %d is expected", screen.imgDataSize, sizeNeeded);
			screen.imgData = (unsigned char*)realloc((void*)screen.imgData, sizeNeeded);
		}
		if (screen.imgData == NULL) {
			qCritical(Q_FUNC_INFO " Failed to allocate black buffer");
			return GrabResultError;
		}

		screen.imgDataSize = sizeNeeded;
		ZeroMemory((void*)screen.imgData, screen.imgDataSize);
	}

	return GrabResultOk;
}

GrabResult DDuplGrabber::grabScreens()
{
	if (m_state != Allocated)
	{
		if (m_state == AccessDeniedDuplication)
		{
			// If access to Desktop Duplication is denied, as far as we know 3D application is running
			// Return black buffers and retry allocation in isReallocationNeeded
			return returnBlackBuffer();
		}
		else if (m_state == AccessDeniedDesktop)
		{
			// If access to the input desktop is denied, as far as we know a secure desktop is active
			// Retry allocation in isReallocationNeeded
			if (m_isSessionLocked)
			{
				// In case of logon screen, keeping the last image will most closely resemble what we've last seen, so GrabResultFrameNotReady is better
				return GrabResultFrameNotReady;
			}
			else
			{
				// In case of UAC prompt, that will at least be what we'll have before and after the prompt, reducing its visual impact
				return returnBlackBuffer();
			}
		}
		else
		{
			return GrabResultFrameNotReady;
		}
	}

	try
	{
		bool anyUpdate = false;
		for (GrabbedScreen& screen : _screensWithWidgets)
		{
			if (screen.associatedData == NULL)
			{
				return GrabResultError;
			}

			DDuplScreenData* screenData = (DDuplScreenData*)screen.associatedData;
			DXGI_OUTDUPL_FRAME_INFO frameInfo;
			IDXGIResourcePtr resource;
			HRESULT hr = screenData->duplication->AcquireNextFrame(ACQUIRE_TIMEOUT_INTERVAL, &frameInfo, &resource);
			if (hr == DXGI_ERROR_WAIT_TIMEOUT)
			{
				// If we have an old image for this screen, we can stick to that, otherwise we have to wait
				if (screen.imgData == NULL)
					return GrabResultFrameNotReady;
				else
					continue;
			}
			else if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL)
			{
				// in theory, DXGI_ERROR_INVALID_CALL is returned if the frame was not released
				// it also happens in conjunction with secure desktop (even though the frame was properly released)
				m_state = LostAccess;
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Lost Access to desktop" << screen.screenInfo.handle << ":" << (void*)(0xffffffff & hr) << ", requesting realloc";
				return GrabResultFrameNotReady;
			}
			else if (FAILED(hr))
			{
				qCritical(Q_FUNC_INFO " Failed to AcquireNextFrame: 0x%X", hr);
				return GrabResultError;
			}
			anyUpdate = true;

			ID3D11Texture2DPtr texture;
			hr = resource->QueryInterface(IID_ID3D11Texture2D, (void**)&texture);
			if (FAILED(hr))
			{
				qCritical(Q_FUNC_INFO " Failed to cast resource to ID3D11Texture2D: 0x%X", hr);
				return GrabResultError;
			}

			D3D11_TEXTURE2D_DESC desc;
			texture->GetDesc(&desc);

			// Expect matching or flipped rotation
			if (screen.rotation % 2 == 0 && (desc.Width != screen.screenInfo.rect.width() || desc.Height != screen.screenInfo.rect.height())
				|| screen.rotation % 2 == 1 && (desc.Height != screen.screenInfo.rect.width() || desc.Width != screen.screenInfo.rect.height()))
			{
				qCritical(Q_FUNC_INFO " Dimension mismatch: screen %d x %d, texture %d x %d",
					screen.screenInfo.rect.width(),
					screen.screenInfo.rect.height(),
					desc.Width,
					desc.Height);
				return GrabResultError;
			}

			D3D11_TEXTURE2D_DESC texDesc;
			ZeroMemory(&texDesc, sizeof(texDesc));
			texDesc.Width = desc.Width >> DownscaleMipLevel;
			texDesc.Height = desc.Height >> DownscaleMipLevel;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 1;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_STAGING;
			texDesc.Format = desc.Format;
			texDesc.BindFlags = 0;
			texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			texDesc.MiscFlags = 0;

			// reset texture and data before getting new one
			if (screenData->textureCopy) {
				screenData->textureCopy = nullptr;
			} else if (screen.imgData) {
				free((void*)screen.imgData);
			}

			screen.imgData = NULL;
			screen.imgDataSize = 0;

			hr = screenData->device->CreateTexture2D(&texDesc, NULL, &screenData->textureCopy);
			if (FAILED(hr))
			{
				qCritical(Q_FUNC_INFO " Failed to CreateTexture2D: 0x%X", hr);
				return GrabResultError;
			}

			if (DownscaleMipLevel > 0) {
				D3D11_TEXTURE2D_DESC scaledTextureDesc;
				ZeroMemory(&scaledTextureDesc, sizeof(scaledTextureDesc));
				scaledTextureDesc.Width = desc.Width;
				scaledTextureDesc.Height = desc.Height;
				scaledTextureDesc.MipLevels = DownscaleMipLevel + 1;
				scaledTextureDesc.ArraySize = 1;
				scaledTextureDesc.SampleDesc.Count = 1;
				scaledTextureDesc.SampleDesc.Quality = 0;
				scaledTextureDesc.Usage = D3D11_USAGE_DEFAULT;
				scaledTextureDesc.Format = desc.Format;
				scaledTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				scaledTextureDesc.CPUAccessFlags = 0;
				scaledTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

				ID3D11Texture2DPtr scaledTexture;
				hr = screenData->device->CreateTexture2D(&scaledTextureDesc, NULL, &scaledTexture);
				if (FAILED(hr))
				{
					qCritical(Q_FUNC_INFO " Failed to CreateTexture2D: 0x%X", hr);
					return GrabResultError;
				}
				ID3D11ShaderResourceViewPtr scaledTextureView;
				hr = screenData->device->CreateShaderResourceView(scaledTexture, NULL, &scaledTextureView);
				if (FAILED(hr))
				{
					qCritical(Q_FUNC_INFO " Failed to CreateShaderResourceView: 0x%X", hr);
					return GrabResultError;
				}

				screenData->context->CopySubresourceRegion(scaledTexture, 0, 0, 0, 0, texture, 0, NULL);
				screenData->context->GenerateMips(scaledTextureView);

				screenData->context->CopySubresourceRegion(screenData->textureCopy, 0, 0, 0, 0, scaledTexture, DownscaleMipLevel, NULL);
			}
			else
				screenData->context->CopyResource(screenData->textureCopy, texture);

			IDXGISurface1Ptr surface;
			hr = screenData->textureCopy->QueryInterface(IID_IDXGISurface1, (void**)&surface);
			if (FAILED(hr))
			{
				qCritical(Q_FUNC_INFO " Failed to cast textureCopy to IID_IDXGISurface1: 0x%X", hr);
				return GrabResultError;
			}

			hr = surface->Map(&screenData->surfaceMap, DXGI_MAP_READ);
			if (FAILED(hr))
			{
				qCritical(Q_FUNC_INFO " Failed to get surface map: 0x%X", hr);
				return GrabResultError;
			}

			screen.imgData = screenData->surfaceMap.pBits;
			screen.imgDataSize = texDesc.Height * screenData->surfaceMap.Pitch;
			screen.imgFormat = mapDXGIFormatToBufferFormat(desc.Format);
			screen.scale = 1.0 / (1 << DownscaleMipLevel);
			screen.bytesPerRow = screenData->surfaceMap.Pitch;

			screenData->duplication->ReleaseFrame();
		}

		if (!anyUpdate)
			return GrabResultFrameNotReady;
	}
	catch (_com_error e)
	{
		qCritical(Q_FUNC_INFO " COM Error: 0x%X", e.Error());
		return GrabResultError;
	}

	return GrabResultOk;
}

#endif
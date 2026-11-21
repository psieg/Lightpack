#include "DxgiFrameGrabber.hpp"

#include "hooksutils.h"
#include "../common/msvcstub.h"
#include <initguid.h>
#include "D3D11.h"
#include "DXGI.h"
#include "D3D10.h"

#include "ProxyFuncVFTable.hpp"
#include "IPCContext.hpp"

DxgiFrameGrabber *DxgiFrameGrabber::m_this = NULL;

enum DxgiDevice {
	DxgiDeviceD3D10,
	DxgiDeviceD3D11,
	DxgiDeviceUnknown
};

DxgiDevice dxgiDevice = DxgiDeviceUnknown;

typedef HRESULT (WINAPI *DXGISCPresentFunc)(IDXGISwapChain *, UINT, UINT);

HRESULT WINAPI DXGIPresent(IDXGISwapChain * sc, UINT b, UINT c);

DxgiFrameGrabber::DxgiFrameGrabber(HANDLE syncRunMutex, Logger *logger) : GAPIProxyFrameGrabber(syncRunMutex), LoggableTrait(logger) {
	m_isInited = false;
	m_ipcContext = NULL;
	m_mapTexture10 = NULL;
	m_mapDevice10 = NULL;
	m_mapTexture11 = NULL;
	m_mapDeviceContext11 = NULL;
	m_mapDevice11 = NULL;
	m_frameCount = 0;
	m_mapPending = false;
	m_lastGrab = 0;
}

bool DxgiFrameGrabber::init() {
	if (!m_isInited) {
		if (m_ipcContext) {
			m_dxgiPresentProxyFuncVFTable = new ProxyFuncVFTable(this->calcDxgiPresentPointer(), reinterpret_cast<void*>(DXGIPresent), m_logger);
			m_isInited = m_dxgiPresentProxyFuncVFTable->init();
		}
	}
	return m_isInited;
}

bool DxgiFrameGrabber::isGAPILoaded() {
	return GetModuleHandleA("dxgi.dll") != NULL;
}

bool DxgiFrameGrabber::installHooks() {
	return m_isInited && m_dxgiPresentProxyFuncVFTable->installHook();
}

bool DxgiFrameGrabber::isHooksInstalled() {
	return isGAPILoaded() && m_isInited && m_dxgiPresentProxyFuncVFTable->isHookInstalled();
}

bool DxgiFrameGrabber::removeHooks() {
	return m_isInited && m_dxgiPresentProxyFuncVFTable->removeHook();
}

void DxgiFrameGrabber::free() {
	if (m_isInited) {
		m_isInited = false;
		delete m_dxgiPresentProxyFuncVFTable;
		m_dxgiPresentProxyFuncVFTable = NULL;
	}

	freeDeviceData10();
	freeDeviceData11();
}

void DxgiFrameGrabber::freeDeviceData10() {
	m_mapPending = false;

	if (m_mapTexture10) {
		m_mapTexture10->Release();
		m_mapTexture10 = NULL;
	}
	if (m_mapDevice10) {
		m_mapDevice10->Release();
		m_mapDevice10 = NULL;
	}
}

void DxgiFrameGrabber::freeDeviceData11() {
	m_mapPending = false;

	if (m_mapTexture11) {
		m_mapTexture11->Release();
		m_mapTexture11 = NULL;
	}
	if (m_mapDeviceContext11) {
		m_mapDeviceContext11->Release();
		m_mapDeviceContext11 = NULL;
	}
	if (m_mapDevice11) {
		m_mapDevice11->Release();
		m_mapDevice11 = NULL;
	}
}

void ** DxgiFrameGrabber::calcDxgiPresentPointer() {
#ifdef HOOKS_SYSWOW64
	UINT offset = m_ipcContext->m_pConfigDesc->dxgiPresentFuncOffset32;
#else
	UINT offset = m_ipcContext->m_pConfigDesc->dxgiPresentFuncOffset;
#endif
	HMODULE hDxgiMod = GetModuleHandleA("dxgi.dll");
	if (!IsModuleOffsetValid(hDxgiMod, offset, sizeof(void *))) {
		if (m_logger)
			m_logger->reportLogError(L"Invalid dxgiPresentFuncOffset = 0x%x", offset);
		return NULL;
	}
	void * hDxgi = reinterpret_cast<void *>(hDxgiMod);
	if (m_logger)
		m_logger->reportLogDebug(L"dxgiPresentFuncOffset = 0x%x, hDxgi = 0x%p", offset, hDxgi);
	void ** result = static_cast<void ** >(incPtr(hDxgi, offset));
	if (m_logger)
		m_logger->reportLogDebug(L"dxgi.dll = 0x%p, swapchain::present location = 0x%p", hDxgi, result);
	return result;
}

void DxgiFrameGrabber::D3D10Grab(ID3D10Texture2D* pBackBuffer) {
	D3D10_TEXTURE2D_DESC tex_desc;
	pBackBuffer->GetDesc(&tex_desc);

	ID3D10Device *pDev;
	pBackBuffer->GetDevice(&pDev);

	ID3D10Texture2D * pTexture;

	HRESULT hr = S_OK;

	if (m_mapTexture10 &&
		(m_mapDevice10 != pDev
		|| m_mapWidth != tex_desc.Width
		|| m_mapHeight != tex_desc.Height)) {
		freeDeviceData10();
	}
	if (!m_mapTexture10) {
		tex_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
		tex_desc.ArraySize = 1;
		tex_desc.MipLevels = 1;
		tex_desc.BindFlags = 0;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Usage = D3D10_USAGE_STAGING;
		tex_desc.MiscFlags = 0;

		hr = pDev->CreateTexture2D(&tex_desc, NULL, &pTexture);
		m_mapTexture10 = pTexture;
		m_mapDevice10 = pDev;
		m_mapDevice10->AddRef();
		m_mapWidth = tex_desc.Width;
		m_mapHeight = tex_desc.Height;
		m_mapFormat = tex_desc.Format;
	}
	pTexture = m_mapTexture10;

	pDev->CopyResource(pTexture, pBackBuffer);

	pDev->Release();
}

bool DxgiFrameGrabber::D3D10Map() {
	IPCContext *ipcContext = m_ipcContext;
	Logger *logger = m_logger;
	D3D10_MAPPED_TEXTURE2D mappedTexture;
	HRESULT hr;

	hr = m_mapTexture10->Map(D3D10CalcSubresource(0, 0, 1), D3D10_MAP_READ, D3D10_MAP_FLAG_DO_NOT_WAIT, &mappedTexture);
	if (hr == DXGI_ERROR_WAS_STILL_DRAWING) {
		return false;
	} else if (S_OK != hr) {
		logger->reportLogError(L"d3d10 couldn't map texture, hresult = 0x%x", hr);
		return false;
	}

	if (!hooksGrabberValidateFrameSize(m_mapWidth, m_mapHeight, mappedTexture.RowPitch)) {
		logger->reportLogWarning(L"d3d10 frame exceeds shared memory bounds (%ux%u, pitch=%u)", m_mapWidth, m_mapHeight, mappedTexture.RowPitch);
		m_mapTexture10->Unmap(D3D10CalcSubresource(0, 0, 1));
		return true;
	}

	DWORD errorcode;
	if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(ipcContext->m_hMutex, 0))) {
		const size_t rowBytes = static_cast<size_t>(m_mapWidth) * 4;
		ipcContext->m_pMemDesc->width = m_mapWidth;
		ipcContext->m_pMemDesc->height = m_mapHeight;
		ipcContext->m_pMemDesc->rowPitch = static_cast<UINT>(rowBytes);
		switch (m_mapFormat)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				ipcContext->m_pMemDesc->format = BufferFormatAbgr;
				break;
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				ipcContext->m_pMemDesc->format = BufferFormatArgb;
				break;
			default:
				ipcContext->m_pMemDesc->format = BufferFormatUnknown;
				logger->reportLogWarning(L"d3d10 unkown buffer format: 0x%x", m_mapFormat);
		}
		ipcContext->m_pMemDesc->frameId++;

		BYTE *pMemDataMap = reinterpret_cast<BYTE *>(ipcContext->m_pMemMap) + sizeof(*ipcContext->m_pMemDesc);
		const BYTE *pSrc = reinterpret_cast<const BYTE *>(mappedTexture.pData);
		if (mappedTexture.RowPitch == rowBytes) {
			memcpy(pMemDataMap, pSrc, rowBytes * static_cast<size_t>(m_mapHeight));
		} else {
			size_t cleanOffset = 0, pitchOffset = 0;
			for (UINT i = 0; i < m_mapHeight; ++i) {
				memcpy(pMemDataMap + cleanOffset, pSrc + pitchOffset, rowBytes);
				cleanOffset += rowBytes;
				pitchOffset += mappedTexture.RowPitch;
			}
		}
		ReleaseMutex(ipcContext->m_hMutex);
		SetEvent(ipcContext->m_hFrameGrabbedEvent);
	} else {
		logger->reportLogError(L"d3d10 couldn't wait mutex. errocode = 0x%x", errorcode);
	}

	m_mapTexture10->Unmap(D3D10CalcSubresource(0, 0, 1));

	return true;
}

void DxgiFrameGrabber::D3D11Grab(ID3D11Texture2D *pBackBuffer) {
	D3D11_TEXTURE2D_DESC tex_desc;
	pBackBuffer->GetDesc(&tex_desc);

	ID3D11Device *pDev;
	pBackBuffer->GetDevice(&pDev);
	ID3D11DeviceContext * pDevContext;

	ID3D11Texture2D * pTexture;

	HRESULT hr = S_OK;
	if (m_mapTexture11 &&
		(m_mapDevice11 != pDev
		|| m_mapWidth != tex_desc.Width
		|| m_mapHeight != tex_desc.Height)) {
		freeDeviceData11();
	}
	if (!m_mapTexture11) {
		tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		tex_desc.ArraySize = 1;
		tex_desc.MipLevels = 1;
		tex_desc.BindFlags = 0;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Usage = D3D11_USAGE_STAGING;
		tex_desc.MiscFlags = 0;

		hr = pDev->CreateTexture2D(&tex_desc, NULL, &pTexture);
		m_mapTexture11 = pTexture;
		m_mapDevice11 = pDev;
		m_mapDevice11->AddRef();
		pDev->GetImmediateContext(&pDevContext);
		m_mapDeviceContext11 = pDevContext;
		m_mapDeviceContext11->AddRef();
		m_mapWidth = tex_desc.Width;
		m_mapHeight = tex_desc.Height;
		m_mapFormat = tex_desc.Format;
	}
	pDevContext = m_mapDeviceContext11;
	pTexture = m_mapTexture11;

	pDevContext->CopyResource(pTexture, pBackBuffer);

	pDevContext->Release();
	pDev->Release();
}

bool DxgiFrameGrabber::D3D11Map() {
	IPCContext *ipcContext = m_ipcContext;
	Logger *logger = m_logger;
	D3D11_MAPPED_SUBRESOURCE mappedTexture;
	HRESULT hr;

	hr = m_mapDeviceContext11->Map(m_mapTexture11, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mappedTexture);
	if (hr == DXGI_ERROR_WAS_STILL_DRAWING) {
		return false;
	} else if (S_OK != hr) {
		logger->reportLogError(L"d3d11 couldn't map texture, hresult = 0x%x", hr);
		return false;
	}

	if (!hooksGrabberValidateFrameSize(m_mapWidth, m_mapHeight, mappedTexture.RowPitch)) {
		logger->reportLogWarning(L"d3d11 frame exceeds shared memory bounds (%ux%u, pitch=%u)", m_mapWidth, m_mapHeight, mappedTexture.RowPitch);
		m_mapDeviceContext11->Unmap(m_mapTexture11, D3D10CalcSubresource(0, 0, 1));
		return true;
	}

	DWORD errorcode;
	if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(ipcContext->m_hMutex, 0))) {
		const size_t rowBytes = static_cast<size_t>(m_mapWidth) * 4;
		ipcContext->m_pMemDesc->width = m_mapWidth;
		ipcContext->m_pMemDesc->height = m_mapHeight;
		ipcContext->m_pMemDesc->rowPitch = static_cast<UINT>(rowBytes);
		switch (m_mapFormat)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				ipcContext->m_pMemDesc->format = BufferFormatAbgr;
				break;
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				ipcContext->m_pMemDesc->format = BufferFormatArgb;
				break;
			default:
				ipcContext->m_pMemDesc->format = BufferFormatUnknown;
				logger->reportLogWarning(L"d3d11 unkown buffer format: 0x%x", m_mapFormat);
		}
		ipcContext->m_pMemDesc->frameId++;

		BYTE *pMemDataMap = reinterpret_cast<BYTE *>(ipcContext->m_pMemMap) + sizeof(*ipcContext->m_pMemDesc);
		const BYTE *pSrc = reinterpret_cast<const BYTE *>(mappedTexture.pData);
		if (mappedTexture.RowPitch == rowBytes) {
			memcpy(pMemDataMap, pSrc, rowBytes * static_cast<size_t>(m_mapHeight));
		} else {
			size_t cleanOffset = 0, pitchOffset = 0;
			for (UINT i = 0; i < m_mapHeight; ++i) {
				memcpy(pMemDataMap + cleanOffset, pSrc + pitchOffset, rowBytes);
				cleanOffset += rowBytes;
				pitchOffset += mappedTexture.RowPitch;
			}
		}
		ReleaseMutex(ipcContext->m_hMutex);
		SetEvent(ipcContext->m_hFrameGrabbedEvent);
	} else {
		logger->reportLogError(L"d3d11 couldn't wait mutex. errocode = 0x%x", errorcode);
	}

	m_mapDeviceContext11->Unmap(m_mapTexture11, D3D10CalcSubresource(0, 0, 1));

	return true;
}

HRESULT WINAPI DXGIPresent(IDXGISwapChain * sc, UINT b, UINT c) {

	DxgiFrameGrabber *dxgiFrameGrabber = DxgiFrameGrabber::getInstance();
	dxgiFrameGrabber->m_frameCount++;

	Logger *logger = dxgiFrameGrabber->m_logger;

	if (WAIT_OBJECT_0 == WaitForSingleObject(dxgiFrameGrabber->m_syncRunMutex, 0)) {

		DXGI_SWAP_CHAIN_DESC desc;
		sc->GetDesc(&desc);
		logger->reportLogDebug(L"dxgi present: Buffers count: %u, Output hwnd: %u, %s", desc.BufferCount, desc.OutputWindow, desc.Windowed ? L"windowed" : L"fullscreen");

		if (dxgiDevice == DxgiDeviceUnknown) {
			// If the process uses DX10, the device will internally be a DX11 device too
			// But if the process uses DX11, the device will not be a DX10 device
			ID3D10Device* dev;
			HRESULT hr = sc->GetDevice(IID_ID3D10Device, (void**)&dev);
			if (hr == S_OK) {
				dxgiDevice = DxgiDeviceD3D10;
				dev->Release();
			}
			else {
				dxgiDevice = DxgiDeviceD3D11;
			}
		}

		if (!desc.Windowed) {
			if (dxgiFrameGrabber->m_mapPending) {
				bool done = false;
				if (dxgiDevice == DxgiDeviceD3D10) {
					done = dxgiFrameGrabber->D3D10Map();
				}
				else if (dxgiDevice == DxgiDeviceD3D11) {
					done = dxgiFrameGrabber->D3D11Map();
				}
				if (done) {
					dxgiFrameGrabber->m_mapPending = false;
					logger->reportLogDebug(L"Map succeeded");
				}
			} else if (dxgiFrameGrabber->m_ipcContext->m_pConfigDesc->grabbingStarted
				&& GetTickCount() - dxgiFrameGrabber->m_lastGrab >= dxgiFrameGrabber->m_ipcContext->m_pConfigDesc->grabDelay) {
				// only capture a new frame if the old one was processed to shared memory and the delay has passed since the last grab
				if (!dxgiFrameGrabber->m_ipcContext->m_pConfigDesc->grabbingStarted) {
					dxgiFrameGrabber->m_ipcContext->m_pMemDesc->frameId = HOOKSGRABBER_BLANK_FRAME_ID;
					SetEvent(dxgiFrameGrabber->m_ipcContext->m_hFrameGrabbedEvent);
				} else {
					if (dxgiDevice == DxgiDeviceD3D10) {
						ID3D10Texture2D *pBackBuffer;
						HRESULT hr = sc->GetBuffer(0, IID_ID3D10Texture2D, reinterpret_cast<LPVOID*> (&pBackBuffer));
						if (hr == S_OK) {
							dxgiFrameGrabber->D3D10Grab(pBackBuffer);
							pBackBuffer->Release();
						}
						else {
							logger->reportLogError(L"couldn't get d3d10 buffer. returned 0x%x", hr);
						}
					}
					if (dxgiDevice == DxgiDeviceD3D11) {
						ID3D11Texture2D *pBackBuffer;
						HRESULT hr = sc->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<LPVOID*> (&pBackBuffer));
						if (hr == S_OK) {
							dxgiFrameGrabber->D3D11Grab(pBackBuffer);
							pBackBuffer->Release();
						}
						else {
							logger->reportLogError(L"couldn't get d3d11 buffer. returned 0x%x", hr);
						}
					}
					dxgiFrameGrabber->m_lastGrab = GetTickCount();
					dxgiFrameGrabber->m_mapPending = true;
					logger->reportLogDebug(L"Grabbed...");
				}
			} else if (!dxgiFrameGrabber->m_ipcContext->m_pConfigDesc->grabbingStarted) {
				dxgiFrameGrabber->m_ipcContext->m_pMemDesc->frameId = HOOKSGRABBER_BLANK_FRAME_ID;
				SetEvent(dxgiFrameGrabber->m_ipcContext->m_hFrameGrabbedEvent);
				logger->reportLogDebug(L"Sending blank frame because grabbing not started");
			}
		}

		DXGISCPresentFunc originalFunc = reinterpret_cast<DXGISCPresentFunc>(dxgiFrameGrabber->m_dxgiPresentProxyFuncVFTable->getOriginalFunc());
		HRESULT res = originalFunc(sc, b, c);
		ReleaseMutex(dxgiFrameGrabber->m_syncRunMutex);
		return res;
	} else {
		return S_OK; // assume it would have worked if we weren't being unloaded
	}

}

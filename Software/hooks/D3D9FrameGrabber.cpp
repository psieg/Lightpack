#include "D3D9FrameGrabber.hpp"
#include "IPCContext.hpp"

#include "hooksutils.h"
#include "../common/msvcstub.h"
#include <initguid.h>
#include "d3d9types.h"
#include "d3d9.h"

#include "ProxyFuncJmpToVFTable.hpp"

#define D3D9_PRESENT_FUNC_ORD 17
#define D3D9_SCPRESENT_FUNC_ORD 3
#define D3D9_RESET_FUNC_ORD 16

//#define SWITCH_TO_VMT

D3D9FrameGrabber *D3D9FrameGrabber::m_this = NULL;

HRESULT WINAPI D3D9Present(IDirect3DDevice9 *, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT(WINAPI *D3D9PresentFunc)(IDirect3DDevice9 *, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
HRESULT WINAPI D3D9SCPresent(IDirect3DSwapChain9 *, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD);
typedef HRESULT (WINAPI *D3D9SCPresentFunc)(IDirect3DSwapChain9 *, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD);

HRESULT WINAPI D3D9Reset(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS*);
typedef HRESULT(WINAPI *D3D9ResetFunc)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS*);

BufferFormat getCompatibleBufferFormat(D3DFORMAT format);

D3D9FrameGrabber::D3D9FrameGrabber(HANDLE syncRunMutex, Logger *logger): GAPIProxyFrameGrabber(syncRunMutex), LoggableTrait(logger) {
	m_isInited = false;
	m_ipcContext = NULL;
	m_pDemultisampledSurf = NULL;
	m_pOffscreenSurf = NULL;
	m_pDev = NULL;
	m_surfFormat = D3DFMT_UNKNOWN;
	m_surfWidth = 0;
	m_surfHeight = 0;
	m_frameCount = 0;
	m_mapPending = false;
	m_lastGrab = 0;
}

bool D3D9FrameGrabber::init() {
	if (!m_isInited) {
		if (m_ipcContext) {
			m_d3d9PresentProxyFunc = new ProxyFuncJmpToVFTable(this->calcD3d9PresentPointer(), reinterpret_cast<void*>(D3D9Present), m_logger);
			m_d3d9SCPresentProxyFunc = new ProxyFuncJmpToVFTable(this->calcD3d9SCPresentPointer(), reinterpret_cast<void*>(D3D9SCPresent), m_logger);
			m_d3d9ResetProxyFunc = new ProxyFuncJmpToVFTable(this->calcD3d9ResetPointer(), reinterpret_cast<void*>(D3D9Reset), m_logger);
			m_isInited = m_d3d9PresentProxyFunc->init() && m_d3d9SCPresentProxyFunc->init() && m_d3d9ResetProxyFunc->init();
		}
	}
	return m_isInited;
}

bool D3D9FrameGrabber::isGAPILoaded() {
	return GetModuleHandleA("d3d9.dll") != NULL;
}

bool D3D9FrameGrabber::installHooks() {
	m_d3d9PresentProxyFunc->installHook();
	m_d3d9SCPresentProxyFunc->installHook();
	m_d3d9ResetProxyFunc->installHook();
	return this->isHooksInstalled();
}

bool D3D9FrameGrabber::isHooksInstalled() {
	return isGAPILoaded() && m_isInited
		&& (m_d3d9PresentProxyFunc->isHookInstalled()
		|| m_d3d9SCPresentProxyFunc->isHookInstalled()
		|| m_d3d9ResetProxyFunc->isHookInstalled());
}

bool D3D9FrameGrabber::removeHooks() {
	m_d3d9PresentProxyFunc->removeHook();
	m_d3d9SCPresentProxyFunc->removeHook();
	m_d3d9ResetProxyFunc->removeHook();

	return !m_d3d9PresentProxyFunc->isHookInstalled() && !m_d3d9SCPresentProxyFunc->isHookInstalled() && !m_d3d9ResetProxyFunc->isHookInstalled();
}

void D3D9FrameGrabber::free() {
	delete m_d3d9PresentProxyFunc;
	delete m_d3d9SCPresentProxyFunc;
	delete m_d3d9ResetProxyFunc;
	m_d3d9PresentProxyFunc = NULL;
	m_d3d9SCPresentProxyFunc = NULL;
	m_d3d9ResetProxyFunc = NULL;
	m_isInited = false;

	freeDeviceData();
}

void D3D9FrameGrabber::freeDeviceData() {
	m_mapPending = false;

	if (m_pDemultisampledSurf) {
		m_pDemultisampledSurf->Release();
		m_pDemultisampledSurf = NULL;
	}
	if (m_pOffscreenSurf) {
		m_pOffscreenSurf->Release();
		m_pOffscreenSurf = NULL;
	}
	if (m_pDev) {
		m_pDev->Release();
		m_pDev = NULL;
	}
}

void ** D3D9FrameGrabber::calcD3d9PresentPointer() {
#ifdef HOOKS_SYSWOW64
	UINT offset = m_ipcContext->m_pConfigDesc->d3d9PresentFuncOffset32;
#else
	UINT offset = m_ipcContext->m_pConfigDesc->d3d9PresentFuncOffset;
#endif
	HMODULE hD3d9Mod = GetModuleHandleA("d3d9.dll");
	if (!IsModuleOffsetValid(hD3d9Mod, offset, 5)) {
		m_logger->reportLogError(L"Invalid d3d9PresentFuncOffset = 0x%x", offset);
		return NULL;
	}
	void * hD3d9 = reinterpret_cast<void *>(hD3d9Mod);
	m_logger->reportLogDebug(L"d3d9PresentFuncOffset = 0x%x, hD3d9 = 0x%p", offset, hD3d9);
	void ** result = static_cast<void ** >(incPtr(hD3d9, offset));
	m_logger->reportLogDebug(L"d3d9.dll = 0x%p, device::present location = 0x%p", hD3d9, result);
	return result;
}

void ** D3D9FrameGrabber::calcD3d9SCPresentPointer() {
#ifdef HOOKS_SYSWOW64
	UINT offset = m_ipcContext->m_pConfigDesc->d3d9SCPresentFuncOffset32;
#else
	UINT offset = m_ipcContext->m_pConfigDesc->d3d9SCPresentFuncOffset;
#endif
	HMODULE hD3d9Mod = GetModuleHandleA("d3d9.dll");
	if (!IsModuleOffsetValid(hD3d9Mod, offset, 5)) {
		m_logger->reportLogError(L"Invalid d3d9SCPresentFuncOffset = 0x%x", offset);
		return NULL;
	}
	void * hD3d9 = reinterpret_cast<void *>(hD3d9Mod);
	m_logger->reportLogDebug(L"d3d9SCPresentFuncOffset = 0x%x, hD3d9 = 0x%p", offset, hD3d9);
	void ** result = static_cast<void ** >(incPtr(hD3d9, offset));
	m_logger->reportLogDebug(L"d3d9.dll = 0x%p, swapchain::present location = 0x%p", hD3d9, result);
	return result;
}

void ** D3D9FrameGrabber::calcD3d9ResetPointer() {
#ifdef HOOKS_SYSWOW64
	UINT offset = m_ipcContext->m_pConfigDesc->d3d9ResetFuncOffset32;
#else
	UINT offset = m_ipcContext->m_pConfigDesc->d3d9ResetFuncOffset;
#endif
	HMODULE hD3d9Mod = GetModuleHandleA("d3d9.dll");
	if (!IsModuleOffsetValid(hD3d9Mod, offset, 5)) {
		m_logger->reportLogError(L"Invalid d3d9ResetFuncOffset = 0x%x", offset);
		return NULL;
	}
	void * hD3d9 = reinterpret_cast<void *>(hD3d9Mod);
	m_logger->reportLogDebug(L"d3d9ResetFuncOffset = 0x%x, hD3d9 = 0x%p", offset, hD3d9);
	void ** result = static_cast<void ** >(incPtr(hD3d9, offset));
	m_logger->reportLogDebug(L"d3d9.dll = 0x%p, device::reset location = 0x%p", hD3d9, result);
	return result;
}

HRESULT WINAPI D3D9Present(IDirect3DDevice9 *pDev, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
	D3D9FrameGrabber *d3d9FrameGrabber = D3D9FrameGrabber::getInstance();
	Logger *logger = d3d9FrameGrabber->m_logger;
	d3d9FrameGrabber->m_frameCount++;

	DWORD errorcode = WaitForSingleObject(d3d9FrameGrabber->m_syncRunMutex, 0);
	if (errorcode == WAIT_OBJECT_0 || errorcode == WAIT_ABANDONED) {
		IPCContext *ipcContext = d3d9FrameGrabber->m_ipcContext;
		logger->reportLogDebug(L"D3D9Present");
		HRESULT hRes;

		if (d3d9FrameGrabber->m_mapPending) {
			IDirect3DSurface9 *pOffscreenSurf = d3d9FrameGrabber->m_pOffscreenSurf;
			UINT width = d3d9FrameGrabber->m_surfWidth;
			UINT height = d3d9FrameGrabber->m_surfHeight;

			RECT newRect = RECT();
			D3DLOCKED_RECT lockedSrcRect;
			newRect.right = width;
			newRect.bottom = height;
			hRes = pOffscreenSurf->LockRect(&lockedSrcRect, &newRect, D3DLOCK_DONOTWAIT);
			if (hRes == D3DERR_WASSTILLDRAWING)
			{
				goto stillWaiting;
			} else if (FAILED(hRes)) {
				logger->reportLogError(L"GetFramePrep: FAILED to lock source rect. (0x%x)", hRes);
				goto endMap;
			}

			if (lockedSrcRect.Pitch <= 0 || !hooksGrabberValidateFrameSize(width, height, static_cast<UINT>(lockedSrcRect.Pitch))) {
				logger->reportLogWarning(L"d3d9 frame exceeds shared memory bounds (%ux%u, pitch=%d)", width, height, lockedSrcRect.Pitch);
				goto endMap;
			}

			errorcode = WaitForSingleObject(ipcContext->m_hMutex, 0);
			if (errorcode == WAIT_OBJECT_0 || errorcode == WAIT_ABANDONED) {
				const size_t rowBytes = static_cast<size_t>(width) * 4;
				ipcContext->m_pMemDesc->width = width;
				ipcContext->m_pMemDesc->height = height;
				ipcContext->m_pMemDesc->rowPitch = static_cast<UINT>(rowBytes);
				ipcContext->m_pMemDesc->frameId++;
				ipcContext->m_pMemDesc->format = getCompatibleBufferFormat(d3d9FrameGrabber->m_surfFormat);

				BYTE *pMemDataMap = reinterpret_cast<BYTE *>(ipcContext->m_pMemMap) + sizeof(*ipcContext->m_pMemDesc);
				const BYTE *pSrc = reinterpret_cast<const BYTE *>(lockedSrcRect.pBits);
				if (static_cast<size_t>(lockedSrcRect.Pitch) == rowBytes) {
					memcpy(pMemDataMap, pSrc, rowBytes * static_cast<size_t>(height));
				} else {
					size_t cleanOffset = 0, pitchOffset = 0;
					for (UINT i = 0; i < height; ++i) {
						memcpy(pMemDataMap + cleanOffset, pSrc + pitchOffset, rowBytes);
						cleanOffset += rowBytes;
						pitchOffset += static_cast<size_t>(lockedSrcRect.Pitch);
					}
				}
				ReleaseMutex(ipcContext->m_hMutex);
				SetEvent(ipcContext->m_hFrameGrabbedEvent);
			} else {
				logger->reportLogError(L"d3d9 couldn't wait mutex. errocode = 0x%x", errorcode);
			}

		endMap:
			pOffscreenSurf->UnlockRect();
			d3d9FrameGrabber->m_mapPending = false;
		}
	stillWaiting:

		// only capture a new frame if the old one was processed to shared memory and the delay has passed since the last grab
		if (!d3d9FrameGrabber->m_mapPending && (GetTickCount() - d3d9FrameGrabber->m_lastGrab >= d3d9FrameGrabber->m_ipcContext->m_pConfigDesc->grabDelay)) {
			if (!ipcContext->m_pConfigDesc->grabbingStarted) {
				ipcContext->m_pMemDesc->frameId = HOOKSGRABBER_BLANK_FRAME_ID;
				SetEvent(ipcContext->m_hFrameGrabbedEvent);
			} else {
				IDirect3DSurface9 *pBackBuffer = NULL;
				IDirect3DSurface9 *pDemultisampledSurf = NULL;
				IDirect3DSurface9 *pOffscreenSurf = NULL;
				IDirect3DSwapChain9 *pSc = NULL;
				D3DPRESENT_PARAMETERS params;

				if (FAILED(hRes = pDev->GetSwapChain(0, &pSc))) {
					logger->reportLogError(L"d3d9present couldn't get swapchain. result 0x%x", hRes);
					goto end;
				}

				hRes = pSc->GetPresentParameters(&params);
				if (FAILED(hRes) || params.Windowed) {
					goto end;
				}

				if (FAILED(hRes = pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))) {
					goto end;
				}

				D3DSURFACE_DESC surfDesc;
				pBackBuffer->GetDesc(&surfDesc);

				if (d3d9FrameGrabber->m_pDemultisampledSurf &&
					(d3d9FrameGrabber->m_pDev != pDev
					|| d3d9FrameGrabber->m_surfFormat != surfDesc.Format
					|| d3d9FrameGrabber->m_surfWidth != surfDesc.Width
					|| d3d9FrameGrabber->m_surfHeight != surfDesc.Height)) {
					d3d9FrameGrabber->freeDeviceData();
				}
				if (!d3d9FrameGrabber->m_pDemultisampledSurf) {
					hRes = pDev->CreateRenderTarget(
						surfDesc.Width, surfDesc.Height,
						surfDesc.Format, D3DMULTISAMPLE_NONE, 0, false,
						&pDemultisampledSurf, NULL);
					if (FAILED(hRes))
					{
						logger->reportLogError(L"GetFramePrep: FAILED to create demultisampled render target. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
						goto end;
					}
					d3d9FrameGrabber->m_pDemultisampledSurf = pDemultisampledSurf;
					d3d9FrameGrabber->m_pDev = pDev;
					d3d9FrameGrabber->m_pDev->AddRef();
					d3d9FrameGrabber->m_surfFormat = surfDesc.Format;
					d3d9FrameGrabber->m_surfWidth = surfDesc.Width;
					d3d9FrameGrabber->m_surfHeight = surfDesc.Height;

					hRes = pDev->CreateOffscreenPlainSurface(
						surfDesc.Width, surfDesc.Height,
						surfDesc.Format, D3DPOOL_SYSTEMMEM,
						&pOffscreenSurf, NULL);
					if (FAILED(hRes))
					{
						d3d9FrameGrabber->m_pDemultisampledSurf->Release();
						d3d9FrameGrabber->m_pDemultisampledSurf = NULL;
						logger->reportLogError(L"GetFramePrep: FAILED to create image surface. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
						goto end;
					}
					d3d9FrameGrabber->m_pOffscreenSurf = pOffscreenSurf;
				}
				pDemultisampledSurf = d3d9FrameGrabber->m_pDemultisampledSurf;
				pOffscreenSurf = d3d9FrameGrabber->m_pOffscreenSurf;

				hRes = pDev->StretchRect(pBackBuffer, NULL, pDemultisampledSurf, NULL, D3DTEXF_LINEAR);
				if (FAILED(hRes))
				{
					logger->reportLogError(L"GetFramePrep: StretchRect FAILED for image surfacee. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
					goto end;
				}

				hRes = pDev->GetRenderTargetData(pDemultisampledSurf, pOffscreenSurf);
				if (FAILED(hRes))
				{
					logger->reportLogError(L"GetFramePrep: GetRenderTargetData() FAILED for image surfacee. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
					goto end;
				}

				d3d9FrameGrabber->m_mapPending = true;
				d3d9FrameGrabber->m_lastGrab = GetTickCount();

			end:
				if (pBackBuffer) pBackBuffer->Release();
				if (pSc) pSc->Release();
			}
		}

		HRESULT result;
#ifdef SWITCH_TO_VMT
		// This method would be more elegant, but leads to crashes when combined with FRAPS (maybe steam overlay is part of the mess, too)
		if (!d3d9FrameGrabber->m_d3d9PresentProxyFunc->isSwitched()) {
			// find the VFT in this process and use it in the future
			uintptr_t * pvtbl = *((uintptr_t**)(pDev));
			uintptr_t* presentFuncPtr = &pvtbl[D3D9_PRESENT_FUNC_ORD];
			if (!d3d9FrameGrabber->m_d3d9PresentProxyFunc->switchToVFTHook((void**)presentFuncPtr, D3D9Present)) {
				logger->reportLogError(L"d3d9 failed to switch from jmp to vft proxy");
			}
		}
		D3D9PresentFunc orig = reinterpret_cast<D3D9PresentFunc>(d3d9FrameGrabber->m_d3d9PresentProxyFunc->getOriginalFunc());
		result = orig(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
#else
		if (!d3d9FrameGrabber->m_d3d9PresentProxyFunc->removeHook()) {
			int i = GetLastError();
			logger->reportLogError(L"d3d9 error occured while trying to removeHook before original call 0x%x", i);
		}
		result = pDev->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
		if (!d3d9FrameGrabber->m_d3d9PresentProxyFunc->installHook()) {
			int i = GetLastError();
			logger->reportLogError(L"d3d9 error occured while trying to installHook after original call 0x%x", i);
		}
#endif

		ReleaseMutex(d3d9FrameGrabber->m_syncRunMutex);
		return result;
	} else {
		logger->reportLogError(L"d3d9sc present is skipped because mutex is busy");
		return S_FALSE;
	}
}

HRESULT WINAPI D3D9SCPresent(IDirect3DSwapChain9 *pSc, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
	D3D9FrameGrabber *d3d9FrameGrabber = D3D9FrameGrabber::getInstance();
	Logger *logger = d3d9FrameGrabber->m_logger;
	d3d9FrameGrabber->m_frameCount++;

	DWORD errorcode = WaitForSingleObject(d3d9FrameGrabber->m_syncRunMutex, 0);
	if (errorcode == WAIT_OBJECT_0 || errorcode == WAIT_ABANDONED) {
		IPCContext *ipcContext = d3d9FrameGrabber->m_ipcContext;
		logger->reportLogDebug(L"D3D9SCPresent");
		HRESULT hRes;

		if (d3d9FrameGrabber->m_mapPending) {
			IDirect3DSurface9 *pOffscreenSurf = d3d9FrameGrabber->m_pOffscreenSurf;
			UINT width = d3d9FrameGrabber->m_surfWidth;
			UINT height = d3d9FrameGrabber->m_surfHeight;
			RECT newRect = RECT();
			D3DLOCKED_RECT lockedSrcRect;
			newRect.right = width;
			newRect.bottom = height;

			hRes = pOffscreenSurf->LockRect(&lockedSrcRect, &newRect, D3DLOCK_DONOTWAIT);
			if (hRes == D3DERR_WASSTILLDRAWING) {
				goto stillWaiting;
			} else if (FAILED(hRes)) {
				logger->reportLogError(L"GetFramePrep: FAILED to lock source rect. (0x%x)", hRes);
				goto endMap;
			}

			if (lockedSrcRect.Pitch <= 0 || !hooksGrabberValidateFrameSize(width, height, static_cast<UINT>(lockedSrcRect.Pitch))) {
				logger->reportLogWarning(L"d3d9sc frame exceeds shared memory bounds (%ux%u, pitch=%d)", width, height, lockedSrcRect.Pitch);
				goto endMap;
			}

			errorcode = WaitForSingleObject(ipcContext->m_hMutex, 0);
			if (errorcode == WAIT_OBJECT_0 || errorcode == WAIT_ABANDONED) {
				const size_t rowBytes = static_cast<size_t>(width) * 4;
				ipcContext->m_pMemDesc->width = width;
				ipcContext->m_pMemDesc->height = height;
				ipcContext->m_pMemDesc->rowPitch = static_cast<UINT>(rowBytes);
				ipcContext->m_pMemDesc->frameId++;
				ipcContext->m_pMemDesc->format = getCompatibleBufferFormat(d3d9FrameGrabber->m_surfFormat);

				BYTE *pMemDataMap = reinterpret_cast<BYTE *>(ipcContext->m_pMemMap) + sizeof(*ipcContext->m_pMemDesc);
				const BYTE *pSrc = reinterpret_cast<const BYTE *>(lockedSrcRect.pBits);
				if (static_cast<size_t>(lockedSrcRect.Pitch) == rowBytes) {
					memcpy(pMemDataMap, pSrc, rowBytes * static_cast<size_t>(height));
				} else {
					size_t cleanOffset = 0, pitchOffset = 0;
					for (UINT i = 0; i < height; ++i) {
						memcpy(pMemDataMap + cleanOffset, pSrc + pitchOffset, rowBytes);
						cleanOffset += rowBytes;
						pitchOffset += static_cast<size_t>(lockedSrcRect.Pitch);
					}
				}
				ReleaseMutex(ipcContext->m_hMutex);
				SetEvent(ipcContext->m_hFrameGrabbedEvent);
			} else {
				logger->reportLogError(L"d3d9sc couldn't wait mutex. errocode = 0x%x", errorcode);
			}
		endMap:
			pOffscreenSurf->UnlockRect();
			d3d9FrameGrabber->m_mapPending = false;
		}
	stillWaiting:

		// only capture a new frame if the old one was processed to shared memory and the delay has passed since the last grab
		if (!d3d9FrameGrabber->m_mapPending && (GetTickCount() - d3d9FrameGrabber->m_lastGrab >= d3d9FrameGrabber->m_ipcContext->m_pConfigDesc->grabDelay)) {
			if (!ipcContext->m_pConfigDesc->grabbingStarted) {
				ipcContext->m_pMemDesc->frameId = HOOKSGRABBER_BLANK_FRAME_ID;
				SetEvent(ipcContext->m_hFrameGrabbedEvent);
			} else {
				IDirect3DSurface9 *pBackBuffer = NULL;
				D3DPRESENT_PARAMETERS params;
				IDirect3DSurface9 *pDemultisampledSurf = NULL;
				IDirect3DSurface9 *pOffscreenSurf = NULL;
				IDirect3DDevice9 *pDev = NULL;

				HRESULT hRes = pSc->GetPresentParameters(&params);

				if (FAILED(hRes) || params.Windowed) {
					goto end;
				}

				if (FAILED(hRes = pSc->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))) {
					logger->reportLogError(L"d3d9sc couldn't get backbuffer. errorcode = 0x%x", hRes);
					goto end;
				}
				D3DSURFACE_DESC surfDesc;
				pBackBuffer->GetDesc(&surfDesc);

				hRes = pSc->GetDevice(&pDev);
				if (FAILED(hRes))
				{
					logger->reportLogError(L"GetFramePrep: FAILED to get pDev. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
					goto end;
				}

				if (d3d9FrameGrabber->m_pDemultisampledSurf &&
					(d3d9FrameGrabber->m_pDev != pDev
					|| d3d9FrameGrabber->m_surfFormat != surfDesc.Format
					|| d3d9FrameGrabber->m_surfWidth != surfDesc.Width
					|| d3d9FrameGrabber->m_surfHeight != surfDesc.Height)) {
					d3d9FrameGrabber->freeDeviceData();
				}
				if (!d3d9FrameGrabber->m_pDemultisampledSurf) {
					hRes = pDev->CreateRenderTarget(
						surfDesc.Width, surfDesc.Height,
						surfDesc.Format, D3DMULTISAMPLE_NONE, 0, false,
						&pDemultisampledSurf, NULL);
					if (FAILED(hRes))
					{
						logger->reportLogError(L"GetFramePrep: FAILED to create demultisampled render target. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
						goto end;
					}
					d3d9FrameGrabber->m_pDemultisampledSurf = pDemultisampledSurf;
					d3d9FrameGrabber->m_pDev = pDev;
					d3d9FrameGrabber->m_pDev->AddRef();
					d3d9FrameGrabber->m_surfFormat = surfDesc.Format;
					d3d9FrameGrabber->m_surfWidth = surfDesc.Width;
					d3d9FrameGrabber->m_surfHeight = surfDesc.Height;

					hRes = pDev->CreateOffscreenPlainSurface(
						surfDesc.Width, surfDesc.Height,
						surfDesc.Format, D3DPOOL_SYSTEMMEM,
						&pOffscreenSurf, NULL);
					if (FAILED(hRes))
					{
						d3d9FrameGrabber->m_pDemultisampledSurf->Release();
						d3d9FrameGrabber->m_pDemultisampledSurf = NULL;
						logger->reportLogError(L"GetFramePrep: FAILED to create image surface. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
						goto end;
					}
				}
				pDemultisampledSurf = d3d9FrameGrabber->m_pDemultisampledSurf;
				pOffscreenSurf = d3d9FrameGrabber->m_pOffscreenSurf;

				hRes = pDev->StretchRect(pBackBuffer, NULL, pDemultisampledSurf, NULL, D3DTEXF_LINEAR);
				if (FAILED(hRes))
				{
					logger->reportLogError(L"GetFramePrep: StretchRect FAILED for image surfacee. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
					goto end;
				}

				hRes = pDev->GetRenderTargetData(pDemultisampledSurf, pOffscreenSurf);
				if (FAILED(hRes))
				{
					logger->reportLogError(L"GetFramePrep: GetRenderTargetData() FAILED for image surfacee. 0x%x, width=%u, height=%u, format=%x", hRes, surfDesc.Width, surfDesc.Height, surfDesc.Format);
					goto end;
				}

				d3d9FrameGrabber->m_mapPending = true;
				d3d9FrameGrabber->m_lastGrab = GetTickCount();

			end:
				if (pBackBuffer) pBackBuffer->Release();
				if (pDev) pDev->Release();
			}
		}

		HRESULT result;
#ifdef SWITCH_TO_VMT
		// This method would be more elegant, but leads to crashes when combined with FRAPS (maybe steam overlay is part of the mess, too)
		if (!d3d9FrameGrabber->m_d3d9PresentProxyFunc->isSwitched()) {
			// find the VFT in this process and use it in the future
			uintptr_t * pvtbl = *((uintptr_t**)(pSc));
			uintptr_t* presentFuncPtr = &pvtbl[D3D9_SCPRESENT_FUNC_ORD];
			if (!d3d9FrameGrabber->m_d3d9PresentProxyFunc->switchToVFTHook((void**)presentFuncPtr, D3D9SCPresent)) {
				logger->reportLogError(L"d3d9sc failed to switch from jmp to vft proxy");
			}
		}
		D3D9SCPresentFunc orig = reinterpret_cast<D3D9SCPresentFunc>(d3d9FrameGrabber->m_d3d9PresentProxyFunc->getOriginalFunc());
		result = orig(pSc, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
#else
		if (d3d9FrameGrabber->m_d3d9SCPresentProxyFunc->removeHook()) {
			int i = GetLastError();
			logger->reportLogError(L"d3d9sc error occured while trying to removeHook before original call 0x%x", i);
		}
		result = pSc->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
		if (d3d9FrameGrabber->m_d3d9SCPresentProxyFunc->installHook()) {
			int i = GetLastError();
			logger->reportLogError(L"d3d9sc error occured while trying to installHook after original call 0x%x", i);
		}
#endif

		ReleaseMutex(d3d9FrameGrabber->m_syncRunMutex);
		return result;
	} else {
		logger->reportLogError(L"d3d9sc present is skipped because mutex is busy");
		return S_FALSE;
	}
}

HRESULT WINAPI D3D9Reset(IDirect3DDevice9* pDev, D3DPRESENT_PARAMETERS* pPresentationParameters) {
	D3D9FrameGrabber *d3d9FrameGrabber = D3D9FrameGrabber::getInstance();
	Logger *logger = d3d9FrameGrabber->m_logger;

	d3d9FrameGrabber->freeDeviceData();

	HRESULT result;
	if (!d3d9FrameGrabber->m_d3d9ResetProxyFunc->isSwitched()) {
		// find the VFT in this process and use it in the future
		uintptr_t * pvtbl = *((uintptr_t**)(pDev));
		uintptr_t* presentFuncPtr = &pvtbl[D3D9_RESET_FUNC_ORD];
		if (!d3d9FrameGrabber->m_d3d9ResetProxyFunc->switchToVFTHook((void**)presentFuncPtr, D3D9Reset)) {
			logger->reportLogError(L"d3d9 failed to switch from jmp to vft proxy");
		}
	}

	D3D9ResetFunc orig = reinterpret_cast<D3D9ResetFunc>(d3d9FrameGrabber->m_d3d9ResetProxyFunc->getOriginalFunc());
	result = orig(pDev, pPresentationParameters);

	if (result == D3DERR_DEVICELOST || result == D3DERR_DEVICENOTRESET) {
		d3d9FrameGrabber->freeDeviceData();
	}

	return result;

}

BufferFormat getCompatibleBufferFormat(D3DFORMAT format) {
	switch(format) {
		case D3DFMT_A8B8G8R8:
			return BufferFormatAbgr;
		case D3DFMT_X8B8G8R8:
			return BufferFormatAbgr;
		case D3DFMT_R8G8_B8G8:
			return BufferFormatRgbg;
		case D3DFMT_A8R8G8B8:
			return BufferFormatArgb;
		case D3DFMT_X8R8G8B8:
			return BufferFormatArgb;
		default:
			return BufferFormatArgb;
	}
}

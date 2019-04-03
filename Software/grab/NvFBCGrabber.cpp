/*
 * NvFBCGrabber.cpp
 *
 *	Created on: 01.04.19
 *		Project: Lightpack
 *
 *	Copyright (c) 2019 Maximilian Roehrl
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

#include "NvFBCGrabber.hpp"
#include "debug.h"

#include <QMessageBox>

#ifdef NVFBC_GRAB_SUPPORT

namespace {
	// 0 = /1 (no scaling)
	// 1 = /2
	// 2 = /4
	// 3 = /8 (best value for now)
	// 4+ seems to be counter-productive
	const uint8_t DownscaleMipLevel = 3;

	// Secret password that enables NvFBC for GeForce cards
	int Magic[] = { 0x0D7BC620, 0x4C17E142, 0x5E6B5997, 0x4B5A855B };

	// ARGB format has 4 bytes per pixel
	const UINT BytesPerPixel = 4;
}

struct NvFBCScreenData
{
	NvFBCScreenData(NvFBCToSys* _nv_fbc_to_sys, NvFBCFrameGrabInfo* _frame_grab_info, unsigned char* _framebuffer)
		: nv_fbc_to_sys(_nv_fbc_to_sys),
		frame_grab_info(_frame_grab_info),
		framebuffer(_framebuffer)
	{}

	NvFBCToSys* nv_fbc_to_sys;
	NvFBCFrameGrabInfo* frame_grab_info;
	unsigned char* framebuffer;
};

NvFBCGrabber::NvFBCGrabber(QObject* parent, GrabberContext* context)
	: GrabberBase(parent, context),
	m_admin_message_shown(FALSE),
	m_nvfbcDll(NULL),
	pfn_get_status(NULL),
	pfn_set_global_flags(NULL),
	pfn_create(NULL),
	pfn_enable(NULL)
{}

NvFBCGrabber::~NvFBCGrabber()
{
	freeScreens();

	if (m_nvfbcDll)
		FreeLibrary(m_nvfbcDll);
}

bool NvFBCGrabber::init()
{
	m_nvfbcDll = LoadLibrary(L"NvFBC64.dll");
	if (!m_nvfbcDll) {
		qCritical(Q_FUNC_INFO " Failed to load NvFBC library!");
		return false;
	}

	// Load the functions exported by NvFBC
	pfn_create = (NvFBC_CreateFunctionExType) GetProcAddress(m_nvfbcDll, "NvFBC_CreateEx");
	pfn_set_global_flags = (NvFBC_SetGlobalFlagsType) GetProcAddress(m_nvfbcDll, "NvFBC_SetGlobalFlags");
	pfn_get_status = (NvFBC_GetStatusExFunctionType) GetProcAddress(m_nvfbcDll, "NvFBC_GetStatusEx");
	pfn_enable = (NvFBC_EnableFunctionType) GetProcAddress(m_nvfbcDll, "NvFBC_Enable");
	if (!pfn_create || !pfn_set_global_flags || !pfn_get_status || !pfn_enable) {
		qCritical(Q_FUNC_INFO " Failed to get NvFBC function pointers!");
		return false;
	}
	return true;
}

void NvFBCGrabber::freeScreens()
{
	for (GrabbedScreen& screen : _screensWithWidgets) {
		NvFBCScreenData* data = (NvFBCScreenData*) screen.associatedData;

		if (data->nv_fbc_to_sys)
			data->nv_fbc_to_sys->NvFBCToSysRelease();
		data->nv_fbc_to_sys = NULL;

		delete data;

		if (screen.imgData != NULL) {
			free((void*) screen.imgData);
			screen.imgData = NULL;
			screen.imgDataSize = 0;
		}
	}
	_screensWithWidgets.clear();
}

QList<ScreenInfo>* NvFBCGrabber::screensWithWidgets(QList<ScreenInfo>* result, const QList<GrabWidget*>& grabWidgets)
{
	result->clear();

	if (!m_nvfbcDll) {
		if (!init())
			return result;
	}

	for (GrabWidget* grabWidget : grabWidgets) {
		HMONITOR monitor = MonitorFromWindow(reinterpret_cast<HWND>(grabWidget->winId()), MONITOR_DEFAULTTONULL);

		if (monitor != NULL) {
			MONITORINFO monitorInfo;
			memset(&monitorInfo, 0, sizeof(MONITORINFO));
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &monitorInfo);

			LONG left = monitorInfo.rcMonitor.left;
			LONG top = monitorInfo.rcMonitor.top;
			LONG right = monitorInfo.rcMonitor.right;
			LONG bottom = monitorInfo.rcMonitor.bottom;

			ScreenInfo screenInfo;
			screenInfo.rect = QRect(left, top, right - left, bottom - top);
			screenInfo.handle = monitor;

			if (!result->contains(screenInfo))
				result->append(screenInfo);
		}
	}
	return result;
}

bool NvFBCGrabber::reallocate(const QList<ScreenInfo>& screens)
{
	freeScreens();
	BOOL error = FALSE;

	// Get d3d9 adapter count
	UINT adapterCount = 0;
	IDirect3D9 *pd3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (pd3d9) {
		adapterCount = pd3d9->GetAdapterCount();
	} else {
		qCritical(Q_FUNC_INFO " Failed to create d3d9 interface!");
	}

	for (ScreenInfo screen : screens) {
		// Get the d3d9 adapter index from the screen handle
		UINT adapterIdx = 0xFFFFFFFF;
		for (UINT i = 0; i < adapterCount; ++i) {
			HMONITOR mon = pd3d9->GetAdapterMonitor(i);

			if (mon == (HMONITOR) screen.handle) {
				adapterIdx = i;
				break;
			}
		}
		if (adapterIdx == 0xFFFFFFFF) {
			qCritical(Q_FUNC_INFO " Failed to get d3d9 adapter index from screen handle!");
			error = TRUE;
			break;
		}

		// Check NvFBC status
		NvFBCStatusEx status;
		memset(&status, 0, sizeof(status));
		status.dwVersion = NVFBC_STATUS_VER;
		status.dwAdapterIdx = adapterIdx;
		NVFBCRESULT res = pfn_get_status(&status);

		if (res != NVFBC_SUCCESS) {
			qCritical(Q_FUNC_INFO " NvFBC status error: %d", res);
			error = TRUE;
			break;
		}

		// Check if the NvFBC feature must be enabled first
		if (!status.bIsCapturePossible) {
			qInfo(Q_FUNC_INFO " NvFBC is disabled. Trying to enable it...");
			res = pfn_enable(NVFBC_STATE_ENABLE);

			if (res == NVFBC_ERROR_INSUFFICIENT_PRIVILEGES) {
				qCritical(Q_FUNC_INFO " Enabling NvFBC needs admin rights!");

				// Show one message box which tells the user to start the program as admin once.
				if (!m_admin_message_shown) {
					QTimer::singleShot(0, []() {
						QMessageBox::warning(
							NULL,
							tr("Prismatik"),
							tr("NvFBC is currently disabled and Prismatik needs administrator rights to enable it.\n"\
								"This program will not capture any screens until it is once started as an administrator with selected NvFBC grabber."),
							QMessageBox::Ok);
					});
					m_admin_message_shown = TRUE;
				}
				error = TRUE;
				break;
			}
			if (res != NVFBC_SUCCESS) {
				qCritical(Q_FUNC_INFO " Error enabling NvFBC: %d", res);
				error = TRUE;
				break;
			}
		}

		// Create the NvFBCToSys object which can capture one screen defined by adapterIdx
		NvFBCCreateParams createParams;
		memset(&createParams, 0, sizeof(createParams));
		createParams.dwVersion = NVFBC_CREATE_PARAMS_VER;
		createParams.dwInterfaceType = NVFBC_TO_SYS;
		createParams.pDevice = NULL;
		createParams.dwAdapterIdx = adapterIdx;
		createParams.pPrivateData = &Magic;
		createParams.dwPrivateDataSize = sizeof(Magic);
		res = pfn_create(&createParams);

		if (res != NVFBC_SUCCESS) {
			qCritical(Q_FUNC_INFO " Error creating NvFBC interface: %d", res);
			error = TRUE;
			break;
		}
		NvFBCToSys* nv_fbc_to_sys = (NvFBCToSys*) createParams.pNvFBC;

		// Setup the NvFBCToSys object which captures a frame in the framebuffer
		unsigned char* framebuffer = NULL;

		NVFBC_TOSYS_SETUP_PARAMS fbcSysSetupParams;
		memset(&fbcSysSetupParams, 0, sizeof(fbcSysSetupParams));
		fbcSysSetupParams.dwVersion = NVFBC_TOSYS_SETUP_PARAMS_VER;
		fbcSysSetupParams.eMode = NVFBC_TOSYS_ARGB;
		fbcSysSetupParams.bWithHWCursor = FALSE;
		fbcSysSetupParams.bDiffMap = FALSE;
		fbcSysSetupParams.ppBuffer = (void **) &framebuffer;
		fbcSysSetupParams.ppDiffMap = NULL;
		res = nv_fbc_to_sys->NvFBCToSysSetUp(&fbcSysSetupParams);

		if (res != NVFBC_SUCCESS) {
			qCritical(Q_FUNC_INFO " Error setting up NvFBCToSys: %d", res);
			error = TRUE;
			break;
		}

		// Set the data associated with this screen and allocate the buffer where we copy the framebuffer
		NvFBCScreenData* data = new NvFBCScreenData(nv_fbc_to_sys, NULL, framebuffer);
		size_t pitch = ((size_t) screen.rect.width() >> DownscaleMipLevel) * BytesPerPixel;

		GrabbedScreen grabScreen;
		memset(&grabScreen, 0, sizeof(grabScreen));
		grabScreen.screenInfo = screen;
		grabScreen.associatedData = data;
		grabScreen.imgDataSize = (screen.rect.height() >> DownscaleMipLevel) * pitch;
		grabScreen.imgFormat = BufferFormatArgb;
		grabScreen.scale = 1.0 / (1 << DownscaleMipLevel);
		grabScreen.bytesPerRow = pitch;
		grabScreen.imgData = (unsigned char*) malloc(grabScreen.imgDataSize);

		_screensWithWidgets.append(grabScreen);
	}
	pd3d9->Release();
	pd3d9 = NULL;

	if (error)
		return false;

	// Sleep so that ToSysSetUp can refresh the screen
	Sleep(100);
	return true;
}

GrabResult NvFBCGrabber::grabScreens()
{
	for (GrabbedScreen& screen : _screensWithWidgets) {
		NvFBCScreenData* data = (NvFBCScreenData*) screen.associatedData;

		// Grab one frame from a monitor into data->framebuffer
		NvFBCFrameGrabInfo frame_grab_info;

		NVFBC_TOSYS_GRAB_FRAME_PARAMS fbcSysGrabParams;
		memset(&fbcSysGrabParams, 0, sizeof(fbcSysGrabParams));
		fbcSysGrabParams.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
		fbcSysGrabParams.dwFlags = NVFBC_TOSYS_NOWAIT;
		fbcSysGrabParams.dwTargetWidth = screen.screenInfo.rect.width() >> DownscaleMipLevel;
		fbcSysGrabParams.dwTargetHeight = screen.screenInfo.rect.height() >> DownscaleMipLevel;
		fbcSysGrabParams.eGMode = NVFBC_TOSYS_SOURCEMODE_SCALE;
		fbcSysGrabParams.pNvFBCFrameGrabInfo = &frame_grab_info;
		NVFBCRESULT res = data->nv_fbc_to_sys->NvFBCToSysGrabFrame(&fbcSysGrabParams);

		if (res != NVFBC_SUCCESS) {
			qCritical(Q_FUNC_INFO " Error grabbing frame with NvFBC: %d", res);
			return GrabResultError;
		}
		data->frame_grab_info = &frame_grab_info;

		// Copy data->framebuffer to screen.imgData
		if (screen.imgData != NULL) {
			memcpy((void*) screen.imgData, data->framebuffer, screen.imgDataSize);
		} else {
			qCritical(Q_FUNC_INFO " Failed to allocate memory to copy the framebuffer!");
			return GrabResultError;
		}
	}
	return GrabResultOk;
}

#endif // WINAPI_GRAB_SUPPORT

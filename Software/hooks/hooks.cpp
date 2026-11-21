#include"hooks.h"
#include"stdio.h"
#include"shlwapi.h"

#define UINT8 unsigned char
#include "hooksutils.h"
#include "Logger.hpp"
#include "IPCContext.hpp"
#include "DxgiFrameGrabber.hpp"
#include "D3D9FrameGrabber.hpp"


HINSTANCE hDLL;
char executableName[255];

Logger *gLog = Logger::getInstance();
IPCContext *gIpcContext;
HANDLE g_syncRunMutex;


void writeBlankFrame() {
	DWORD errorcode;
	if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(gIpcContext->m_hMutex, 0))) {
		gIpcContext->m_pMemDesc->frameId = HOOKSGRABBER_BLANK_FRAME_ID;
		gIpcContext->m_pMemDesc->width = 0;
		gIpcContext->m_pMemDesc->height = 0;
		gIpcContext->m_pMemDesc->rowPitch = 0;
		ReleaseMutex(gIpcContext->m_hMutex);
	} else {
		gLog->reportLogError(L"couldn't wait mutex while writing blank frame. errocode = 0x%x", errorcode);
	}
}

WCHAR *getEventSourceName(char *executableName) {
	const size_t kBufChars = MAX_PATH + 64;
	LPWSTR wstrResult = (LPWSTR)calloc(kBufChars, sizeof(WCHAR));
	if (!wstrResult)
		return NULL;
	WCHAR wstrBuf[MAX_PATH] = {};
	mbstowcs_s(NULL, wstrBuf, MAX_PATH, executableName, _TRUNCATE);
#ifdef HOOKS_SYSWOW64
	wcscpy_s(wstrResult, kBufChars, L"prismatik-hooks32.dll ");
#else
	wcscpy_s(wstrResult, kBufChars, L"prismatik-hooks.dll ");
#endif
	wcscat_s(wstrResult, kBufChars, wstrBuf);

	return wstrResult;
}

HOOKSDLL_API BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved) {
	UNREFERENCED_PARAMETER(lpReserved);

	if (fdwReason == DLL_PROCESS_ATTACH /*|| fdwReason == DLL_THREAD_ATTACH*/) // When initializing....
	{
		DxgiFrameGrabber *dxgiFrameGrabber = NULL;
		D3D9FrameGrabber *d3d9FrameGrabber = NULL;
//		__asm__("int $3");
		// We don't need thread notifications for what we're doing.	Thus, get
		// rid of them, thereby eliminating some of the overhead of this DLL
		DisableThreadLibraryCalls(hModule);

		HMODULE hProc = GetModuleHandle(NULL);
		GetModuleFileNameA(hProc, executableName, sizeof (executableName));
		PathStripPathA(executableName);

		WCHAR *wstrEventSource = getEventSourceName(executableName);

		gLog->initLog(wstrEventSource, PRISMATIK_LOG_SEVERITY_INFO);
		gLog->reportLogInfo(L"Library initialization...");

		free(wstrEventSource);

//		void *zctx = zmq_ctx_new();
//		void *req_socket = zmq_socket(zctx, ZMQ_REQ);
//		zmq_connect(req_socket,	"");


		if (!gIpcContext)
			gIpcContext = new IPCContext(gLog);

		if (gIpcContext->init()) {

			if (NULL == (g_syncRunMutex = CreateMutex(NULL, false, NULL))) {
			}

			gLog->setLogLevel(gIpcContext->m_pConfigDesc->logLevel);

			gLog->reportLogInfo(L"d3d9 device::present(): 0x%x", gIpcContext->m_pConfigDesc->d3d9PresentFuncOffset);
			gLog->reportLogInfo(L"d3d9 swapchain::present(): 0x%x", gIpcContext->m_pConfigDesc->d3d9SCPresentFuncOffset);
			gLog->reportLogInfo(L"dxgi swapchain::present(): 0x%x", gIpcContext->m_pConfigDesc->dxgiPresentFuncOffset);

			if (!D3D9FrameGrabber::hasInstance()) {
				d3d9FrameGrabber = D3D9FrameGrabber::getInstance(g_syncRunMutex);
				d3d9FrameGrabber->setIPCContext(gIpcContext);
			} else {
				d3d9FrameGrabber = D3D9FrameGrabber::getInstance();
			}

			if (d3d9FrameGrabber->isGAPILoaded()) {
				if (!d3d9FrameGrabber->init() || !d3d9FrameGrabber->installHooks()) {
					DWORD errorcode = GetLastError();
					gLog->reportLogError(L"error occured while hijacking d3d9 0x%x", errorcode);
				} else {
					gLog->reportLogInfo(L"d3d9 hook has been installed successfully");
				}
			}

			if (!DxgiFrameGrabber::hasInstance()) {
				dxgiFrameGrabber = DxgiFrameGrabber::getInstance(g_syncRunMutex);
				dxgiFrameGrabber->setIPCContext(gIpcContext);
			} else {
				dxgiFrameGrabber = DxgiFrameGrabber::getInstance();
			}

			if(dxgiFrameGrabber->isGAPILoaded()) {
				if (!dxgiFrameGrabber->init() || !dxgiFrameGrabber->installHooks()) {
					DWORD errorcode = GetLastError();
					gLog->reportLogError(L"error occured while hijacking dxgi 0x%x", errorcode);
				} else {
					gLog->reportLogInfo(L"dxgi hook has been installed successfully");
				}
			}
		}
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		if (gLog != NULL) {
			gLog->reportLogInfo(L"detaching dll...");
			if (WAIT_OBJECT_0 == WaitForSingleObject(g_syncRunMutex, INFINITE)) {
				if (D3D9FrameGrabber::hasInstance() && D3D9FrameGrabber::getInstance()->isHooksInstalled()) {
					gLog->reportLogInfo(L"removing d3d9hooks");
					D3D9FrameGrabber::getInstance()->removeHooks();
					delete D3D9FrameGrabber::getInstance();
				}
				if (DxgiFrameGrabber::hasInstance() && DxgiFrameGrabber::getInstance()->isHooksInstalled()) {
					gLog->reportLogInfo(L"removing dxgihooks");
					DxgiFrameGrabber::getInstance()->removeHooks();
					delete DxgiFrameGrabber::getInstance();
				}

				gLog->reportLogInfo(L"clearing shared memory");
				writeBlankFrame();

				gLog->reportLogInfo(L"clearing IPC context");
				if (gIpcContext) delete gIpcContext;

				gLog->reportLogInfo(L"releasing syncRunMutex");
				ReleaseMutex(g_syncRunMutex);
			} else {
				gLog->reportLogError(L"couldn't lock syncRunMutex");
			}

			gLog->reportLogInfo(L"close syncRunMutex");
			CloseHandle(g_syncRunMutex);

			gLog->reportLogInfo(L"close log");
			gLog->closeLog();
			gLog = NULL;
		}

	}

	return TRUE;
}

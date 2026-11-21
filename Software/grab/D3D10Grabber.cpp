/*
 * D3D10Grabber.cpp
 *
 *	Created on: 29.05.2012
 *		Project: Lightpack
 *
 *	Copyright (c) 2012 Timur Sattarov
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

#include "D3D10Grabber.hpp"

#ifdef D3D10_GRAB_SUPPORT

#include <winsock2.h>
#include <shlwapi.h>
#define WINAPI_INLINE WINAPI

#include <QObject>
#include <QThread>
#include <QScreen>
#include <QGuiApplication>
#include <QMessageBox>
#include <cstdlib>
#include <stdio.h>
#include "calculations.hpp"
#include "WinUtils.hpp"
#include "../common/WinDXUtils.hpp"
#include "../../common/D3D10GrabberDefs.hpp"
#include "../src/debug.h"
#include "../libraryinjector/ILibraryInjector.h"

#include "../common/msvcstub.h"
#include <D3D10_1.h>
#include <D3D10.h>

#define SIZEOF_ARRAY(a) (sizeof(a)/sizeof(a[0]))

using namespace std;
using namespace WinUtils;

namespace {
	class D3D10GrabberWorker : public QObject {
		Q_OBJECT

	public:
		D3D10GrabberWorker(QObject *parent, LPSECURITY_ATTRIBUTES lpsa);
		bool init();
		~D3D10GrabberWorker();
	private:
		HANDLE m_frameGrabbedEvent;
	signals:
		void frameGrabbed();
	public slots:
		void runLoop();

	public:
		bool stop = false;
	private:
		LPSECURITY_ATTRIBUTES m_lpsa;
	};

	const unsigned kBytesPerPixel = 4;

	D3D10GrabberWorker::D3D10GrabberWorker(QObject *parent, LPSECURITY_ATTRIBUTES lpsa) :
		QObject(parent),
		m_lpsa(lpsa)
	{
	}

	bool D3D10GrabberWorker::init() {
		SetLastError(0);
		m_frameGrabbedEvent = CreateEventW(m_lpsa, FALSE, FALSE, HOOKSGRABBER_FRAMEGRABBED_EVENT_NAME);
		if (NULL == m_frameGrabbedEvent || GetLastError() == ERROR_ALREADY_EXISTS) {
			qCritical() << Q_FUNC_INFO << "unable to create exclusive frameGrabbedEvent, error:" << GetLastError();
			if (m_frameGrabbedEvent) {
				CloseHandle(m_frameGrabbedEvent);
				m_frameGrabbedEvent = NULL;
			}
			return false;
		}

		return true;
	}

	D3D10GrabberWorker::~D3D10GrabberWorker() {
		if (m_frameGrabbedEvent)
			CloseHandle(m_frameGrabbedEvent);
	}

	void D3D10GrabberWorker::runLoop() {
		while (!stop) {
			if (WAIT_OBJECT_0 == WaitForSingleObject(m_frameGrabbedEvent, 50)) {
				emit frameGrabbed();
				if (!ResetEvent(m_frameGrabbedEvent)) {
					qCritical() << Q_FUNC_INFO << "couldn't reset frameGrabbedEvent";
				}
			}
		}
	}
} // namespace

namespace {
	class D3D10GrabberInjector : public QObject {
		Q_OBJECT

	public:
		D3D10GrabberInjector(QObject *parent, bool injectD3D9);
		bool init();
		~D3D10GrabberInjector();
	public slots:
		void infectCleanDxProcesses();
	public:
		void sanitizeProcesses();
	private:
		QList<DWORD> m_lastSeenDxProcesses;
		ILibraryInjector * m_libraryInjector;
		WCHAR m_hooksLibPath[300];
		WCHAR m_unhookLibPath[300];
		WCHAR m_systemrootPath[300];
		bool m_injectD3D9;
	};

	D3D10GrabberInjector::D3D10GrabberInjector(QObject *parent, bool injectD3D9) : QObject(parent),
		m_libraryInjector(NULL),
		m_injectD3D9(injectD3D9)
	{
	}

	bool D3D10GrabberInjector::init() {
		AcquirePrivileges();

		DWORD len = GetModuleFileNameW(NULL, m_hooksLibPath, SIZEOF_ARRAY(m_hooksLibPath));
		if (len == 0 || len >= SIZEOF_ARRAY(m_hooksLibPath))
			return false;
		PathRemoveFileSpecW(m_hooksLibPath);
		if (wcscat_s(m_hooksLibPath, SIZEOF_ARRAY(m_hooksLibPath), L"\\") != 0 ||
			wcscat_s(m_hooksLibPath, SIZEOF_ARRAY(m_hooksLibPath), lightpackHooksDllName) != 0)
			return false;

		len = GetModuleFileNameW(NULL, m_unhookLibPath, SIZEOF_ARRAY(m_unhookLibPath));
		if (len == 0 || len >= SIZEOF_ARRAY(m_unhookLibPath))
			return false;
		PathRemoveFileSpecW(m_unhookLibPath);
		if (wcscat_s(m_unhookLibPath, SIZEOF_ARRAY(m_unhookLibPath), L"\\") != 0 ||
			wcscat_s(m_unhookLibPath, SIZEOF_ARRAY(m_unhookLibPath), lightpackUnhookDllName) != 0)
			return false;

		if (!GetWindowsDirectoryW(m_systemrootPath, SIZEOF_ARRAY(m_systemrootPath)))
			return false;

		HRESULT hr = CoInitialize(0);
		//		hr = InitComSecurity();
		//		hr = CoCreateInstanceAsAdmin(NULL, CLSID_ILibraryInjector, IID_ILibraryInjector, reinterpret_cast<void **>(&m_libraryInjector));
		hr = CoCreateInstance(CLSID_ILibraryInjector, NULL, CLSCTX_INPROC_SERVER, IID_ILibraryInjector, reinterpret_cast<void **>(&m_libraryInjector));
		if (FAILED(hr)) {
			qCritical() << Q_FUNC_INFO << "Can't create libraryinjector. D3D10Grabber wasn't initialised. Please try to register server: regsvr32 libraryinjector.dll";
			CoUninitialize();
			return false;
		}

		// Remove any injections from previous runs
		sanitizeProcesses();

		return true;
	}

	D3D10GrabberInjector::~D3D10GrabberInjector() {
		if (m_libraryInjector) {
			m_libraryInjector->Release();
			CoUninitialize();
		}
	}

	void D3D10GrabberInjector::infectCleanDxProcesses() {
		QList<DWORD> processes = QList<DWORD>();
		if (m_injectD3D9)
			getDxProcessesIDs(&processes, m_systemrootPath);
		else
			getDxgiProcessesIDs(&processes, m_systemrootPath);
		foreach(DWORD procId, processes) {
			// Require the process to have run for at least one full timer tick,
			// hoping when injection happens their swapchain is already setup
			if (m_lastSeenDxProcesses.contains(procId)) {
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Infecting DX process " << procId;
				m_libraryInjector->Inject(procId, m_hooksLibPath);
			}
		}
		m_lastSeenDxProcesses.clear();
		m_lastSeenDxProcesses.append(processes);
	}

	void D3D10GrabberInjector::sanitizeProcesses() {
		QList<DWORD> processes = QList<DWORD>();
		getHookedProcessesIDs(&processes, m_systemrootPath);
		foreach(DWORD procId, processes) {
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Sanitizing process " << procId;
			m_libraryInjector->Inject(procId, m_unhookLibPath);
		}
	}

} // namespace

class D3D10GrabberImpl: public QObject
{
	Q_OBJECT

public:
	D3D10GrabberImpl(D3D10Grabber &owner, GrabberContext *context, GetHwndCallback_t getHwndCb, bool injectD3D9)
		: m_configMem(NULL),
			m_configMap(NULL),
			m_sharedMem(NULL),
			m_mutex(NULL),
			m_isStarted(false),
			m_memMap(NULL),
			m_lastFrameId(0),
			m_isInited(false),
			m_isFrameGrabbedDuringLastSecond(false),
			m_context(context),
			m_getHwndCb(getHwndCb),
			m_owner(owner),
			m_injectD3D9(injectD3D9)
	{
		memset(&m_configDesc, 0, sizeof(m_configDesc));
		memset(&m_memDesc, 0, sizeof(m_memDesc));
	}

	~D3D10GrabberImpl()
	{
		shutdown();
	}

	bool initIPC() {
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (result != 0) {
			qCritical() << Q_FUNC_INFO << "can't initialize winsocks2. error code " << result;
			return false;
		}
		if (!LoadD3DandDXGI()) {
			qCritical() << Q_FUNC_INFO << "D3D10 and DXGI initialization failed ";
			return false;
		}

		PSECURITY_DESCRIPTOR pConfigSD = BuildHookObjectSD(FILE_MAP_READ | READ_CONTROL, true);
		PSECURITY_DESCRIPTOR pFrameSD = BuildHookObjectSD(FILE_MAP_READ | FILE_MAP_WRITE | READ_CONTROL, false);
		PSECURITY_DESCRIPTOR pMutexSD = BuildHookObjectSD(SYNCHRONIZE | MUTEX_MODIFY_STATE | READ_CONTROL, false);
		if (!pConfigSD || !pFrameSD || !pMutexSD) {
			qCritical() << Q_FUNC_INFO << "couldn't build IPC security descriptors";
			FreeHookObjectSD(pConfigSD);
			FreeHookObjectSD(pFrameSD);
			FreeHookObjectSD(pMutexSD);
			return false;
		}

		SECURITY_ATTRIBUTES configSa = { sizeof(SECURITY_ATTRIBUTES), pConfigSD, FALSE };
		SECURITY_ATTRIBUTES frameSa = { sizeof(SECURITY_ATTRIBUTES), pFrameSD, FALSE };
		SECURITY_ATTRIBUTES mutexSa = { sizeof(SECURITY_ATTRIBUTES), pMutexSD, FALSE };

		bool ok = initConfigMemory(&configSa) && initSharedMemory(&frameSa) && mapFile() && createMutex(&mutexSa);

		FreeHookObjectSD(pConfigSD);
		FreeHookObjectSD(pFrameSD);
		FreeHookObjectSD(pMutexSD);

		if (!ok) {
			freeIPC();
			return false;
		}

		HOOKSGRABBER_CONFIG_DESC configDesc;
		memset(&configDesc, 0, sizeof(configDesc));
		if (!fillConfigDesc(&configDesc)) {
			qWarning() << Q_FUNC_INFO << "couldn't fill HOOKSGRABBER_CONFIG_DESC";
			freeIPC();
			return false;
		}
		memcpy(m_configMap, &configDesc, sizeof(configDesc));
		memcpy(&m_configDesc, &configDesc, sizeof(configDesc));

		HOOKSGRABBER_SHARED_MEM_DESC memDesc;
		memset(&memDesc, 0, sizeof(memDesc));
		memDesc.frameId = HOOKSGRABBER_BLANK_FRAME_ID;
		memcpy(m_memMap, &memDesc, sizeof(memDesc));
		memcpy(&m_memDesc, &memDesc, sizeof(memDesc));

		return true;
	}

	bool init() {
		if(m_isInited)
			return true;

		if (!initIPC()) {
			freeIPC();
			qCritical() << Q_FUNC_INFO << "Can't init interprocess communication mechanism. D3D10Grabber wasn't initialised.";
			return false;
		}

#ifdef _WIN64 // Find the required 32-bit addresses via offsetfinder
		WCHAR path[MAX_PATH + 32];

		DWORD pathLen = GetModuleFileNameW(NULL, path, SIZEOF_ARRAY(path));
		if (pathLen == 0 || pathLen >= SIZEOF_ARRAY(path)) {
			freeIPC();
			return false;
		}
		PathRemoveFileSpecW(path);
		if (wcscat_s(path, SIZEOF_ARRAY(path), L"\\") != 0 ||
			wcscat_s(path, SIZEOF_ARRAY(path), lightpackOffsetFinderName) != 0) {
			freeIPC();
			return false;
		}

		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));
		if (!CreateProcessW(path,	// Module name
			NULL,			// Command line
			NULL,			// Process handle not inheritable
			NULL,			// Thread handle not inheritable
			FALSE,			// Set handle inheritance to FALSE
			0,				// No creation flags
			NULL,			// Use parent's environment block
			NULL,			// Use parent's starting directory
			&si,			// Pointer to STARTUPINFO structure
			&pi)			// Pointer to PROCESS_INFORMATION structure
			) {
			qCritical(Q_FUNC_INFO " Can't get 32bit offsets, failed to run offsetfinder: %x. D3D10Grabber wasn't initialised.", GetLastError());
			freeIPC();
			return false;
		}

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode = -1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (exitCode) {
			qCritical(Q_FUNC_INFO " Can't get 32bit offsets, offsetfinder returned %x. D3D10Grabber wasn't initialised.", exitCode);
			freeIPC();
			return false;
		}

#endif

		m_injectorThread.reset(new QThread());
		m_injector.reset(new D3D10GrabberInjector(NULL, m_injectD3D9));
		if (!m_injector->init()) {
			qCritical(Q_FUNC_INFO " Init D3D10GrabberInjector failed. D3D10Grabber wasn't initialised.");
			m_injector.reset();
			freeIPC();
			return false;
		}
		m_injector->moveToThread(m_injectorThread.data());
		m_injectorThread->start();

		PSECURITY_DESCRIPTOR pEventSD = BuildHookObjectSD(SYNCHRONIZE | EVENT_MODIFY_STATE | READ_CONTROL, false);
		if (!pEventSD) {
			m_injector.reset();
			m_injectorThread->quit();
			m_injectorThread->wait();
			m_injectorThread.reset();
			freeIPC();
			return false;
		}
		SECURITY_ATTRIBUTES eventSa = { sizeof(SECURITY_ATTRIBUTES), pEventSD, FALSE };

		m_workerThread.reset(new QThread());
		m_worker.reset(new D3D10GrabberWorker(NULL, &eventSa));
		const bool workerOk = m_worker->init();
		FreeHookObjectSD(pEventSD);
		if (!workerOk) {
			qCritical(Q_FUNC_INFO " Init D3D10GrabberWorker failed. D3D10Grabber wasn't initialised.");
			m_worker.reset();
			m_injector.reset();
			m_injectorThread->quit();
			m_injectorThread->wait();
			m_injectorThread.reset();
			freeIPC();
			return false;
		}
		m_worker->moveToThread(m_workerThread.data());
		m_workerThread->start();
		connect(m_worker.data(), &D3D10GrabberWorker::frameGrabbed, this, &D3D10GrabberImpl::frameGrabbed, Qt::QueuedConnection);
		QMetaObject::invokeMethod(m_worker.data(), "runLoop", Qt::QueuedConnection);

		using namespace std::chrono_literals;
		m_processesScanAndInfectTimer.reset(new QTimer(this));
		m_processesScanAndInfectTimer->setInterval(5s);
		m_processesScanAndInfectTimer->setSingleShot(false);
		connect(m_processesScanAndInfectTimer.data(), &QTimer::timeout, m_injector.data(), &D3D10GrabberInjector::infectCleanDxProcesses);
		m_processesScanAndInfectTimer->start();

		m_checkIfFrameGrabbedTimer.reset(new QTimer(this));
		m_checkIfFrameGrabbedTimer->setSingleShot(false);
		m_checkIfFrameGrabbedTimer->setInterval(1s);
		connect(m_checkIfFrameGrabbedTimer.data(), &QTimer::timeout, this, &D3D10GrabberImpl::handleIfFrameGrabbed);
		m_checkIfFrameGrabbedTimer->start();
		m_isInited = true;
		return m_isInited;
	}

	void start()
	{
		if (!m_isInited) {
			emit m_owner.grabberStateChangeRequested(false);
			return;
		}
		m_isStarted = true;
		if (m_configMap) {
			DWORD errorcode;
			if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(m_mutex, INFINITE))) {
				memcpy(&m_configDesc, m_configMap, sizeof(m_configDesc));
				m_configDesc.grabbingStarted = TRUE;
				memcpy(m_configMap, &m_configDesc, sizeof(m_configDesc));
				ReleaseMutex(m_mutex);
			} else {
				qWarning() << Q_FUNC_INFO << "couldn't start grabbing: " << errorcode;
			}
		}
	}

	void stop()
	{
		m_isStarted = false;
		if (m_configMap) {
			DWORD errorcode;
			if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(m_mutex, INFINITE))) {
				memcpy(&m_configDesc, m_configMap, sizeof(m_configDesc));
				m_configDesc.grabbingStarted = FALSE;
				memcpy(m_configMap, &m_configDesc, sizeof(m_configDesc));
				ReleaseMutex(m_mutex);
			} else {
				qWarning() << Q_FUNC_INFO << "couldn't stop grabbing: " << errorcode;
			}
		}
	}

	bool isStarted() const { return m_isStarted; }

	void setGrabInterval(int msec) {
		if (msec < 0)
			msec = 0;
		if (m_configMap) {
			DWORD errorcode;
			if (WAIT_OBJECT_0 == (errorcode = WaitForSingleObject(m_mutex, INFINITE))) {
				memcpy(&m_configDesc, m_configMap, sizeof(m_configDesc));
				m_configDesc.grabDelay = static_cast<UINT>(msec);
				memcpy(m_configMap, &m_configDesc, sizeof(m_configDesc));
				ReleaseMutex(m_mutex);
			} else {
				qWarning(Q_FUNC_INFO " couldn't set grab interval: ", errorcode);
			}
		}
	}

	QList< ScreenInfo > * screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets)
	{
		Q_UNUSED(grabWidgets);

		result->clear();
		return result;
	}

	GrabResult grabScreens(QList<GrabbedScreen> &grabbedScreens)
	{
		if (!m_isInited || !m_memMap || grabbedScreens.isEmpty()) {
			return GrabResultFrameNotReady;
		}

		HOOKSGRABBER_SHARED_MEM_DESC localDesc;
		memcpy(&localDesc, m_memMap, sizeof(localDesc));
		m_memDesc = localDesc;

		if (localDesc.frameId == HOOKSGRABBER_BLANK_FRAME_ID || localDesc.frameId == m_lastFrameId) {
			return GrabResultFrameNotReady;
		}
		m_lastFrameId = localDesc.frameId;

		const UINT expectedRowBytes = localDesc.width * kBytesPerPixel;
		if (!hooksGrabberValidateFrameSize(localDesc.width, localDesc.height, expectedRowBytes)) {
			qCritical() << Q_FUNC_INFO << "rejected invalid frame dimensions from shared memory:" << localDesc.width << "x" << localDesc.height;
			return GrabResultError;
		}

		switch (localDesc.format) {
		case BufferFormatArgb:
		case BufferFormatAbgr:
		case BufferFormatRgba:
		case BufferFormatBgra:
		case BufferFormatRgbg:
			break;
		default:
			qWarning() << Q_FUNC_INFO << "unsupported buffer format in shared memory:" << localDesc.format;
			return GrabResultError;
		}

		if (static_cast<int>(localDesc.width) != grabbedScreens[0].screenInfo.rect.width() ||
			static_cast<int>(localDesc.height) != grabbedScreens[0].screenInfo.rect.height())
		{
			qCritical() << Q_FUNC_INFO << "illegal state, screens don't match: qt " <<
							grabbedScreens[0].screenInfo.rect << ", remote proc's " <<
							localDesc.width << "x" << localDesc.height;
			return GrabResultError;
		}

		grabbedScreens[0].imgData = reinterpret_cast<unsigned char *>(m_memMap) + sizeof(HOOKSGRABBER_SHARED_MEM_DESC);
		grabbedScreens[0].imgDataSize = static_cast<size_t>(localDesc.height) * static_cast<size_t>(localDesc.width) * kBytesPerPixel;
		grabbedScreens[0].bytesPerRow = expectedRowBytes;
		grabbedScreens[0].imgFormat = localDesc.format;
		m_isFrameGrabbedDuringLastSecond = true;
		return GrabResultOk;
	}
signals:
	void frameGrabbed();

private slots:
	void handleIfFrameGrabbed() {
		if (!m_isFrameGrabbedDuringLastSecond) {
			if (m_isStarted) {
				emit m_owner.grabberStateChangeRequested(false);
			}
		} else {
			m_isFrameGrabbedDuringLastSecond = false;
		}
	}

private:

	void freeIPC()
	{
		if (m_configMap) {
			UnmapViewOfFile(m_configMap);
			m_configMap = NULL;
		}

		if (m_configMem) {
			CloseHandle(m_configMem);
			m_configMem = NULL;
		}

		if(m_memMap) {
			UnmapViewOfFile(m_memMap);
			m_memMap = NULL;
		}

		if(m_sharedMem) {
			CloseHandle(m_sharedMem);
			m_sharedMem = NULL;
		}

		if(m_mutex) {
			CloseHandle(m_mutex);
			m_mutex = NULL;
		}
	}

	bool initConfigMemory(LPSECURITY_ATTRIBUTES lpsa)
	{
		SetLastError(0);
		m_configMem = CreateFileMappingW(INVALID_HANDLE_VALUE, lpsa, PAGE_READWRITE, 0, static_cast<DWORD>(HOOKSGRABBER_CONFIG_MEM_SIZE), HOOKSGRABBER_CONFIG_MEM_NAME);
		if (!m_configMem || GetLastError() == ERROR_ALREADY_EXISTS) {
			qCritical() << Q_FUNC_INFO << "couldn't create exclusive config mapping, error code " << GetLastError();
			if (m_configMem) {
				CloseHandle(m_configMem);
				m_configMem = NULL;
			}
			return false;
		}

		m_configMap = MapViewOfFile(m_configMem, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, HOOKSGRABBER_CONFIG_MEM_SIZE);
		if (!m_configMap) {
			qCritical() << Q_FUNC_INFO << "couldn't map config memory, error code " << GetLastError();
			return false;
		}
		return true;
	}

	bool initSharedMemory(LPSECURITY_ATTRIBUTES lpsa)
	{
		SetLastError(0);
		m_sharedMem = CreateFileMappingW(INVALID_HANDLE_VALUE, lpsa, PAGE_READWRITE, 0, HOOKSGRABBER_SHARED_MEM_SIZE, HOOKSGRABBER_SHARED_MEM_NAME);
		if (!m_sharedMem || GetLastError() == ERROR_ALREADY_EXISTS) {
			qCritical() << Q_FUNC_INFO << "couldn't create exclusive frame mapping, error code " << GetLastError();
			if (m_sharedMem) {
				CloseHandle(m_sharedMem);
				m_sharedMem = NULL;
			}
			return false;
		}

		return true;
	}

	bool mapFile()
	{
		if (!m_sharedMem)
			return false;

		m_memMap = MapViewOfFile(m_sharedMem, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, HOOKSGRABBER_SHARED_MEM_SIZE );
		if (!m_memMap)
			qWarning() << Q_FUNC_INFO << "couldn't map shared memory. error code " << GetLastError();

		return NULL != m_memMap;
	}

	bool fillConfigDesc(HOOKSGRABBER_CONFIG_DESC *configDesc)
	{
		if (!m_getHwndCb) {
			qWarning() << Q_FUNC_INFO << "m_getHwndCb is NULL!";
			return false;
		}

		HWND wnd = (HWND)m_getHwndCb();
		configDesc->d3d9PresentFuncOffset = GetD3D9PresentOffset(wnd);
		configDesc->d3d9SCPresentFuncOffset = GetD3D9SCPresentOffset(wnd);
		configDesc->d3d9ResetFuncOffset = GetD3D9ResetOffset(wnd);
		configDesc->dxgiPresentFuncOffset = GetDxgiPresentOffset(wnd);

		//converting logLevel from our app's level to EventLog's level
		configDesc->logLevel = Debug::HighLevel - g_debugLevel;
		configDesc->grabDelay = 50; // will be overriden by setGrabInterval
		configDesc->grabbingStarted = FALSE;
		return true;
	}

	bool createMutex(LPSECURITY_ATTRIBUTES lpsa)
	{
		SetLastError(0);
		m_mutex = CreateMutexW(lpsa, FALSE, HOOKSGRABBER_MUTEX_MEM_NAME);
		if (!m_mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
			qCritical() << Q_FUNC_INFO << "couldn't create exclusive mutex, error code " << GetLastError();
			if (m_mutex) {
				CloseHandle(m_mutex);
				m_mutex = NULL;
			}
			return false;
		}

		return true;
	}

	void shutdown()
	{
		if (!m_isInited)
			return;

		disconnect(m_worker.data());
		m_worker->stop = true;

		m_injector->sanitizeProcesses();

		m_workerThread->quit();
		m_injectorThread->quit();
		m_workerThread->wait();
		m_injectorThread->wait();


		disconnect(this, &D3D10GrabberImpl::handleIfFrameGrabbed, nullptr, nullptr);
		freeIPC();
		CoUninitialize();
		m_isInited = false;
	}

	HANDLE m_configMem;
	PVOID m_configMap;
	HOOKSGRABBER_CONFIG_DESC m_configDesc;
	HANDLE m_sharedMem;
	HANDLE m_mutex;
	bool m_isStarted;
	PVOID m_memMap;
	UINT m_lastFrameId;
	MONITORINFO m_monitorInfo;
	QScopedPointer<QTimer> m_processesScanAndInfectTimer;
	bool m_isInited;
	bool m_isFrameGrabbedDuringLastSecond;
	GrabberContext *m_context;
	GetHwndCallback_t m_getHwndCb;

	QScopedPointer<QTimer> m_checkIfFrameGrabbedTimer;
	QScopedPointer<D3D10GrabberWorker> m_worker;
	QScopedPointer<QThread> m_workerThread;
	QScopedPointer<D3D10GrabberInjector> m_injector;
	QScopedPointer<QThread> m_injectorThread;
	HOOKSGRABBER_SHARED_MEM_DESC m_memDesc;
	D3D10Grabber &m_owner;
	bool m_injectD3D9;
};

// This will force qmake and moc to process internal classes in this file.
#include "D3D10Grabber.moc"

D3D10Grabber::D3D10Grabber(QObject *parent, GrabberContext *context, GetHwndCallback_t getHwndCb, bool injectD3D9)
	: GrabberBase(parent, context) {
	m_impl.reset(new D3D10GrabberImpl(*this, context, getHwndCb, injectD3D9));
}

void D3D10Grabber::init() {
	m_impl->init();
	connect(m_impl.data(), &D3D10GrabberImpl::frameGrabbed, this, &D3D10Grabber::grab);
	_screensWithWidgets.clear();
	GrabbedScreen grabbedScreen;
	HMONITOR hPrimaryMon = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi;
	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	if (hPrimaryMon && GetMonitorInfo(hPrimaryMon, &mi)) {
		grabbedScreen.screenInfo.handle = hPrimaryMon;
		grabbedScreen.screenInfo.rect = QRect(mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
	} else if (QScreen *primary = QGuiApplication::primaryScreen()) {
		const qreal dpr = primary->devicePixelRatio();
		grabbedScreen.screenInfo.handle = primary;
		grabbedScreen.screenInfo.rect = QRect(primary->geometry().topLeft(), QSize(qRound(primary->geometry().width() * dpr), qRound(primary->geometry().height() * dpr)));
	}
	_screensWithWidgets.append(grabbedScreen);

	if (!WinUtils::IsUserAdmin()) {
		qWarning() << Q_FUNC_INFO << "DX hooking is enabled but application not running elevated";
		// Do not show the message box during initialization (creating a unwanted message loop)
		// Show as soon as the message loop is established
		QTimer::singleShot(0, this, &D3D10Grabber::showAdminMessage);
	}
}

void D3D10Grabber::showAdminMessage() {
	QMessageBox::warning(
		NULL,
		tr("Prismatik"),
		tr("DX hooking is enabled but will not function properly because the application is not running as local Administrator.\n"\
		"You can either disable DX hooking or run this program as an Administrator."),
		QMessageBox::Ok);
}

void D3D10Grabber::startGrabbing() {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	m_impl->start();
}

void D3D10Grabber::stopGrabbing() {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	m_impl->stop();
}

void D3D10Grabber::setGrabInterval(int msec) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	m_impl->setGrabInterval(msec);
}

/*!
 * Just stub, we don't need to reallocate anything, and we suppose fullscreen application
 * runs on primary screen \see D3D10Grabber#init()
 * \param result
 * \param grabWidgets
 * \return
 */
QList< ScreenInfo > * D3D10Grabber::screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets)
{
	Q_UNUSED(grabWidgets);

	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	return result;
}

/*!
 * Just stub. Reallocations are unnecessary here.
 * \param grabScreens
 * \return
 */
bool D3D10Grabber::reallocate(const QList<ScreenInfo> &grabScreens)
{
	Q_UNUSED(grabScreens);

	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	return true;
}

GrabResult D3D10Grabber::grabScreens()
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	if (m_impl && m_impl->isStarted()) {
		GrabResult result = m_impl->grabScreens(_screensWithWidgets);
		return result;
	} else {
		emit grabberStateChangeRequested(true);
		return GrabResultFrameNotReady;
	}
}

D3D10Grabber::~D3D10Grabber() {
}

bool D3D10Grabber::isGrabbingStarted() const {
	return m_impl->isStarted();
}

#endif

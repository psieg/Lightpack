/*
 * WinUtils.cpp
 *
 *	Created on: 25.07.11
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Timur Sattarov, Mike Shatohin
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

#include "WinUtils.hpp"

#ifdef NIGHTLIGHT_SUPPORT
#include "NightLightLibrary.h"
#include "PrismatikMath.hpp"
#endif // NIGHTLIGHT_SUPPORT

#include <psapi.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <sddl.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

#include "../src/debug.h"

#define SIZEOF_ARRAY(a) (sizeof(a)/sizeof(a[0]))

namespace WinUtils
{

const WCHAR lightpackHooksDllName[] = L"prismatik-hooks.dll";
const WCHAR lightpackUnhookDllName[] = L"prismatik-unhook.dll";
#ifdef _WIN64
const WCHAR lightpackHooksDllName32[] = L"prismatik-hooks32.dll";
const WCHAR lightpackOffsetFinderName[] = L"offsetfinder.exe";
#endif
static LPCWSTR pwstrExcludeProcesses[] = {
	// Windows
	L"dwm.exe", L"ShellExperienceHost.exe", L"ApplicationFrameHost.exe", L"LockAppHost.exe", L"explorer.exe", L"SearchUI.exe",
	L"svchost.exe", L"lsass.exe", L"fontdrvhost.exe", L"winlogon.exe", L"dllhost.exe",
	// Graphics Drivers
	L"igfxEM.exe", L"igfxTray.exe", L"nvxdsync.exe", L"nvvsvc.exe",
	// Browsers
	L"chrome.exe", L"firefox.exe", L"iexplore.exe", L"MicrosoftEdgeCP.exe",
	// Apps
	L"skype.exe", L"SkypeHost.exe", L"qtcreator.exe", L"devenv.exe", L"thunderbird.exe", L"Steam.exe"
};
static LPCWSTR pwstrDxModules[] = { L"d3d9.dll", L"dxgi.dll" };
static LPCWSTR pwstrDxgiModules[] = { L"dxgi.dll" };
static LPCWSTR pwstrHookModules[] = { L"prismatik-hooks.dll", L"prismatik-hooks32.dll" };


struct handle_data {
	unsigned long process_id;
	HWND best_handle;
};

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id) {
		return TRUE;
	}
	data.best_handle = handle;
	return FALSE;
}

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.best_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.best_handle;
}

BOOL SetPrivilege(HANDLE hToken, LPCTSTR szPrivName, BOOL fEnable) {

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	LookupPrivilegeValue(NULL, szPrivName, &tp.Privileges[0].Luid);
	tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof (tp), NULL, NULL);
	return ((GetLastError() == ERROR_SUCCESS));

}

BOOL AcquirePrivileges() {

	HANDLE hCurrentProc = GetCurrentProcess();
	HANDLE hCurrentProcToken = NULL;
	BOOL ok = FALSE;

	if (OpenProcessToken(hCurrentProc, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hCurrentProcToken)) {
		if (SetPrivilege(hCurrentProcToken, SE_DEBUG_NAME, TRUE)) {
			ok = TRUE;
		}
		CloseHandle(hCurrentProcToken);
	}
	return ok;
}

BOOL IsUserAdmin() {
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa376389%28v=vs.85%29.aspx

	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}

	return(b);
}

QList<DWORD> * getProcessesIDs(QList<DWORD> * processes, LPCWSTR withModule[], UINT withModuleCount, LPCWSTR withoutModule[], UINT withoutModuleCount, LPCWSTR wstrSystemRootPath, BOOL requireWindow) {

	DWORD aProcesses[1024];
	HMODULE hMods[1024];
	DWORD cbNeeded;
	DWORD cProcesses;
	char debug_buf_process[255];
	//char debug_buf_module[255];
	WCHAR executableName[MAX_PATH];
	unsigned int i;

	DEBUG_MID_LEVEL << Q_FUNC_INFO << "scanning";

	//		Get the list of process identifiers.
	processes->clear();

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return NULL;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the names of the modules for each process.

	for (i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != GetCurrentProcessId()) {
			HANDLE hProcess;
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
				PROCESS_VM_READ,
				FALSE, aProcesses[i]);
			if (NULL == hProcess)
				goto nextProcess;

			if (!GetModuleFileNameExW(hProcess, 0, executableName, SIZEOF_ARRAY(executableName))) {
				goto nextProcess;
			}

			if (wcsstr(executableName, wstrSystemRootPath) != NULL) {
				goto nextProcess;
			}

			PathStripPathW(executableName);

			::WideCharToMultiByte(CP_ACP, 0, executableName, -1, debug_buf_process, 255, NULL, NULL);
			DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "evaluating process" << debug_buf_process;

			for (unsigned k = 0; k < SIZEOF_ARRAY(pwstrExcludeProcesses); k++) {
				if (wcsicmp(executableName, pwstrExcludeProcesses[k]) == 0) {
					DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "skipping" << debug_buf_process;
					goto nextProcess;
				}
			}

			// Get a list of all the modules in this process.
#ifdef _WIN64
			if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
#else
			if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
#endif
			{
				bool isModulePresent = false;
				for (DWORD j = 0; j < (cbNeeded / sizeof(HMODULE)); j++)
				{
					WCHAR szModName[MAX_PATH];

					if (GetModuleFileNameExW(hProcess, hMods[j], szModName,
						sizeof(szModName) / sizeof(WCHAR)))
					{

						PathStripPathW(szModName);
						//::WideCharToMultiByte(CP_ACP, 0, szModName, -1, debug_buf_module, 255, NULL, NULL);
						//DEBUG_HIGH_LEVEL << Q_FUNC_INFO << debug_buf_process << "has module" << debug_buf_module;

						for (unsigned k = 0; k < withoutModuleCount; k++) {
							if (wcsicmp(szModName, withoutModule[k]) == 0) {
								goto nextProcess;
							}
						}

						for (unsigned k = 0; k < withModuleCount; k++) {
							if (wcsicmp(szModName, withModule[k]) == 0) {
								isModulePresent = true;
								break;
							}
						}
					}
				}
				if (isModulePresent) {
					if (requireWindow) {
						HWND wnd = find_main_window(aProcesses[i]);
						if (wnd) {
							int w = GetSystemMetrics(SM_CXSCREEN);
							int h = GetSystemMetrics(SM_CYSCREEN);
							RECT rcWindow;
							GetWindowRect(wnd, &rcWindow);
							if ((w == (rcWindow.right - rcWindow.left)) &&
								(h == (rcWindow.bottom - rcWindow.top))) {

								DEBUG_MID_LEVEL << Q_FUNC_INFO << debug_buf_process << "has required module and fullscreen window";
								processes->append(aProcesses[i]);
							} else {
								DEBUG_MID_LEVEL << Q_FUNC_INFO << debug_buf_process << "has required module and non-fullscreen window";
							}
						} else {
							DEBUG_MID_LEVEL << Q_FUNC_INFO << debug_buf_process << "has required module and no window";
						}
					}
					else {
						DEBUG_MID_LEVEL << Q_FUNC_INFO << debug_buf_process << "has required module";
						processes->append(aProcesses[i]);
					}

				}

			} else {
				DEBUG_HIGH_LEVEL << Q_FUNC_INFO << debug_buf_process << "module enum failed:" << GetLastError();
			}
		nextProcess:
			// Release the handle to the process.
			CloseHandle(hProcess);
		}
	}

	return processes;
}

QList<DWORD> * getDxProcessesIDs(QList<DWORD> * processes, LPCWSTR wstrSystemRootPath) {
	return getProcessesIDs(processes, pwstrDxModules, SIZEOF_ARRAY(pwstrDxModules), pwstrHookModules, SIZEOF_ARRAY(pwstrHookModules), wstrSystemRootPath, true);
}

QList<DWORD> * getDxgiProcessesIDs(QList<DWORD> * processes, LPCWSTR wstrSystemRootPath) {
	return getProcessesIDs(processes, pwstrDxgiModules, SIZEOF_ARRAY(pwstrDxgiModules), pwstrHookModules, SIZEOF_ARRAY(pwstrHookModules), wstrSystemRootPath, true);
}

QList<DWORD> * getHookedProcessesIDs(QList<DWORD> * processes, LPCWSTR wstrSystemRootPath) {
	return getProcessesIDs(processes, pwstrHookModules, SIZEOF_ARRAY(pwstrHookModules), NULL, 0, wstrSystemRootPath, false);
}


static QString getProcessTokenUserSidString(HANDLE hProcess) {
	HANDLE hToken = NULL;
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		return QString();

	DWORD needed = 0;
	GetTokenInformation(hToken, TokenUser, NULL, 0, &needed);
	if (needed == 0) {
		CloseHandle(hToken);
		return QString();
	}

	QByteArray buf(static_cast<int>(needed), 0);
	QString result;
	if (GetTokenInformation(hToken, TokenUser, buf.data(), needed, &needed)) {
		const TOKEN_USER *tokenUser = reinterpret_cast<const TOKEN_USER *>(buf.constData());
		LPWSTR strSid = NULL;
		if (ConvertSidToStringSidW(tokenUser->User.Sid, &strSid) && strSid) {
			result = QString::fromWCharArray(strSid);
			LocalFree(strSid);
		}
	}
	CloseHandle(hToken);
	return result;
}

PSECURITY_DESCRIPTOR BuildHookObjectSD(DWORD interactiveUserAccessMask, bool highIntegrityNoWriteUp) {
	const QString currentSid = getProcessTokenUserSidString(GetCurrentProcess());
	if (currentSid.isEmpty())
		return NULL;

	QString shellSid;
	HWND hShellWnd = GetShellWindow();
	if (hShellWnd) {
		DWORD shellPid = 0;
		GetWindowThreadProcessId(hShellWnd, &shellPid);
		if (shellPid != 0) {
			HANDLE hShellProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, shellPid);
			if (hShellProc) {
				shellSid = getProcessTokenUserSidString(hShellProc);
				CloseHandle(hShellProc);
			}
		}
	}

	const BOOL isAdmin = IsUserAdmin();
	const QString hexMask = QStringLiteral("0x%1").arg(static_cast<quint32>(interactiveUserAccessMask), 8, 16, QLatin1Char('0'));

	QString sddl = QStringLiteral("D:P(A;;GA;;;SY)(A;;GA;;;BA)");
	if (!isAdmin) {
		sddl += QStringLiteral("(A;;GA;;;%1)").arg(currentSid);
	} else {
		sddl += QStringLiteral("(A;;%1;;;%2)").arg(hexMask, currentSid);
	}

	if (!shellSid.isEmpty() && shellSid.compare(currentSid, Qt::CaseInsensitive) != 0) {
		sddl += QStringLiteral("(A;;%1;;;%2)").arg(hexMask, shellSid);
	}

	if (highIntegrityNoWriteUp && isAdmin) {
		sddl += QStringLiteral("S:(ML;;NW;;;HI)");
	} else {
		sddl += QStringLiteral("S:(ML;;NW;;;ME)");
	}

	PSECURITY_DESCRIPTOR pSD = NULL;
	const std::wstring sddlW = sddl.toStdWString();
	if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddlW.c_str(), SDDL_REVISION_1, &pSD, NULL)) {
		return NULL;
	}
	return pSD;
}

VOID FreeHookObjectSD(PSECURITY_DESCRIPTOR pSD) {
	if (pSD)
		LocalFree(pSD);
}

#if defined(NIGHTLIGHT_SUPPORT)
	NightLight::NightLight() : _client(new NightLightLibrary::NightLightWrapper())
	{
		_client->startWatching();
	}

	NightLight::~NightLight()
	{
		_client->stopWatching();
		delete _client;
	}

	bool NightLight::isSupported()
	{
		return NightLightLibrary::NightLightWrapper::isSupported(true);
	}

	void NightLight::apply(QList<QRgb>& colors, const double gamma)
	{
		PrismatikMath::applyColorTemperature(colors, _client->getSmoothenedColorTemperature(), gamma);
	}
#endif // NIGHTLIGHT_SUPPORT

	bool GammaRamp::isSupported()
	{
		HDC dc = NULL;
		WORD GammaArray[3][256];
		return loadGamma(&GammaArray, &dc);
	}

	bool GammaRamp::loadGamma(LPVOID gamma, HDC* dc)
	{
		POINT p;
		MONITORINFOEX monInfo;
		p.x = 0;
		p.y = 0;
		monInfo.cbSize = sizeof(MONITORINFOEX);

		HMONITOR mon = MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY);
		if (!GetMonitorInfo(mon, &monInfo)) {
			qWarning() << Q_FUNC_INFO << "Unable to get monitor info:" << GetLastError();
			return false;
		}

		*dc = CreateDC(TEXT("DISPLAY"), monInfo.szDevice, NULL, NULL);
		if (!GetDeviceGammaRamp(*dc, gamma)) {
			qWarning() << Q_FUNC_INFO << "Unable to create DC:" << GetLastError();
			return false;
		}

		return true;
	}

	void GammaRamp::apply(QList<QRgb>& colors, const double/*gamma*/)
	{
		HDC dc = NULL;

		// Reload the gamma ramp every 15 seconds only.
		// There seems to be a memory leak somewhere in the API and we don't
		// expect the ramp to change every frame.
		if (time(nullptr) - _gammaAge > 15) {
			if (!loadGamma(&_gammaArray, &dc))
				return;
			_gammaAge = time(nullptr);
		}

		for (QRgb& color : colors) {
			int red = qRed(color);
			int green = qGreen(color);
			int blue = qBlue(color);

			red = _gammaArray[0][red] >> 8;
			green = _gammaArray[1][green] >> 8;
			blue = _gammaArray[2][blue] >> 8;

			color = qRgb(red, green, blue);
		}

		DeleteObject(dc);
	}

} // namespace WinUtils

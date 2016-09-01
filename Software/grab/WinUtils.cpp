/*
 * WinUtils.cpp
 *
 *  Created on: 25.07.11
 *     Project: Lightpack
 *
 *  Copyright (c) 2011 Timur Sattarov, Mike Shatohin
 *
 *  Lightpack a USB content-driving ambient lighting system
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "WinUtils.hpp"

#include <psapi.h>
#include <tlhelp32.h>
#include <shlwapi.h>

#include "common/DebugOut.hpp"

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
    L"dwm.exe", L"ShellExperienceHost.exe", L"ApplicationFrameHost.exe", L"LockAppHost.exe", L"explorer.exe", L"SearchUI.exe"
    // Graphics Drivers
    L"igfxEM.exe", L"igfxTray.exe", L"nvxdsync.exe", L"nvvsvc.exe",
    // Browsers
    L"chrome.exe", L"firefox.exe", L"iexplore.exe",
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
    HANDLE hCurrentProcToken;

    if (!OpenProcessToken(hCurrentProc, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hCurrentProcToken)) {
//        syslog(LOG_ERR, "OpenProcessToken Error %u", GetLastError());
    } else {
        if (!SetPrivilege(hCurrentProcToken, SE_DEBUG_NAME, TRUE)) {
//            syslog(LOG_ERR, "SetPrivleges SE_DEBUG_NAME Error %u", GetLastError());
        } else {
            return TRUE;
        }
    }
    return FALSE;
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
    char debug_buf_module[255];
    WCHAR executableName[MAX_PATH];
    unsigned int i;

    DEBUG_MID_LEVEL << Q_FUNC_INFO << "scanning";

    //     Get the list of process identifiers.
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

            GetModuleFileNameExW(hProcess, 0, executableName, sizeof(executableName));

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
                        ::WideCharToMultiByte(CP_ACP, 0, szModName, -1, debug_buf_module, 255, NULL, NULL);
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


PVOID BuildRestrictedSD(PSECURITY_DESCRIPTOR pSD) {
    DWORD  dwAclLength;
    PSID   pAuthenticatedUsersSID = NULL;
    PACL   pDACL   = NULL;
    BOOL   bResult = FALSE;
    SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_NT_AUTHORITY;

    // initialize the security descriptor
    if (!InitializeSecurityDescriptor(pSD,
                                      SECURITY_DESCRIPTOR_REVISION)) {
//        syslog(LOG_ERR, "InitializeSecurityDescriptor() failed with error %d\n",
//               GetLastError());
        goto end;
    }

    // obtain a sid for the Authenticated Users Group
    if (!AllocateAndInitializeSid(&siaNT, 1,
                                  SECURITY_AUTHENTICATED_USER_RID, 0, 0, 0, 0, 0, 0, 0,
                                  &pAuthenticatedUsersSID)) {
//        syslog(LOG_ERR, "AllocateAndInitializeSid() failed with error %d\n",
//               GetLastError());
        goto end;
    }

    // NOTE:
    //
    // The Authenticated Users group includes all user accounts that
    // have been successfully authenticated by the system. If access
    // must be restricted to a specific user or group other than
    // Authenticated Users, the SID can be constructed using the
    // LookupAccountSid() API based on a user or group name.

    // calculate the DACL length
    dwAclLength = sizeof(ACL)
            // add space for Authenticated Users group ACE
            + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)
            + GetLengthSid(pAuthenticatedUsersSID);

    // allocate memory for the DACL
    pDACL = (PACL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                             dwAclLength);
    if (!pDACL) {
//        syslog(LOG_ERR, "HeapAlloc() failed with error %d\n", GetLastError());
        goto end;
    }

    // initialize the DACL
    if (!InitializeAcl(pDACL, dwAclLength, ACL_REVISION)) {
//        syslog(LOG_ERR, "InitializeAcl() failed with error %d\n",
//               GetLastError());
        goto end;
    }

    // add the Authenticated Users group ACE to the DACL with
    // GENERIC_READ, GENERIC_WRITE, and GENERIC_EXECUTE access

    if (!AddAccessAllowedAce(pDACL, ACL_REVISION,
                             MAXIMUM_ALLOWED ,
                             pAuthenticatedUsersSID)) {
//        syslog(LOG_ERR, "AddAccessAllowedAce() failed with error %d\n",
//               GetLastError());
        goto end;
    }

    // set the DACL in the security descriptor
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pDACL, FALSE)) {
//        syslog(LOG_ERR, "SetSecurityDescriptorDacl() failed with error %d\n",
//               GetLastError());
        goto end;
    }

    bResult = TRUE;

end:
    if (pAuthenticatedUsersSID) FreeSid(pAuthenticatedUsersSID);

    if (bResult == FALSE) {
        if (pDACL) HeapFree(GetProcessHeap(), 0, pDACL);
        pDACL = NULL;
    }

    return (PVOID) pDACL;
}

VOID FreeRestrictedSD(PVOID ptr) {

    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);

    return;
}

} // namespace WinUtils

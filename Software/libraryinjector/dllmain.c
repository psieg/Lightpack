/*
 * dllmain.c
 *
 *  Created on: 12.01.2014
 *     Project: Lightpack
 *
 *  Copyright (c) 2012 Timur Sattarov
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

#include <stdlib.h>
#include "LibraryInjector.h"

#define GUID_BUFFER_SIZE 40
static const WCHAR ObjectDescription[] = L"LibraryInjector COM component";
static const WCHAR ClassKeyName[] = L"Software\\Classes";
static const WCHAR CLSID_Str[] = L"CLSID";
static const WCHAR InprocServer32Name[] = L"InprocServer32";
static const WCHAR ThreadingModel[] = L"ThreadingModel";
static const WCHAR BothStr[] = L"Apartment";

static HMODULE GetCurrentModule() {
  // Here's a trick that will get you the handle of the module
  // you're running in without any a-priori knowledge:
  // http://www.dotnet247.com/247reference/msgs/13/65259.aspx

  MEMORY_BASIC_INFORMATION mbi;
  static int dummy;
  VirtualQuery( &dummy, &mbi, sizeof(mbi) );

  return (HMODULE)(mbi.AllocationBase);
}

STDAPI DllRegisterServer(void) {
    HKEY        rootKey;
    HKEY        hKey;
    HKEY        hKey2;
    HKEY        hkExtra;
    WCHAR       buffer[GUID_BUFFER_SIZE];
    DWORD       disposition;
    HRESULT     result;
    WCHAR       filename[MAX_PATH];

    DWORD fnLen = GetModuleFileNameW(GetCurrentModule(), filename, _countof(filename));
    if (fnLen == 0 || fnLen >= _countof(filename))
        return E_FAIL;

    result = S_FALSE;
    // Open "HKEY_LOCAL_MACHINE\Software\Classes"
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, &ClassKeyName[0], 0, KEY_WRITE, &rootKey))
    {
        // Open "HKEY_LOCAL_MACHINE\Software\Classes\CLSID"
        if (!RegOpenKeyEx(rootKey, &CLSID_Str[0], 0, KEY_ALL_ACCESS, &hKey))
        {
            // Create a subkey whose name is the ascii string that represents
            // our IExample2 object's GUID
            StringFromGUID2(&CLSID_ILibraryInjector, buffer, _countof(buffer));
            if (!RegCreateKeyEx(hKey, &buffer[0], 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hKey2, &disposition))
            {
                // Set its default value to some "friendly" string that helps
                // a user identify what this COM DLL is for. Setting this value
                // is optional. You don't need to do it
                RegSetValueEx(hKey2, 0, 0, REG_SZ, (const BYTE *)&ObjectDescription[0], sizeof(ObjectDescription));

                // Create an "InprocServer32" key whose default value is the path of this DLL
                if (!RegCreateKeyEx(hKey2, &InprocServer32Name[0], 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hkExtra, &disposition)) {
                    if (!RegSetValueEx(hkExtra, 0, 0, REG_SZ, (const BYTE *)&filename[0], (DWORD)wcslen(&filename[0]) * 2 + 2)) {
                        // Create a "ThreadingModel" value set to the string "both" (ie, we don't need to restrict an
                        // application to calling this DLL's functions only from a single thread. We don't use global
                        // data in our IExample functions, so we're thread-safe)
                        if (!RegSetValueEx(hkExtra, &ThreadingModel[0], 0, REG_SZ, (const BYTE *)&BothStr[0], sizeof(BothStr))) {
                            result = S_OK;
                        }
                    }

                    // Close all keys we opened/created.
                    RegCloseKey(hkExtra);
                }

                RegCloseKey(hKey2);
            }

            RegCloseKey(hKey);
        }

        RegCloseKey(rootKey);
    }
    return result;
}

STDAPI DllUnregisterServer(void) {
    HKEY        rootKey;
    HKEY        hKey;
    HKEY        hKey2;
    WCHAR       buffer[GUID_BUFFER_SIZE];

    StringFromGUID2(&CLSID_ILibraryInjector, buffer, _countof(buffer));

    // Open "HKEY_LOCAL_MACHINE\Software\Classes"
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, &ClassKeyName[0], 0, KEY_WRITE, &rootKey))
    {
        // Delete our CLSID key and everything under it
        if (!RegOpenKeyEx(rootKey, &CLSID_Str[0], 0, KEY_ALL_ACCESS, &hKey))
        {
            if (!RegOpenKeyEx(hKey, buffer, 0, KEY_ALL_ACCESS, &hKey2))
            {
                RegDeleteKey(hKey2, &InprocServer32Name[0]);

                RegCloseKey(hKey2);
                RegDeleteKey(hKey, &buffer[0]);
            }

            RegCloseKey(hKey);
        }

        RegCloseKey(rootKey);
    }
    return S_OK;
}

 /************************ DllGetClassObject() ***********************
 * This is called by the OLE functions CoGetClassObject() or
 * CoCreateInstance() in order to get our DLL's IClassFactory object
 * (and return it to someone who wants to use it to get ahold of one
 * of our LibraryInjector objects). Our IClassFactory's CreateInstance() can
 * be used to allocate/retrieve our LibraryInjector object.
 *
 * NOTE: After we return the pointer to our IClassFactory, the caller
 * will typically call its CreateInstance() function.
 */

LIBRARYINJECTORSHARED_EXPORT HRESULT PASCAL DllGetClassObject(REFCLSID objGuid, REFIID factoryGuid, void **factoryHandle) {
    register HRESULT        hr;

#ifdef DEBUG
    WCHAR guid[GUID_BUFFER_SIZE];
    StringFromGUID2(objGuid, guid, _countof(guid));
    reportLog(EVENTLOG_INFORMATION_TYPE, L"class guid %s",guid);
    StringFromGUID2(factoryGuid, guid, _countof(guid));
    reportLog(EVENTLOG_INFORMATION_TYPE, L"factory guid %s", guid);
#endif

    if (IsEqualCLSID(objGuid, &CLSID_ILibraryInjector))
    {
        IClassFactory * classFactory = allocClassFactory();
        if (classFactory)
        {
            classFactory->lpVtbl->AddRef(classFactory);
            // Fill in the caller's handle with a pointer to our IClassFactory object.
            // We'll let our IClassFactory's QueryInterface do that, because it also
            // checks the IClassFactory GUID and does other book-keeping
            hr = classFactory->lpVtbl->QueryInterface(classFactory, factoryGuid, factoryHandle);
            classFactory->lpVtbl->Release(classFactory);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
#ifdef DEBUG
        WCHAR supportedGuid[GUID_BUFFER_SIZE], requestedGuid[GUID_BUFFER_SIZE];
        StringFromGUID2(&CLSID_ILibraryInjector, supportedGuid, _countof(supportedGuid));
        StringFromGUID2(objGuid, requestedGuid, _countof(requestedGuid));
        reportLog(EVENTLOG_ERROR_TYPE, L"class guid is not matched. supported guid %s, requested guid %s", supportedGuid, requestedGuid);
#endif

        // We don't understand this GUID. It's obviously not for our DLL.
        // Let the caller know this by clearing his handle and returning
        // CLASS_E_CLASSNOTAVAILABLE
        *factoryHandle = 0;
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return (hr);
}

/************************ DllCanUnloadNow() ***********************
 * This is called by some OLE function in order to determine
 * whether it is safe to unload our DLL from memory.
 *
 * RETURNS: S_OK if safe to unload, or S_FALSE if not.
 */

LIBRARYINJECTORSHARED_EXPORT HRESULT PASCAL DllCanUnloadNow(void) {
    // If someone has retrieved pointers to any of our objects, and
    // not yet Release()'ed them, then we return S_FALSE to indicate
    // not to unload this DLL. Also, if someone has us locked, return
    // S_FALSE
    return (isLibraryInjectorActive() ? S_FALSE : S_OK);
}

/************************** DllMain() **************************
 * Called by the Windows OS when this DLL is loaded or unloaded.
 */

BOOL WINAPI DllMain(HINSTANCE instance, DWORD fdwReason, LPVOID lpvReserved) {
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            libraryInjectorInit();

            // We don't need to do any thread initialization
            DisableThreadLibraryCalls(instance);
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            libraryInjectorShutdown();
            break;
        }
    }

    return (1);
}

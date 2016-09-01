/*
 * WinDXUtils.cpp
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

#include "WinDXUtils.hpp"

#ifndef NO_QT
#include <QtGlobal>
#include <QString>
#endif
#include <DXGI.h>
#include <D3D10_1.h>
#include <D3D10.h>
#include <d3d9.h>

#include "common/DebugOut.hpp"
#include "common/msvcstub.h"

#define DXGI_PRESENT_FUNC_ORD 8
#define D3D9_SCPRESENT_FUNC_ORD 3
#define D3D9_PRESENT_FUNC_ORD 17
#define D3D9_RESET_FUNC_ORD 16

namespace WinUtils
{
typedef HRESULT (WINAPI* CREATEDXGIFACTORY)(REFIID, void**);
typedef HRESULT (WINAPI* D3D10CREATEDEVICEANDSWAPCHAIN)(IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D10Device**);
static CREATEDXGIFACTORY pCreateDXGIFactory = NULL;
static D3D10CREATEDEVICEANDSWAPCHAIN pD3D10CreateDeviceAndSwapChain = NULL;


HWND GetFirstWindowOfProcess()
{
    HWND h = ::GetTopWindow(0);
    DWORD procID = GetCurrentProcessId();

    while (h)
    {
        DWORD pid;
        ::GetWindowThreadProcessId(h, &pid);

        if (pid == procID)
        {
            return h;
        }
        h = ::GetNextWindow(h, GW_HWNDNEXT);
    }

    return (HWND)INVALID_HANDLE_VALUE;
}

BOOL LoadD3DandDXGI()
{
    HMODULE dxgi = LoadLibrary(L"dxgi.dll");
    HMODULE d3d10 = LoadLibrary(L"d3d10.dll");
    if (!dxgi || !d3d10)
        return false;

    pCreateDXGIFactory = (CREATEDXGIFACTORY) GetProcAddress(dxgi, "CreateDXGIFactory");
    if (!pCreateDXGIFactory) return false;
    pD3D10CreateDeviceAndSwapChain = (D3D10CREATEDEVICEANDSWAPCHAIN) GetProcAddress(d3d10, "D3D10CreateDeviceAndSwapChain");
    if (!pD3D10CreateDeviceAndSwapChain) return false;

    return true;
}

UINT GetDxgiPresentOffset(HWND hwnd) {
#ifndef NO_QT
    Q_ASSERT(hwnd != NULL);
#endif
    IDXGIFactory1 * factory = NULL;
    IDXGIAdapter * adapter;
    pCreateDXGIFactory = (CREATEDXGIFACTORY)GetProcAddress(GetModuleHandle(L"dxgi.dll"), "CreateDXGIFactory");
    HRESULT hresult = pCreateDXGIFactory(IID_IDXGIFactory, reinterpret_cast<void **>(&factory));
    if (hresult != S_OK) {
#ifndef NO_QT
        qCritical() << "Can't create DXGIFactory. " << hresult;
#endif
        return 0;
    }
    hresult = factory->EnumAdapters(0, &adapter);
    if (hresult != S_OK) {
#ifndef NO_QT
        qCritical() << "Can't enumerate adapters. " << hresult;
#endif
        return 0;
    }

    DXGI_MODE_DESC dxgiModeDesc;
    dxgiModeDesc.Width                      = 1;
    dxgiModeDesc.Height                     = 1;
    dxgiModeDesc.RefreshRate.Numerator      = 1;
    dxgiModeDesc.RefreshRate.Denominator    = 1;
    dxgiModeDesc.Format                     = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiModeDesc.ScanlineOrdering           = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    dxgiModeDesc.Scaling                    = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SAMPLE_DESC dxgiSampleDesc = {1, 0};

    DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
    memset(&dxgiSwapChainDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
    dxgiSwapChainDesc.BufferDesc    = dxgiModeDesc;
    dxgiSwapChainDesc.SampleDesc    = dxgiSampleDesc;
    dxgiSwapChainDesc.BufferUsage   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc.BufferCount   = 2;
    dxgiSwapChainDesc.OutputWindow  = hwnd;
    dxgiSwapChainDesc.Windowed      = true;
    dxgiSwapChainDesc.SwapEffect    = DXGI_SWAP_EFFECT_DISCARD;
    dxgiSwapChainDesc.Flags         = 0;

    IDXGISwapChain * pSc;
    ID3D10Device  * pDev;
    pD3D10CreateDeviceAndSwapChain = (D3D10CREATEDEVICEANDSWAPCHAIN)GetProcAddress(GetModuleHandle(L"d3d10.dll"), "D3D10CreateDeviceAndSwapChain");
    hresult = pD3D10CreateDeviceAndSwapChain(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &dxgiSwapChainDesc, &pSc, &pDev);
    if (S_OK != hresult) {
#ifndef NO_QT
        qCritical() << Q_FUNC_INFO << QString("Can't create D3D10Device and SwapChain. hresult = 0x%1").arg(QString::number(hresult, 16));
#endif
        return 0;
    }


    uintptr_t * pvtbl = reinterpret_cast<uintptr_t*>(pSc);
    uintptr_t presentFuncPtr = *pvtbl + sizeof(void *)*DXGI_PRESENT_FUNC_ORD;

#ifndef NO_QT
    char buf[100];
    sprintf(buf, "presentFuncPtr=%x", presentFuncPtr);
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

    intptr_t hDxgi = reinterpret_cast<intptr_t>(GetModuleHandle(L"dxgi.dll"));
    int presentFuncOffset = static_cast<UINT>(presentFuncPtr - hDxgi);

#ifndef NO_QT
    sprintf(buf, "presentFuncOffset=%x", presentFuncOffset);
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

    pSc->Release();
    pDev->Release();
    adapter->Release();
    factory->Release();

    return presentFuncOffset;
}

typedef IDirect3D9 * (WINAPI *Direct3DCreate9Func)(UINT SDKVersion);
UINT GetD3D9PresentOffset(HWND hWnd){
    IDirect3D9 *pD3D = NULL;
    HINSTANCE hD3d9 = LoadLibrary(L"d3d9.dll");
    Direct3DCreate9Func createDev = reinterpret_cast<Direct3DCreate9Func>(GetProcAddress(hD3d9, "Direct3DCreate9"));
    pD3D = createDev(D3D_SDK_VERSION);
    if ( !pD3D)
    {
#ifndef NO_QT
        qCritical() << "Test_DirectX9 Direct3DCreate9(%d) call FAILED" << D3D_SDK_VERSION ;
#endif
        return 0;
    }

    // step 3: Get IDirect3DDevice9
    D3DDISPLAYMODE d3ddm;
    HRESULT hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm );
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 GetAdapterDisplayMode failed. 0x%x").arg( hRes);
#endif
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    IDirect3DDevice9 *pD3DDevice;
    hRes = pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,    // the device we suppose any app would be using.
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3dpp, &pD3DDevice);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 CreateDevice failed. 0x%x").arg(hRes);
#endif
        return 0;
    }

    IDirect3DSwapChain9 *pD3DSwapChain;
    hRes = pD3DDevice->GetSwapChain( 0, &pD3DSwapChain);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 GetSwapChain failed. 0x%x").arg(hRes);
#endif
        return 0;
    }
    else
    {
        uintptr_t * pvtbl = *((uintptr_t**)(pD3DDevice));
        uintptr_t presentFuncPtr = pvtbl[D3D9_PRESENT_FUNC_ORD];
#ifndef NO_QT
        char buf[100];
        sprintf(buf, "presentFuncPtr=%x", presentFuncPtr);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

        UINT presentFuncOffset = (presentFuncPtr - reinterpret_cast<UINT>(hD3d9));

#ifndef NO_QT
        sprintf(buf, "presentFuncOffset=%x", presentFuncOffset);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

        pD3DSwapChain->Release();
        pD3DDevice->Release();
        pD3D->Release();

        return presentFuncOffset;
    }
}

UINT GetD3D9SCPresentOffset(HWND hWnd){
    IDirect3D9 *pD3D = NULL;
    HINSTANCE hD3d9 = LoadLibrary(L"d3d9.dll");
    Direct3DCreate9Func createDev = reinterpret_cast<Direct3DCreate9Func>(GetProcAddress(hD3d9, "Direct3DCreate9"));
    pD3D = createDev(D3D_SDK_VERSION);
    if ( !pD3D)
    {
#ifndef NO_QT
        qCritical() << "Test_DirectX9 Direct3DCreate9(%d) call FAILED" << D3D_SDK_VERSION ;
#endif
        return 0;
    }

    // step 3: Get IDirect3DDevice9
    D3DDISPLAYMODE d3ddm;
    HRESULT hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm );
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 GetAdapterDisplayMode failed. 0x%x").arg( hRes);
#endif
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    IDirect3DDevice9 *pD3DDevice;
    hRes = pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,    // the device we suppose any app would be using.
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3dpp, &pD3DDevice);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 CreateDevice failed. 0x%x").arg(hRes);
#endif
        return 0;
    }

    IDirect3DSwapChain9 *pD3DSwapChain;
    hRes = pD3DDevice->GetSwapChain( 0, &pD3DSwapChain);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 GetSwapChain failed. 0x%x").arg(hRes);
#endif
        return 0;
    }
    else
    {
        uintptr_t * pvtbl = *((uintptr_t**)(pD3DSwapChain));
        uintptr_t presentFuncPtr = pvtbl[D3D9_SCPRESENT_FUNC_ORD];
#ifndef NO_QT
        char buf[100];
        sprintf(buf, "presentFuncPtr=%x", presentFuncPtr);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

        void *pD3d9 = reinterpret_cast<void *>(hD3d9);
        UINT presentFuncOffset = (presentFuncPtr - reinterpret_cast<UINT>(pD3d9));

#ifndef NO_QT
        sprintf(buf, "presentFuncOffset=%x", presentFuncOffset);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

        pD3DSwapChain->Release();
        pD3DDevice->Release();
        pD3D->Release();

        return presentFuncOffset;
    }
}


UINT GetD3D9ResetOffset(HWND hWnd){
    IDirect3D9 *pD3D = NULL;
    HINSTANCE hD3d9 = LoadLibrary(L"d3d9.dll");
    Direct3DCreate9Func createDev = reinterpret_cast<Direct3DCreate9Func>(GetProcAddress(hD3d9, "Direct3DCreate9"));
    pD3D = createDev(D3D_SDK_VERSION);
    if (!pD3D)
    {
#ifndef NO_QT
        qCritical() << "Test_DirectX9 Direct3DCreate9(%d) call FAILED" << D3D_SDK_VERSION;
#endif
        return 0;
    }

    // step 3: Get IDirect3DDevice9
    D3DDISPLAYMODE d3ddm;
    HRESULT hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 GetAdapterDisplayMode failed. 0x%x").arg(hRes);
#endif
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    IDirect3DDevice9 *pD3DDevice;
    hRes = pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,    // the device we suppose any app would be using.
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3dpp, &pD3DDevice);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 CreateDevice failed. 0x%x").arg(hRes);
#endif
        return 0;
    }

    IDirect3DSwapChain9 *pD3DSwapChain;
    hRes = pD3DDevice->GetSwapChain(0, &pD3DSwapChain);
    if (FAILED(hRes))
    {
#ifndef NO_QT
        qCritical() << QString("Test_DirectX9 GetSwapChain failed. 0x%x").arg(hRes);
#endif
        return 0;
    }
    else
    {
        uintptr_t * pvtbl = *((uintptr_t**)(pD3DDevice));
        uintptr_t resetFuncPtr = pvtbl[D3D9_RESET_FUNC_ORD];
#ifndef NO_QT
        char buf[100];
        sprintf(buf, "resetFuncPtr=%x", resetFuncPtr);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

        UINT resetFuncOffset = (resetFuncPtr - reinterpret_cast<UINT>(hD3d9));

#ifndef NO_QT
        sprintf(buf, "resetFuncOffset=%x", resetFuncOffset);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;
#endif

        pD3DSwapChain->Release();
        pD3DDevice->Release();
        pD3D->Release();

        return resetFuncOffset;
    }
}
} // namespace WinUtils

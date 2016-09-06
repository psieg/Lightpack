/*
 * LedDeviceAlienFx.cpp
 *
 *  Created on: 17.04.2011
 *      Author: Timur Sattarov && Mike Shatohin
 *     Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#include <qglobal.h>

#ifdef Q_OS_WIN

#if !defined NOMINMAX
#define NOMINMAX
#endif

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "LedDeviceAlienFx.hpp"
#include "Settings.hpp"
#include <QtDebug>
#include "debug.h"
#include "../alienfx/LFX2.h"
#include <windows.h>

using namespace SettingsScope;

LFX2INITIALIZE lfxInitFunction;
LFX2RELEASE lfxReleaseFunction;
LFX2RESET lfxResetFunction;
LFX2UPDATE lfxUpdateFunction;
LFX2GETNUMDEVICES lfxGetNumDevicesFunction;
LFX2GETDEVDESC lfxGetDeviceDescriptionFunction;
LFX2GETNUMLIGHTS lfxGetNumLightsFunction;
LFX2SETLIGHTCOL lfxSetLightColorFunction;
LFX2GETLIGHTCOL lfxGetLightColorFunction;
LFX2GETLIGHTDESC lfxGetLightDescriptionFunction;
LFX2LIGHT lfxLightFunction;

LedDeviceAlienFx::LedDeviceAlienFx(QObject *parent) :
        AbstractLedDevice(parent)
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;

    m_isInitialized = false;

    m_hLfxLibrary = LoadLibrary(L"LightFX.dll");
    if (m_hLfxLibrary)
    {
        lfxInitFunction = (LFX2INITIALIZE)GetProcAddress(m_hLfxLibrary, LFX_DLL_INITIALIZE);
        lfxReleaseFunction = (LFX2RELEASE)GetProcAddress(m_hLfxLibrary, LFX_DLL_RELEASE);
        lfxResetFunction = (LFX2RESET)GetProcAddress(m_hLfxLibrary, LFX_DLL_RESET);
        lfxUpdateFunction = (LFX2UPDATE)GetProcAddress(m_hLfxLibrary, LFX_DLL_UPDATE);
        lfxGetNumDevicesFunction = (LFX2GETNUMDEVICES)GetProcAddress(m_hLfxLibrary, LFX_DLL_GETNUMDEVICES);
        lfxGetDeviceDescriptionFunction = (LFX2GETDEVDESC)GetProcAddress(m_hLfxLibrary, LFX_DLL_GETDEVDESC);
        lfxGetNumLightsFunction = (LFX2GETNUMLIGHTS)GetProcAddress(m_hLfxLibrary, LFX_DLL_GETNUMLIGHTS);
        lfxSetLightColorFunction = (LFX2SETLIGHTCOL)GetProcAddress(m_hLfxLibrary, LFX_DLL_SETLIGHTCOL);
        lfxGetLightColorFunction = (LFX2GETLIGHTCOL)GetProcAddress(m_hLfxLibrary, LFX_DLL_GETLIGHTCOL);
        lfxGetLightDescriptionFunction = (LFX2GETLIGHTDESC)GetProcAddress(m_hLfxLibrary, LFX_DLL_GETLIGHTDESC);
        lfxLightFunction = (LFX2LIGHT)GetProcAddress(m_hLfxLibrary, LFX_DLL_LIGHT);

        LFX_RESULT result = lfxInitFunction();
        if (result == LFX_SUCCESS)
        {
            m_isInitialized = true;
            result = lfxResetFunction();
        } else {
            emit ioDeviceSuccess(false);
        }

    } else {
        qWarning() << "couldn't load LightFX.dll";
    }

    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "initialized";
}

LedDeviceAlienFx::~LedDeviceAlienFx()
{
    if (m_isInitialized)
        lfxReleaseFunction();
    if (m_hLfxLibrary)
        FreeLibrary(m_hLfxLibrary);

    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "destroy LedDeviceAlienFx : ILedDevice complete";
}

void LedDeviceAlienFx::setColorsUnsmoothed(const QList<QRgb> & colors)
{
    DEBUG_MID_LEVEL << Q_FUNC_INFO;
    if (m_isInitialized)
    {
        unsigned int numDevs = 0;
        LFX_RESULT result = lfxGetNumDevicesFunction(&numDevs);
        Q_UNUSED(result);

        for(unsigned int devIndex = 0; devIndex < numDevs; devIndex++)
        {
            unsigned int numLights = 0;
            result = lfxGetNumLightsFunction(devIndex, &numLights);

            LFX_COLOR lfxColor;

            lfxColor.red   = qRed   ( colors[0] );
            lfxColor.green = qGreen ( colors[0] );
            lfxColor.blue  = qBlue  ( colors[0] );
            lfxColor.brightness = 255;

            for(unsigned int lightIndex = 0; lightIndex < numLights; lightIndex++)
                lfxSetLightColorFunction(devIndex, lightIndex, &lfxColor);
            lfxUpdateFunction();
        }
        emit ioDeviceSuccess(true);
    }
    DEBUG_MID_LEVEL << Q_FUNC_INFO;

    // Request new colors
    emit commandCompleted(true);
}

void LedDeviceAlienFx::switchOffLeds()
{
    // TODO: fill it with current leds count
    QList<QRgb> blackColor;
    blackColor << 0;
    setColors(blackColor);
}

void LedDeviceAlienFx::setColorSequence(QString /*value*/)
{
    emit commandCompleted(true);
}


void LedDeviceAlienFx::requestFirmwareVersion()
{
    emit firmwareVersion("unknown (alienfx)");
    emit commandCompleted(true);
}

void LedDeviceAlienFx::open()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;
    emit openDeviceSuccess(true);
}

#endif /* Q_OS_WIN */

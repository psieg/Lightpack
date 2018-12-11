/*
 * SettingsDefaults.hpp
 *
 *	Created on: 01.10.2011
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#pragma once

#include <QSize>
#include <QString>
#include "debug.h"
#include "../common/defs.h"
#include "enums.hpp"

#ifdef ALIEN_FX_SUPPORTED
#	define SUPPORTED_DEVICES			"Lightpack,AlienFx,Adalight,Ardulight,Virtual"
#else
#	define SUPPORTED_DEVICES			"Lightpack,Adalight,Ardulight,Virtual"
#endif

#ifdef DDUPL_GRAB_SUPPORT
#	define GRABMODE_DEFAULT			::Grab::GrabberTypeDDupl
#	define GRABMODE_DEFAULT_STR		"DDupl"
#elif defined(WINAPI_GRAB_SUPPORT)
#	define GRABMODE_DEFAULT			::Grab::GrabberTypeWinAPI
#	define GRABMODE_DEFAULT_STR		"WinAPI"
#elif defined(X11_GRAB_SUPPORT)
#	define GRABMODE_DEFAULT			::Grab::GrabberTypeX11
#	define GRABMODE_DEFAULT_STR		"X11"
#elif defined(MAC_OS_CG_GRAB_SUPPORT)
#	define GRABMODE_DEFAULT			::Grab::GrabberTypeMacCoreGraphics
#	define GRABMODE_DEFAULT_STR		"MacCoreGraphics"
#else
#	define GRABMODE_DEFAULT			::Grab::GrabberTypeQt
#	define GRABMODE_DEFAULT_STR		"Qt"
#endif

#ifdef Q_OS_UNIX
#	define SERIAL_PORT_DEFAULT	"/dev/ttyUSB0" /* For UART over USB on FT232RL */
#elif defined(Q_OS_WIN)
#	define SERIAL_PORT_DEFAULT	"COM1"
#endif

namespace SettingsScope
{

// LightpackMain.conf
namespace Main
{
// [General]
static const QString ProfileNameDefault = "Lightpack";
static const QString LanguageDefault = "<System>";
static const Debug::DebugLevels DebugLevelDefault = Debug::LowLevel;
static const bool IsExpertModeEnabledDefault = false;
static const bool IsKeepLightsOnAfterExit = false;
static const bool IsKeepLightsOnAfterLock = true;
static const bool IsKeepLightsOnAfterSuspend = false;
static const bool IsPingDeviceEverySecond = true;
static const bool IsUpdateFirmwareMessageShown = false;
static const QString ConnectedDeviceDefault = "Lightpack";
static const QString SupportedDevices = SUPPORTED_DEVICES; /* comma separated values! */
static const bool CheckForUpdates = true;
static const bool InstallUpdates = true;

// [HotKeys]
namespace HotKeys
{
static const QString HotkeyDefault = "Undefined";
}

// [API]
namespace Api
{
static const bool IsEnabledDefault = true;
static const bool ListenOnlyOnLoInterfaceDefault = true;
static const int PortDefault = 3636;
static const QString AuthKey = "";
// See ApiKey generation in Settings initialization
}
namespace Adalight
{
static const int NumberOfLedsDefault = 25;
static const QString ColorSequence = "RGB";
static const QString PortDefault = SERIAL_PORT_DEFAULT;
static const QString BaudRateDefault = "115200";
}
namespace Ardulight
{
static const int NumberOfLedsDefault = 10;
static const QString ColorSequence = "RGB";
static const QString PortDefault = SERIAL_PORT_DEFAULT;
static const QString BaudRateDefault = "115200";
}
namespace AlienFx
{
static const int NumberOfLedsDefault = 1;
}
namespace Lightpack
{
static const int NumberOfLedsDefault = 10;
}
namespace Virtual
{
static const int NumberOfLedsDefault = 10;
}
}

// ProfileName.ini
namespace Profile
{
// [General]
static const bool IsBacklightEnabledDefault = true;
static const QString LightpackModeDefault = "Ambilight";

// [Grab]
namespace Grab
{
static const ::Grab::GrabberType GrabberDefault = GRABMODE_DEFAULT;
static const QString GrabberDefaultString = GRABMODE_DEFAULT_STR;
static const bool IsAvgColorsEnabledDefault = false;
static const bool IsSendDataOnlyIfColorsChangesDefault = true;
static const bool IsMinimumLuminosityEnabledDefault = true;
static const bool IsDx1011GrabberEnabledDefault = false;
static const bool IsDx9GrabbingEnabledDefault = false;
static const int SlowdownMin = 1;
static const int SlowdownDefault = 50;
static const int SlowdownMax = 1000;
static const int LuminosityThresholdMin = 0;
static const int LuminosityThresholdDefault = 3;
static const int LuminosityThresholdMax = 100;
static const int OverBrightenMin = 0;
static const int OverBrightenDefault = 0;
static const int OverBrightenMax = 100;
static const bool IsApplyGammaRampEnabledDefault = true;
static const bool IsApplyColorTemperatureEnabledDefault = false;
static const int ColorTemperatureMin = 1000;
static const int ColorTemperatureDefault = 6500;
static const int ColorTemperatureMax = 10000;
static const double GammaMin = 0.05;
static const double GammaDefault = 1.2;
static const double GammaMax = 10.0;
}
// [MoodLamp]
namespace MoodLamp
{
static const int SpeedMin = 1;
static const int SpeedDefault = 50;
static const int SpeedMax = 100;
static const QString ColorDefault = "#00FF00";
static const bool IsLiquidModeDefault = true;
}
// [SoundVisualizer]
namespace SoundVisualizer
{
static const int DeviceDefault = -1;
static const QString MinColorDefault = "#301000";
static const QString MaxColorDefault = "#0000FF";
static const bool IsLiquidModeDefault = true;
static const int LiquidSpeedMin = 1;
static const int LiquidSpeedDefault = 10;
static const int LiquidSpeedMax = 100;
}
// [Device]
namespace Device
{
static const int RefreshDelayMin = 64;
static const int RefreshDelayDefault = 100;
static const int RefreshDelayMax = 1023;

static const bool IsUsbPowerLedDisabled = false;

static const int BrightnessMin = 0;
static const int BrightnessDefault = 100;
static const int BrightnessMax = 100;

static const int SmoothMin = 0;
static const int SmoothDefault = 100;
static const int SmoothMax = 255;

static const int ColorDepthMin = 32;
static const int ColorDepthDefault = 128;
static const int ColorDepthMax = 255;

static const double GammaMin = 0.01;
static const double GammaDefault = 2.0;
static const double GammaMax = 10.0;
}
// [LED_i]
namespace Led
{
static const bool IsEnabledDefault = true;
static const double CoefMin = 0.0;
static const double CoefDefault = 1.0;
static const double CoefMax = 1.0;
static const QSize SizeDefault = QSize(150, 150);
}
} /*Profile*/

} /*SettingsScope*/

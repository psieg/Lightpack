/*
 * enums.hpp
 *
 *	Created on: 11.09.2011
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Mike Shatohin (MikeShatohin@gmail.com)
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

#include "../common/defs.h"

namespace Backlight
{
enum Status {
	StatusOff = 0,
	StatusOn = 1,
	StatusDeviceError = -1,
	// Just for API
	StatusUnknown = -93
};
}

namespace DeviceLocked
{
enum DeviceLockStatus {
	Unlocked,
	Api,
	Plugin,
	ApiPersist
};
}

namespace Lightpack
{
enum Mode {
	AmbilightMode,
	MoodLampMode,
	SoundVisualizeMode,
	UnknownMode,
	Default = AmbilightMode
};
}

namespace Grab
{

enum GrabberType {
	GrabberTypeQt,
	GrabberTypeQtEachWidget,
	GrabberTypeX11,
	GrabberTypeWinAPI,
	GrabberTypeWinAPIEachWidget,
	GrabberTypeD3D9,
	GrabberTypeMacCoreGraphics,
	GrabberTypeMacAVFoundation,
	GrabberTypeDDupl,
	GrabberTypeNvFBC,

	GrabbersCount,

	GrabberTypeDX10_11 //since d3d10 grabber works simultaneously with regular grabber we don't count it as others
};
}

namespace SupportedDevices
{
enum DeviceType {
	DeviceTypeLightpack,
	DeviceTypeAlienFx,
	DeviceTypeAdalight,
	DeviceTypeVirtual,
	DeviceTypeArdulight,

	DeviceTypesCount,
	DefaultDeviceType = DeviceTypeLightpack
};
}

namespace MaximumNumberOfLeds
{
enum Devices
{
	AbsoluteMaximum = 511,

	Adalight	= 511,
	Ardulight	= 255,
	AlienFx		= 1,
	Virtual		= 511,

	Lightpack4	= 8,
	Lightpack5	= 10,
	Lightpack6	= 10,

	Default		= 10
};
}

// Configure SettingsWindow Device tab for suitable device options
namespace DeviceTab
{

/*!
\deprecated unused with new GUI
*/
enum Options
{
	NumberOfLeds	= (1 << 0),
	Brightness		= (1 << 1),
	Gamma			= (1 << 2),

	RefreshDelay	= (1 << 3),
	ColorDepth		= (1 << 4),
	SmoothSlowdown	= (1 << 5),
	SerialPort		= (1 << 6), /* serial port name and baud rate */
	VirtualLeds		= (1 << 7),
	ColorSequence	= (1 << 8),

	Default			= NumberOfLeds | Brightness | Gamma,

	Adalight		= Default | SerialPort | ColorSequence,
	Ardulight		= Default | SerialPort | ColorSequence,
	AlienFx			= Default,
	Lightpack		= Default | SmoothSlowdown | RefreshDelay | ColorDepth,
	Virtual			= Default | VirtualLeds
};
}

// Using in commands queue of LedDeviceManager
namespace LedDeviceCommands
{
enum Cmd {
	OffLeds,
	SetColors,
	SetUsbPowerLedDisabled,
	SetRefreshDelay,
	SetColorDepth,
	SetSmoothSlowdown,
	SetGamma,
	SetBrightness,
	SetLuminosityThreshold,
	SetMinimumLuminosityEnabled,
	SetColorSequence,
	RequestFirmwareVersion,
	UpdateWBAdjustments,
	UpdateDeviceSettings
};
}

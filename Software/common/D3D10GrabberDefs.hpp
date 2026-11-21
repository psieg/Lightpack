/*
 * D3D10GrabberDefs.hpp
 *
 *	Created on: 01.06.2012
 *		Author: Timur Sattarov
 *		Project: Lightpack
 *
 *	Copyright (c) 2012 Timur Sattarov
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
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

#include "BufferFormat.h"

#define HOOKSGRABBER_CONFIG_MEM_NAME L"Lightpack.D3D10Grabber.Config"
#define HOOKSGRABBER_SHARED_MEM_NAME L"Lightpack.D3D10Grabber.SharedMem"
#define MB (1024*1024)
#define HOOKSGRABBER_SHARED_MEM_SIZE (50*MB)
#define HOOKSGRABBER_MUTEX_MEM_NAME L"Lightpack.D3D10Grabber.Mutex"
#define HOOKSGRABBER_FRAMEGRABBED_EVENT_NAME L"Lightpack.D3D10Grabber.Event.FrameGrabbed"
#define HOOKSGRABBER_PIPE_NAME L"\\\\.\\pipe\\Lightpack.D3D10Grabber"

#define HOOKSGRABBER_BLANK_FRAME_ID 0xffffffff
#define HOOKSGRABBER_MAX_FRAME_DIM 16384U

#ifdef HOOKS_SYSWOW64
#define HOOKSGRABBER_SYSWOW64_DESC
#endif

/* Read-only configuration written exclusively by Prismatik / offsetfinder */
struct HOOKSGRABBER_CONFIG_DESC {
	UINT d3d9PresentFuncOffset;
	UINT d3d9SCPresentFuncOffset;
	UINT d3d9ResetFuncOffset;
	UINT dxgiPresentFuncOffset;
#if defined(_WIN64) || defined(HOOKSGRABBER_SYSWOW64_DESC)
	UINT d3d9PresentFuncOffset32;
	UINT d3d9SCPresentFuncOffset32;
	UINT d3d9ResetFuncOffset32;
	UINT dxgiPresentFuncOffset32;
	UINT loadLibraryWAddress32;
#endif
	UINT logLevel;
	UINT grabDelay;
	UINT grabbingStarted;
};

#define HOOKSGRABBER_CONFIG_MEM_SIZE (sizeof(struct HOOKSGRABBER_CONFIG_DESC))

/* Frame metadata written by hooked DX processes into the frame shared memory */
struct HOOKSGRABBER_SHARED_MEM_DESC {
	UINT frameId;
	UINT width;
	UINT height;
	UINT rowPitch;
#ifdef __cplusplus
	BufferFormat format;
#else
	UINT format;
#endif
};


#define HOOKSGRABBER_SHARED_MEM_DATA_SIZE (HOOKSGRABBER_SHARED_MEM_SIZE - sizeof(struct HOOKSGRABBER_SHARED_MEM_DESC))

#ifdef __cplusplus
inline bool hooksGrabberValidateFrameSize(UINT width, UINT height, UINT rowPitch) {
	if (width == 0 || height == 0 || width > HOOKSGRABBER_MAX_FRAME_DIM || height > HOOKSGRABBER_MAX_FRAME_DIM)
		return false;
	const unsigned long long minPitch = static_cast<unsigned long long>(width) * 4ULL;
	if (static_cast<unsigned long long>(rowPitch) < minPitch)
		return false;
	const unsigned long long frameBytes = minPitch * static_cast<unsigned long long>(height);
	return frameBytes <= static_cast<unsigned long long>(HOOKSGRABBER_SHARED_MEM_DATA_SIZE);
}
#endif

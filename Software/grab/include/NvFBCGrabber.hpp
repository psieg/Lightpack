/*
 * NvFBCGrabber.hpp
 *
 *	Created on: 01.04.19
 *		Project: Lightpack
 *
 *	Copyright (c) 2019 Maximilian Roehrl
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

#ifdef NVFBC_GRAB_SUPPORT

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <d3d9.h>

#include "GrabberBase.hpp"
#include "NvFBC/NvFBC.h"
#include "NvFBC/NvFBCToSys.h"

class NvFBCGrabber : public GrabberBase
{
	Q_OBJECT
public:
	NvFBCGrabber(QObject* parent, GrabberContext* context);
	virtual ~NvFBCGrabber();

	DECLARE_GRABBER_NAME("NvFBCGrabber")

protected slots:
	GrabResult grabScreens() override;
	bool reallocate(const QList<ScreenInfo>& grabScreens) override;
	QList<ScreenInfo>* screensWithWidgets(QList<ScreenInfo>* result, const QList<GrabWidget*>& grabWidgets) override;

protected:
	bool init();
	void freeScreens();

private:
	BOOL m_admin_message_shown;
	HMODULE m_nvfbcDll;
	NvFBC_GetStatusExFunctionType pfn_get_status;
	NvFBC_SetGlobalFlagsType pfn_set_global_flags;
	NvFBC_CreateFunctionExType pfn_create;
	NvFBC_EnableFunctionType pfn_enable;
};

#endif // NVFBC_GRAB_SUPPORT
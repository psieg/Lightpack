/*
* DDGrabber.hpp
*
*	Created on: 21.12.2018
*		Project: Lightpack
*
*	Copyright (c) 2014 Patrick Siegler
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

#include <GrabberBase.hpp>
#ifdef DDUPL_GRAB_SUPPORT

#if !defined NOMINMAX
#define NOMINMAX
#endif

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// Forward declaration of IDXGIAdapter1 to avoid including dxgi here
#include <comdef.h>
MIDL_INTERFACE("29038f61-3839-4626-91fd-086879011a05") IDXGIAdapter1;
_COM_SMARTPTR_TYPEDEF(IDXGIAdapter1, __uuidof(IDXGIAdapter1));

enum DDuplGrabberState
{
	Uninitialized,
	Ready,
	Allocated,
	LostAccess,
	AccessDeniedDesktop,
	AccessDeniedDuplication,
	Unavailable
};
enum DDuplGrabberThreadCommand
{
	Exit,
	Reallocate
};


class DDuplGrabber : public GrabberBase
{
	Q_OBJECT
public:
	DDuplGrabber(QObject * parent, GrabberContext *context);
	virtual ~DDuplGrabber();

	DECLARE_GRABBER_NAME("DDuplGrabber")

public slots:
	void onSessionChange(int change);

protected slots:
	virtual GrabResult grabScreens();
	virtual bool reallocate(const QList< ScreenInfo > &grabScreens);
	bool _reallocate(const QList< ScreenInfo > &grabScreens, bool noRecursion = false);

	virtual QList< ScreenInfo > * screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets);
	QList< ScreenInfo > * __screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets, bool noRecursion = false);

	virtual bool isReallocationNeeded(const QList< ScreenInfo > &grabScreens) const;

protected:
	bool init();
	bool recreateAdapters();
	void freeScreens();
	GrabResult returnBlackBuffer();
	bool runThreadCommand(DWORD timeout);

private:
	QList<IDXGIAdapter1Ptr> m_adapters;
	DDuplGrabberState m_state;
	DWORD m_accessDeniedLastCheck;

	FARPROC m_createDXGIFactory1Func;
	FARPROC m_D3D11CreateDeviceFunc;
	HMODULE m_dxgiDll;
	HMODULE m_d3d11Dll;

	friend DWORD WINAPI DDuplGrabberThreadProc(LPVOID arg);
	HANDLE m_thread;
	HANDLE m_threadEvent, m_threadReturnEvent;
	DDuplGrabberThreadCommand m_threadCommand;
	QList<ScreenInfo> m_threadReallocateArg;
	bool m_threadReallocateResult;
	bool m_isSessionLocked;
};


#endif
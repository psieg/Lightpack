/*
 * EndSessionDetector.hpp
 *
 *	Created on: 01.03.2014
 *		Author: EternalWind
 *		Project: Lightpack
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#ifndef SESSION_CHANGE_DETECTOR
#define SESSION_CHANGE_DETECTOR
#include "QObject"
#include "QAbstractNativeEventFilter"

#ifdef Q_OS_WIN
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

class SessionChangeDetector :
	public QObject,
	public QAbstractNativeEventFilter
{
	Q_OBJECT

public:

	enum SessionChange : int {
		Ending,
		Locking,
		Unlocking,
		Sleeping,
		Resuming,
		DisplayOn,
		DisplayOff,
		DisplayDimmed
	};

	SessionChangeDetector();

	bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) Q_DECL_OVERRIDE;

	~SessionChangeDetector();

signals:
	void sessionChangeDetected(int change);

private:
	void Destroy();

	bool m_isDestroyed;

#ifdef Q_OS_WIN
	HPOWERNOTIFY m_powerSettingNotificationHandle;
#endif
};
#endif
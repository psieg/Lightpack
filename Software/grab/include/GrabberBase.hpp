/*
 * GrabberBase.hpp
 *
 *	Created on: 18.07.2012
 *		Project: Lightpack
 *
 *	Copyright (c) 2012 Timur Sattarov
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

#include <QTimer>
#include "src/GrabWidget.hpp"
#include "calculations.hpp"


class GrabberContext;

enum GrabResult {
	GrabResultOk,
	GrabResultFrameNotReady,
	GrabResultError
};

struct ScreenInfo {
	ScreenInfo() = default;

	bool operator==(const ScreenInfo &other) const {
		return other.rect == this->rect;
	}

	QRect rect;
	void * handle = nullptr;
};

struct GrabbedScreen {
	GrabbedScreen() = default;

	const unsigned char * imgData = nullptr;

	size_t imgDataSize = 0;
	BufferFormat imgFormat = BufferFormatUnknown;
	ScreenInfo screenInfo;
	void * associatedData = nullptr;

	double scale = 1.0; // if grabber has ability to scale frames
	size_t bytesPerRow = 0; // some grabbing methods won't return values equal to (width * bytesPerPixel) because of alignment / padding
};

#define DECLARE_GRABBER_NAME(grabber_name) \
	const char * name() const override { \
		static const char * static_grabber_name = (grabber_name); \
		return static_grabber_name; \
	}

/*!
	Base class which represents each particular grabber. If you want to add a new grabber just add implementation of \code GrabberBase \endcode
	and modify \a GrabManager
*/
class GrabberBase : public QObject
{
	Q_OBJECT
public:
	GrabberBase(QObject * parent, GrabberContext * grabberContext);
	virtual ~GrabberBase() = default;

	virtual const char * name() const = 0;

public slots:
	virtual void startGrabbing();
	virtual void stopGrabbing();
	virtual bool isGrabbingStarted() const;
	virtual void setGrabInterval(int msec);

	virtual void grab();

protected slots:
	/*!
		Grabs screens and saves them to \a GrabberBase#_screensWithWidgets field. Called by
		\a GrabberBase#grab() slot. Needs to be implemented in derived classes.
		\return GrabResult
	*/
	virtual GrabResult grabScreens() = 0;

	/*!
		* Frees unnecessary resources and allocates needed ones based on \a ScreenInfo
		* \param grabScreens
		* \return
		*/
	virtual bool reallocate(const QList< ScreenInfo > &grabScreens) = 0;

	/*!
		* Get all screens grab widgets lies on.
		* \param result
		* \param grabWidgets
		* \return
		*/
	virtual QList< ScreenInfo > * screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets) = 0;

	virtual bool isReallocationNeeded(const QList< ScreenInfo > &grabScreens) const;

protected:
	const GrabbedScreen * screenOfRect(const QRect &rect) const;

signals:
	void frameGrabAttempted(GrabResult grabResult);

	/*!
		Signals \a GrabManager that the grabber wants to be started or stopped
	*/
	void grabberStateChangeRequested(bool isStartRequested);

protected:
	GrabberContext *_context;
	GrabResult _lastGrabResult;
	int grabScreensCount;
	QList<GrabbedScreen> _screensWithWidgets;
	QScopedPointer<QTimer> m_timer;
};

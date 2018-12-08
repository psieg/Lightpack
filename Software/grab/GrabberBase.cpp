/*
 * GrabberBase.cpp
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

#include "GrabberContext.hpp"
#include "GrabWidget.hpp"
#include "GrabberBase.hpp"
#include "src/debug.h"

namespace
{

int validCoord(int a)
{
	const unsigned int neg = (1 << 15);
	if (a & neg)
		a = ((~a + 1) & 0x0000ffff) * -1;
	return a;
}

inline QRect & getValidRect(QRect & rect)
{
	int x1,x2,y1,y2;
	rect.getCoords(&x1, &y1, &x2, &y2);
	x1 = validCoord(x1);
	y1 = validCoord(y1);
	rect.setCoords(x1, y1, x1 + rect.width() - 1, y1 + rect.height() - 1);
	return rect;
}

} // anonymous namespace


GrabberBase::GrabberBase(QObject *parent, GrabberContext *grabberContext) : QObject(parent)
{
	_context = grabberContext;
	if (m_timer && m_timer->isActive())
		m_timer->stop();
	m_timer.reset(new QTimer(this));
	m_timer->setTimerType(Qt::PreciseTimer);
	connect(m_timer.data(), SIGNAL(timeout()), this, SLOT(grab()));
}

void GrabberBase::setGrabInterval(int msec)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO <<	this->metaObject()->className();
	m_timer->setInterval(msec);
}

void GrabberBase::startGrabbing()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	grabScreensCount = 0;
	m_timer->start();
}

void GrabberBase::stopGrabbing()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	DEBUG_MID_LEVEL << "grabbed" << grabScreensCount << "frames";
	m_timer->stop();
}

bool GrabberBase::isGrabbingStarted() const
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	return m_timer->isActive();
}

const GrabbedScreen * GrabberBase::screenOfRect(const QRect &rect) const
{
	QPoint center = rect.center();
	for (int i = 0; i < _screensWithWidgets.size(); ++i) {
		if (_screensWithWidgets[i].screenInfo.rect.contains(center))
			return &_screensWithWidgets[i];
	}
	for (int i = 0; i < _screensWithWidgets.size(); ++i) {
		if (_screensWithWidgets[i].screenInfo.rect.intersects(rect))
			return &_screensWithWidgets[i];
	}
	return NULL;
}

bool GrabberBase::isReallocationNeeded(const QList< ScreenInfo > &screensWithWidgets) const
{
	if (_screensWithWidgets.size() == 0 || screensWithWidgets.size() != _screensWithWidgets.size())
		return true;

	for (int i = 0; i < screensWithWidgets.size(); ++i) {
		if (screensWithWidgets[i].rect != _screensWithWidgets[i].screenInfo.rect)
			return true;
	}
	return false;
}

void GrabberBase::grab()
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << this->metaObject()->className();
	QList< ScreenInfo > screens2Grab;
	screens2Grab.reserve(5);
	screensWithWidgets(&screens2Grab, *_context->grabWidgets);
	if (screens2Grab.empty()) {
		qCritical() << Q_FUNC_INFO << "No screens with widgets found";
		emit frameGrabAttempted(GrabResultError);
		return;
	}
	if (isReallocationNeeded(screens2Grab)) {
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "reallocating";
		if (!reallocate(screens2Grab)) {
			qCritical() << Q_FUNC_INFO << " couldn't reallocate grabbing buffer";
			emit frameGrabAttempted(GrabResultError);
			return;
		}
	}
	_lastGrabResult = grabScreens();
	++grabScreensCount;
	if (_lastGrabResult == GrabResultOk) {
		_context->grabResult->clear();

		for (int i = 0; i < _context->grabWidgets->size(); ++i) {
			QRect widgetRect = _context->grabWidgets->at(i)->frameGeometry();
			getValidRect(widgetRect);

			const GrabbedScreen *grabbedScreen = screenOfRect(widgetRect);
			if (grabbedScreen == NULL) {
				DEBUG_HIGH_LEVEL << Q_FUNC_INFO << " widget is out of screen " << Debug::toString(widgetRect);
				_context->grabResult->append(0);
				continue;
			}
			DEBUG_HIGH_LEVEL << Q_FUNC_INFO << Debug::toString(widgetRect);
			QRect monitorRect = grabbedScreen->screenInfo.rect;

			QRect clippedRect = monitorRect.intersected(widgetRect);

			// Checking for the 'grabme' widget position inside the monitor that is used to capture color
			if( !clippedRect.isValid() ){

				DEBUG_HIGH_LEVEL << "Widget 'grabme' is out of screen:" << Debug::toString(clippedRect);

				_context->grabResult->append(qRgb(0,0,0));
				continue;
			}

			// Convert coordinates from "Main" desktop coord-system to capture-monitor coord-system
			QRect preparedRect = clippedRect.translated(-monitorRect.x(), -monitorRect.y());

			// Align width by 4 for accelerated calculations
			preparedRect.setWidth(preparedRect.width() - (preparedRect.width() % 4));

			if( !preparedRect.isValid() ){
				qWarning() << Q_FUNC_INFO << " preparedRect is not valid:" << Debug::toString(preparedRect);
				// width and height can't be negative

				_context->grabResult->append(qRgb(0,0,0));
				continue;
			}

			using namespace Grab;
			const int bytesPerPixel = 4;
			QRgb avgColor;
			if (_context->grabWidgets->at(i)->isAreaEnabled()) {
				Q_ASSERT(grabbedScreen->imgData);
				Calculations::calculateAvgColor(
					&avgColor, grabbedScreen->imgData, grabbedScreen->imgFormat,
					grabbedScreen->screenInfo.rect.width() * bytesPerPixel,
					preparedRect);
				_context->grabResult->append(avgColor);
			} else {
				_context->grabResult->append(qRgb(0,0,0));
			}
		}

	}
	emit frameGrabAttempted(_lastGrabResult);
}

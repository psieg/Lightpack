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

#include <QGuiApplication>
#include "GrabberContext.hpp"
#include "GrabWidget.hpp"
#include "GrabberBase.hpp"
#include "src/debug.h"
#include <cmath>

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

	if (_lastGrabResult == GrabResultOk) {
		++grabScreensCount;
		_context->grabResult->clear();

		for (GrabWidget* grabWidget: *_context->grabWidgets) {
			if (!grabWidget->isAreaEnabled()) {
				_context->grabResult->append(qRgb(0,0,0));
				continue;
			}
			QRect widgetRect = grabWidget->frameGeometry();
			const qreal scale = qApp->devicePixelRatio();
			widgetRect = QRect(widgetRect.topLeft() * scale, widgetRect.size() * scale);
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

			// grabbed screen is rotated => rotate the widget
			if (grabbedScreen->rotation != 0) {
				if (grabbedScreen->rotation % 4 == 1) { // rotated 90
					preparedRect.setCoords(
						monitorRect.height() - preparedRect.bottom(),
						preparedRect.left(),
						monitorRect.height() - preparedRect.top(),
						preparedRect.right()
					);
				} else if (grabbedScreen->rotation % 4 == 2) { // rotated 180
					preparedRect.setCoords(
						monitorRect.width() - preparedRect.right(),
						monitorRect.height() - preparedRect.bottom(),
						monitorRect.width() - preparedRect.left(),
						monitorRect.height() - preparedRect.top()
					);
				} else if (grabbedScreen->rotation % 4 == 3) { // rotated 270
					preparedRect.setCoords(
						preparedRect.top(),
						monitorRect.width() - preparedRect.right(),
						preparedRect.bottom(),
						monitorRect.width() - preparedRect.left()
					);
				}
			}

			// grabbed screen was scaled => scale the widget
			if (grabbedScreen->scale != 1.0)
				preparedRect.setCoords(
					std::ceil(grabbedScreen->scale * preparedRect.left()),
					std::ceil(grabbedScreen->scale * preparedRect.top()),
					std::floor(grabbedScreen->scale * preparedRect.right()),
					std::floor(grabbedScreen->scale * preparedRect.bottom())
				);

			if( !preparedRect.isValid() ){
				qWarning() << Q_FUNC_INFO << " preparedRect is not valid:" << Debug::toString(preparedRect);
				// width and height can't be negative

				_context->grabResult->append(qRgb(0,0,0));
				continue;
			}

			const int bytesPerPixel = 4;
			Q_ASSERT(grabbedScreen->imgData);
			QRgb avgColor = Grab::Calculations::calculateAvgColor(
				grabbedScreen->imgData, grabbedScreen->imgFormat,
				grabbedScreen->bytesPerRow > 0 ? grabbedScreen->bytesPerRow : grabbedScreen->screenInfo.rect.width() * bytesPerPixel,
				preparedRect);
			_context->grabResult->append(avgColor);
		}

	}
	emit frameGrabAttempted(_lastGrabResult);
}

/*
 * GrabManager.hpp
 *
 *	Created on: 26.07.2010
 *		Project: Lightpack
 *
 *	Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#include <QtGui>

#include "GrabberBase.hpp"
#include "enums.hpp"

#if defined(Q_OS_WIN) && defined(NIGHTLIGHT_SUPPORT)
namespace NightLightLibrary { class NightLightWrapper; };
#endif // NIGHTLIGHT_SUPPORT

class GrabberContext;
class TimeEvaluations;
class D3D10Grabber;

class GrabManager : public QObject
{
	Q_OBJECT

public:
	GrabManager(QWidget *parent = 0);
	virtual ~GrabManager();

signals:
	void updateLedsColors(const QList<QRgb> & colors);
	void ambilightTimeOfUpdatingColors(double ms);
	void changeScreen();
	void onSessionChange(int change);

public:

	// Grab options

	// Common options
	void setNumberOfLeds(int numberOfLeds);
	void reset();

public slots:
	void onGrabberTypeChanged(const Grab::GrabberType grabberType);
	void onGrabSlowdownChanged(int ms);
	void onGrabAvgColorsEnabledChanged(bool state);
	void onGrabOverBrightenChanged(int value);
	void onGrabApplyGammaRampChanged(bool state);
	void onGrabApplyColorTemperatureChanged(bool state);
	void onGrabColorTemperatureChanged(int value);
	void onGrabGammaChanged(double value);
	void onSendDataOnlyIfColorsEnabledChanged(bool state);
#ifdef D3D10_GRAB_SUPPORT
	void onDx1011GrabberEnabledChanged(bool state);
	void onDx9GrabberEnabledChanged(bool state);
#endif
	void start(bool isGrabEnabled);
	void settingsProfileChanged(const QString &profileName);
	void setVisibleLedWidgets(bool state);
	void setColoredLedWidgets(bool state);
	void setWhiteLedWidgets(bool state);
	void onGrabberStateChangeRequested(bool isStartRequested);

private slots:
	void handleGrabbedColors();
	void timeoutFakeGrab();
	void timeoutUpdateFPS();
	void pauseWhileResizeOrMoving();
	void resumeAfterResizeOrMoving();
	void scaleLedWidgets(int screenIndexResized);
	void onFrameGrabAttempted(GrabResult result);
	void updateScreenGeometry();
	void onScreenCountChanged(int);

private:
	GrabberBase *queryGrabber(Grab::GrabberType grabber);
	void initGrabbers();
	GrabberBase *initGrabber(GrabberBase *grabber);
#ifdef D3D10_GRAB_SUPPORT
	void reinitDx1011Grabber();
#endif
	void initColorLists(int numberOfLeds);
	void clearColorsNew();
	void clearColorsCurrent();
	void initLedWidgets(int numberOfLeds);

private:
	QList<GrabberBase*> m_grabbers;
	GrabberBase *m_grabber;
	QList<QRect> m_lastScreenGeometry;

#ifdef D3D10_GRAB_SUPPORT
	D3D10Grabber *m_d3d10Grabber;
#endif

#if defined(Q_OS_WIN) && defined(NIGHTLIGHT_SUPPORT)
	std::unique_ptr<NightLightLibrary::NightLightWrapper> m_nightLight;
	void startNightLight();
	void stopNightLight();
#endif // NIGHTLIGHT_SUPPORT

	QTimer *m_timerUpdateFPS;
	QTimer *m_timerFakeGrab;
	QWidget *m_parentWidget;
	QList<GrabWidget *> m_ledWidgets;
	QList<QRgb> m_grabResult;
	const static QColor m_backgroundAndTextColors[10][2];

	QList<QRgb> m_colorsCurrent;
	QList<QRgb> m_colorsNew;
	QList<QRgb> m_colorsProcessing;

	QRect m_screenSavedRect;
	int m_screenSavedIndex;

	bool m_isPauseGrabWhileResizeOrMoving;
	bool m_isSendDataOnlyIfColorsChanged;
	bool m_avgColorsOnAllLeds;
	bool m_isGrabbingStarted;
	int m_overBrighten;
	bool m_isApplyGammaRamp;
	bool m_isApplyColorTemperature;
	double m_gamma;
	int m_colorTemperature;

	int m_grabCountThisInterval;
	int m_grabCountLastInterval;

	bool m_isGrabWidgetsVisible;
	GrabberContext * m_grabberContext;
};
/*
 * LightpackApplication.hpp
 *
 *	Created on: 06.09.2011
 *		Author: Mike Shatohin (brunql)
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

#pragma once

#include "SettingsWindow.hpp"
#include "LedDeviceManager.hpp"
#include "qtsingleapplication.h"

#include <memory>

#define getLightpackApp() static_cast<LightpackApplication *>(QCoreApplication::instance())

class LightpackPluginInterface;
class ApiServer;
class PluginsManager;

class LightpackApplication : public QtSingleApplication
{
	Q_OBJECT
public:
	LightpackApplication(int &argc, char **argv);
	~LightpackApplication();

	void initializeAll(const QString & appDirPath);
#ifdef Q_OS_WIN
	bool winEventFilter ( MSG * msg, long * result );
	HWND getMainWindowHandle();
#endif
	SettingsWindow * settingsWnd() { return m_settingsWindow; }
	const SettingsScope::Settings * settings() { return SettingsScope::Settings::settingsSingleton(); }
	enum ErrorCodes {
		OK_ErrorCode							= 0,
		WrongCommandLineArgument_ErrorCode		= 1,
		AppDirectoryCreationFail_ErrorCode		= 2,
		OpenLogsFail_ErrorCode					= 3,
		QFatalMessageHandler_ErrorCode			= 4,
		LogsDirecroryCreationFail_ErrorCode		= 5,
		// Append new ErrorCodes here
		JustEpicFail_ErrorCode					= 93
	};

signals:
	void clearColorBuffers();
	void postInitialization(); /*!< emits at the end of initializeAll method*/

public slots:
	void setStatusChanged(Backlight::Status);
	void setBacklightChanged(Lightpack::Mode);

private slots:
	void requestBacklightStatus();
	void setDeviceLockViaAPI(DeviceLocked::DeviceLockStatus status, QList<QString> modules);
	void profileSwitch(const QString & configName);
	void settingsChanged();
//	void numberOfLedsChanged(int);
	void showLedWidgets(bool visible);
	void setColoredLedWidget(bool colored);
//	void handleConnectedDeviceChange(const SupportedDevices::DeviceType);
	void onFocusChanged(QWidget *, QWidget *);
	void quitFromWizard(int result);
	void onSessionChange(int change);
	void openMoodLampScriptDir();

private:
	void processCommandLineArguments();
	void outputMessage(QString message) const;
	void printVersionsSoftwareQtOS() const;
	bool checkSystemTrayAvailability() const;
	void startApiServer();
	void startLedDeviceManager();
	void initGrabManager();
	void startPluginManager();
	void startBacklight();

	void runWizardLoop(bool isInitFromSettings);

	virtual void commitData(QSessionManager &sessionManager);

public:
	QMutex m_mutex;

private:
	SettingsWindow *m_settingsWindow{nullptr};
	ApiServer *m_apiServer{nullptr};
	LedDeviceManager *m_ledDeviceManager{nullptr};
	QThread *m_ledDeviceManagerThread{nullptr};
	QThread *m_apiServerThread{nullptr};
	GrabManager *m_grabManager{nullptr};
	MoodLampManager *m_moodlampManager{nullptr};
#ifdef SOUNDVIZ_SUPPORT
	SoundManagerBase *m_soundManager{nullptr};
#endif

	PluginsManager *m_pluginManager{nullptr};
	LightpackPluginInterface *m_pluginInterface{nullptr};
	QWidget *consolePlugin{nullptr};

	QString m_applicationDirPath;
	bool m_isDebugLevelObtainedFromCmdArgs;
	bool m_noGui;
	DeviceLocked::DeviceLockStatus m_deviceLockStatus;
	bool m_isSettingsWindowActive;
	Backlight::Status m_backlightStatus;

	typedef std::vector<QSharedPointer<QAbstractNativeEventFilter> > EventFilters;
	EventFilters m_EventFilters;

	bool m_isLightsTurnedOffBySessionChange;
	bool m_isSessionLocked;
	bool m_isDisplayOff;
	bool m_isSuspending;
	bool m_isLightsWereOnBeforeLock;
	bool m_isLightsWereOnBeforeDisplaySleep;
	bool m_isLightsWereOnBeforeSuspend;
};

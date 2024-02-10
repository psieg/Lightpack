/*
 * LightpackApplication.cpp
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

#include "LightpackApplication.hpp"
#include "LedDeviceLightpack.hpp"
#include "version.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include "ApiServer.hpp"
#include "LightpackPluginInterface.hpp"
#include "PluginsManager.hpp"
#include "wizard/Wizard.hpp"
#include "Plugin.hpp"
#include "SystemSession.hpp"
#include "LightpackCommandLineParser.hpp"

#ifdef Q_OS_WIN
#include "WinUtils.hpp"
#endif

#include <stdio.h>
#include <iostream>

using namespace std;
using namespace SettingsScope;

LightpackApplication::LightpackApplication(int &argc, char **argv)
	: QtSingleApplication(argc, argv)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	initializeAll();
}

LightpackApplication::~LightpackApplication()
{

	m_moodlampManager->start(false);
	m_grabManager->start(false);
	m_pluginManager->StopPlugins();

	QApplication::processEvents(QEventLoop::AllEvents, 1000);

	m_EventFilters.clear();

	m_ledDeviceManagerThread->quit();
	m_apiServerThread->quit();
	m_ledDeviceManagerThread->wait();
	m_apiServerThread->wait();

	delete m_settingsWindow;
	m_settingsWindow = NULL;
	delete m_ledDeviceManager;
	m_ledDeviceManager = NULL;
	delete m_ledDeviceManagerThread;
	m_ledDeviceManagerThread = NULL;
	delete m_apiServerThread;
	m_apiServerThread = NULL;
	delete m_grabManager;
	m_grabManager = NULL;
#ifdef SOUNDVIZ_SUPPORT
	if (m_soundManager)
		delete m_soundManager;
	m_soundManager = NULL;
#endif// SOUNDVIZ_SUPPORT
	delete m_moodlampManager;
	m_moodlampManager = NULL;

	delete m_pluginManager;
	m_pluginManager = NULL;
	delete m_pluginInterface;
	m_pluginInterface = NULL;
}

const QString& LightpackApplication::configDir() {
	return m_configDirPath;
}


void LightpackApplication::determineConfigDir(QString overrideDir)
{
	QString appDirPath = applicationDirPath();

	QString lightpackMainConfPath = appDirPath + QStringLiteral("/main.conf");

	cout << lightpackMainConfPath.toStdString() << endl;

	if (overrideDir.isEmpty() && QFile::exists(lightpackMainConfPath))
	{
		/// Portable version
		/// Store logs and settings in application directory
		cout << "Portable version" << endl;
	}
	else
	{
		/// Unportable version
		/// Store logs and settings in home directory of the current user
		cout << "Unportable version" << endl;

		QString home = QDir::homePath();
		QString normalizedHome = home.endsWith(QStringLiteral("/")) ? home.left(home.size() - 1) : home;

#		ifdef Q_OS_WIN
		appDirPath = normalizedHome + QStringLiteral("/Prismatik");
#		else
		appDirPath = normalizedHome + QStringLiteral("/.Prismatik");
#		endif
		if (!overrideDir.isEmpty())
			appDirPath = overrideDir;

		QDir dir(appDirPath);
		if (dir.exists() == false)
		{
			cout << "mkdir " << appDirPath.toStdString() << endl;
			if (dir.mkdir(appDirPath) == false)
			{
				cerr << "Failed mkdir '" << appDirPath.toStdString() << "' for application generated stuff. Exit." << endl;
				exit(LightpackApplication::AppDirectoryCreationFail_ErrorCode);
			}
		}
	}

	cout << "Configuration directory: " << appDirPath.toStdString() << endl;

	m_configDirPath = appDirPath;
	addLibraryPath(appDirPath + QStringLiteral("/plugins"));
	setId("Prismatik-" + m_configDirPath);
}


void LightpackApplication::initializeAll()
{
	setApplicationName(QStringLiteral("Prismatik"));
	setOrganizationName(QStringLiteral("Woodenshark LLC"));
	setApplicationVersion(QStringLiteral(VERSION_STR));
	setQuitOnLastWindowClosed(false);

	m_noGui = false;
	m_isSessionLocked = false;

	processCommandLineArguments();

	printVersionsSoftwareQtOS();

	if (!Settings::Initialize(m_configDirPath, m_isDebugLevelObtainedFromCmdArgs)
			&& !m_noGui) {
		runWizardLoop(false);
	}

	m_isSettingsWindowActive = false;

	if (!m_noGui)
	{
		bool trayAvailable = checkSystemTrayAvailability();

		m_settingsWindow = new SettingsWindow();
		if (trayAvailable) {
			m_settingsWindow->setVisible(false); /* Load to tray */
			m_settingsWindow->createTrayIcon();
		}
		else
			m_settingsWindow->setVisible(true);
		m_settingsWindow->connectSignalsSlots();
		connect(this, &LightpackApplication::postInitialization, m_settingsWindow, &SettingsWindow::onPostInit);
//		m_settingsWindow->profileSwitch(Settings::getCurrentProfileName());
		// Process messages from another instances in SettingsWindow
		connect(this, &LightpackApplication::messageReceived, m_settingsWindow, &SettingsWindow::processMessage);
		connect(this, &LightpackApplication::focusChanged, this, &LightpackApplication::onFocusChanged);
	} else
		connect(this, &LightpackApplication::messageReceived, this, &LightpackApplication::processMessageWithNoGui);

	// Register QMetaType for Qt::QueuedConnection
	qRegisterMetaType< QList<QRgb> >("QList<QRgb>");
	qRegisterMetaType< QList<QString> >("QList<QString>");
	qRegisterMetaType<Lightpack::Mode>("Lightpack::Mode");
	qRegisterMetaType<Backlight::Status>("Backlight::Status");
	qRegisterMetaType<DeviceLocked::DeviceLockStatus>("DeviceLocked::DeviceLockStatus");
	qRegisterMetaType< QList<Plugin*> >("QList<Plugin*>");


	if (Settings::isBacklightEnabled())
	{
		m_backlightStatus = Backlight::StatusOn;
	} else {
		m_backlightStatus = Backlight::StatusOff;
	}
	m_deviceLockStatus = DeviceLocked::Unlocked;

	startLedDeviceManager();

	startApiServer();

	initGrabManager();

	if (!m_noGui && m_settingsWindow)
	{
		connect(m_settingsWindow, &SettingsWindow::backlightStatusChanged, this, &LightpackApplication::setStatusChanged);
		m_settingsWindow->startBacklight();
	}

	this->settingsChanged();

//	handleConnectedDeviceChange(settings()->getConnectedDevice());

	startPluginManager();

    SystemSession::EventFilter* eventFilter = SystemSession::EventFilter::create();
    if (eventFilter)
    {
        QSharedPointer<SystemSession::EventFilter> sessionChangeDetector(eventFilter);
        connect(sessionChangeDetector.data(), &SystemSession::EventFilter::sessionChangeDetected, this, &LightpackApplication::onSessionChange);
        connect(sessionChangeDetector.data(), &SystemSession::EventFilter::sessionChangeDetected, m_grabManager, &GrabManager::onSessionChange);
        m_EventFilters.push_back(sessionChangeDetector);
    }
	for (EventFilters::const_iterator it = m_EventFilters.cbegin(); it != m_EventFilters.cend(); ++it)
		this->installNativeEventFilter(it->data());

	emit postInitialization();
}

void LightpackApplication::runWizardLoop(bool isInitFromSettings)
{
	Wizard *w = new Wizard(isInitFromSettings);
	connect(w, &Wizard::finished, this, &LightpackApplication::quitFromWizard);
	w->setWindowFlags(Qt::WindowStaysOnTopHint);
	w->show();
	this->exec();
	delete w;
}

#ifdef Q_OS_WIN
HWND LightpackApplication::getMainWindowHandle() {
	// to get HWND sometimes needed to activate window
//	winFocus(m_settingsWindow, true);
	return reinterpret_cast<HWND>(m_settingsWindow->winId());
}
#endif

void LightpackApplication::setStatusChanged(Backlight::Status status)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << status;
	m_backlightStatus = status;

	// In ApiPersist mode, we keep the colors left behind by API until the Backlight is restarted by some action
	if (m_deviceLockStatus == DeviceLocked::ApiPersist) {
		setDeviceLockViaAPI(DeviceLocked::Unlocked, QList<QString>());
		m_settingsWindow->setDeviceLockViaAPI(DeviceLocked::Unlocked, QList<QString>());
	}
	startBacklight();
}

void LightpackApplication::setBacklightChanged(Lightpack::Mode mode)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << mode;
	Settings::setLightpackMode(mode);
	if (!m_noGui && m_settingsWindow)
		m_settingsWindow->setModeChanged(mode);

	// In ApiPersist mode, we keep the colors left behind by API until the Backlight is restarted by some action
	if (m_deviceLockStatus == DeviceLocked::ApiPersist) {
		setDeviceLockViaAPI(DeviceLocked::Unlocked, QList<QString>());
		m_settingsWindow->setDeviceLockViaAPI(DeviceLocked::Unlocked, QList<QString>());
	}
	startBacklight();
}

void LightpackApplication::setDeviceLockViaAPI(const DeviceLocked::DeviceLockStatus status, const QList<QString>& modules)
{
	Q_UNUSED(modules);
	m_deviceLockStatus = status;

	if (m_grabManager == NULL)
		qFatal("%s m_grabManager == NULL", Q_FUNC_INFO);
	startBacklight();
}

void LightpackApplication::startBacklight()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "m_backlightStatus =" << m_backlightStatus
					<< "m_deviceLockStatus =" << m_deviceLockStatus;

	bool isBacklightEnabled = (m_backlightStatus == Backlight::StatusOn || m_backlightStatus == Backlight::StatusDeviceError);
	bool isCanStart = (isBacklightEnabled && m_deviceLockStatus == DeviceLocked::Unlocked);

	m_pluginInterface->resultBacklightStatus(m_backlightStatus);

	Settings::setIsBacklightEnabled(isBacklightEnabled);

	const Lightpack::Mode lightpackMode = Settings::getLightpackMode();
	switch (lightpackMode)
	{
	case Lightpack::AmbilightMode:
		m_grabManager->start(isCanStart);
		m_moodlampManager->start(false);
#ifdef SOUNDVIZ_SUPPORT
		if (m_soundManager)
			m_soundManager->start(false);
#endif
		break;

	case Lightpack::MoodLampMode:
		m_grabManager->start(false);
		m_moodlampManager->start(isCanStart);
#ifdef SOUNDVIZ_SUPPORT
		if (m_soundManager)
			m_soundManager->start(false);
#endif
		break;

#ifdef SOUNDVIZ_SUPPORT
	case Lightpack::SoundVisualizeMode:
		m_grabManager->start(false);
		m_moodlampManager->start(false);
		if (m_soundManager)
			m_soundManager->start(isCanStart);
		break;
#endif

	default:
		qWarning() << Q_FUNC_INFO << "lightpackMode unsupported value =" << lightpackMode;
		break;
	}

	if (m_backlightStatus == Backlight::StatusOff)
		QMetaObject::invokeMethod(m_ledDeviceManager, "switchOffLeds", Qt::QueuedConnection);
	else
		QMetaObject::invokeMethod(m_ledDeviceManager, "switchOnLeds", Qt::QueuedConnection);


	switch (m_backlightStatus)
	{
	case Backlight::StatusOff:
		emit clearColorBuffers();
		break;

	case Backlight::StatusOn:
		break;

	default:
		qWarning() << Q_FUNC_INFO << "status contains crap =" << m_backlightStatus;
		break;
	}
}
void LightpackApplication::onFocusChanged(QWidget *old, QWidget *now)
{
	Q_UNUSED(now);
	Q_UNUSED(old);
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << (this->activeWindow() != NULL ? this->activeWindow()->metaObject()->className() : "null");
	if(!m_isSettingsWindowActive && (this->activeWindow() == m_settingsWindow)) {
		m_settingsWindow->onFocus();
		m_isSettingsWindowActive = true;
	} else {
		if(m_isSettingsWindowActive && (this->activeWindow() == NULL)) {
			m_settingsWindow->onBlur();
			m_isSettingsWindowActive = false;
		}
	}
}

void LightpackApplication::processMessageWithNoGui(const QString& message)
{
	qWarning() << Q_FUNC_INFO << "Cannot process " << message << " with --nogui. Use the network API as an alternative.";
}

void LightpackApplication::quitFromWizard(int result)
{
	Q_UNUSED(result);
	quit();
}

void LightpackApplication::onSessionChange(int change)
{
	switch (change)
	{
		case SystemSession::Status::Ending:
			if (!SettingsScope::Settings::isKeepLightsOnAfterExit())
			{
				// Process all currently pending signals (which may include updating the color signals)
				QApplication::processEvents(QEventLoop::AllEvents, 500);

				emit getLightpackApp()->settingsWnd()->switchOffLeds();
				QApplication::processEvents(QEventLoop::AllEvents, 500);
			}
			break;
		case SystemSession::Status::Locking:
			if (!m_isSessionLocked)
				m_isLightsWereOnBeforeLock = m_backlightStatus && !m_isLightsTurnedOffBySessionChange;

			if (!SettingsScope::Settings::isKeepLightsOnAfterLock())
			{
				emit getLightpackApp()->settingsWnd()->switchOffLeds();
				m_isLightsTurnedOffBySessionChange = true;
			}
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Sleeping, isSessionLocked:" << m_isSessionLocked << "isLightsWereOnBeforeLock:" << m_isLightsWereOnBeforeLock << "isLightsTurnedOffBySessionChange : " << m_isLightsTurnedOffBySessionChange;
			m_isSessionLocked = true;
			break;
		case SystemSession::Status::Unlocking:
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Unlocking, isSessionLocked:" << m_isSessionLocked << "isLightsWereOnBeforeLock:" << m_isLightsWereOnBeforeLock;
			if (m_isSessionLocked && !SettingsScope::Settings::isKeepLightsOnAfterLock() && m_isLightsWereOnBeforeLock)
			{
				emit getLightpackApp()->settingsWnd()->switchOnLeds();
				m_isLightsTurnedOffBySessionChange = false;
			}
			m_isSessionLocked = false;
			break;
		case SystemSession::Status::Sleeping:
			if (!m_isSuspending)
				m_isLightsWereOnBeforeSuspend = m_backlightStatus && !m_isLightsTurnedOffBySessionChange;

			if (!SettingsScope::Settings::isKeepLightsOnAfterSuspend())
			{
				emit getLightpackApp()->settingsWnd()->switchOffLeds();
				m_isLightsTurnedOffBySessionChange = true;
			}

			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Sleeping, isSuspending:" << m_isSuspending << "isLightsWereOnBeforeSuspend:" << m_isLightsWereOnBeforeSuspend << "isLightsTurnedOffBySessionChange : " << m_isLightsTurnedOffBySessionChange;
			m_isSuspending = true;
			break;
		case SystemSession::Status::Resuming:
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Resuming, isSuspending:" << m_isSuspending << "isLightsWereOnBeforeSuspend:" << m_isLightsWereOnBeforeSuspend;

			if (m_isSuspending && !SettingsScope::Settings::isKeepLightsOnAfterSuspend() && m_isLightsWereOnBeforeSuspend)
			{
				emit getLightpackApp()->settingsWnd()->switchOnLeds();
				m_isLightsTurnedOffBySessionChange = false;
			}
			if (m_isSuspending)
				QMetaObject::invokeMethod(m_ledDeviceManager, "updateDeviceSettings", Qt::QueuedConnection);

			m_isSuspending = false;
			break;
		case SystemSession::Status::DisplayOff:
			if (!m_isDisplayOff)
				m_isLightsWereOnBeforeDisplaySleep = m_backlightStatus && !m_isLightsTurnedOffBySessionChange;

			if (!SettingsScope::Settings::isKeepLightsOnAfterScreenOff())
			{
				emit getLightpackApp()->settingsWnd()->switchOffLeds();
				m_isLightsTurnedOffBySessionChange = true;
			}
			m_isDisplayOff = true;
			break;
		case SystemSession::Status::DisplayOn:
			if (m_isDisplayOff && !SettingsScope::Settings::isKeepLightsOnAfterScreenOff() && m_isLightsWereOnBeforeDisplaySleep)
			{
				emit getLightpackApp()->settingsWnd()->switchOnLeds();
				m_isLightsTurnedOffBySessionChange = false;
			}
			m_isDisplayOff = false;
			break;
	}
}

void LightpackApplication::processCommandLineArguments()
{
	g_debugLevel = SettingsScope::Main::DebugLevelDefault;

	m_isDebugLevelObtainedFromCmdArgs = false;

	LightpackCommandLineParser parser;
	if (!parser.parse(arguments())) {
		outputMessage(parser.errorText());
		::exit(WrongCommandLineArgument_ErrorCode);
	}

	if (parser.isSetHelp()) {
		outputMessage(parser.helpText());
		::exit(0);
	}
	if (parser.isSetVersion())
	{
		const QString versionString =
			QStringLiteral("Version: " VERSION_STR "\n") +
#ifdef GIT_REVISION
			QStringLiteral("Revision: " GIT_REVISION "\n") +
#endif
			QStringLiteral("Build with Qt version " QT_VERSION_STR "\n");
		outputMessage(versionString);
		::exit(0);
	}

	bool ignore_already_running = false;
	if (parser.isSetConfigDir()) {
		determineConfigDir(parser.configDir());
		//ignore_already_running = true;
	} else {
		determineConfigDir();
	}

	// these two options are mutually exclusive
	if (parser.isSetNoGUI()) {
		m_noGui = true;
		DEBUG_LOW_LEVEL <<	"Application running no_GUI mode";
	} else if (parser.isSetWizard()) {
		if (isRunning()) {
			DEBUG_LOW_LEVEL << "Application still running, telling it to quit";
			sendMessage(QStringLiteral("quitForWizard"));
			while (isRunning()) {
				QThread::sleep(200);
			}
		}
		bool isInitFromSettings = Settings::Initialize(m_configDirPath, false);
		runWizardLoop(isInitFromSettings);
	}

	// 'on' and 'off' are mutually exclusive also
	if (parser.isSetBacklightOff()) {
		if (!isRunning()) {
			LedDeviceLightpack lightpackDevice;
			lightpackDevice.switchOffLeds();
		} else {
			sendMessage(QStringLiteral("off"));
		}
		::exit(0);
	}
	else if (parser.isSetBacklightOn())
	{
		if (isRunning())
			sendMessage(QStringLiteral("on"));
		::exit(0);
	}

	if (parser.isSetProfile()) {
		if (isRunning())
			sendMessage(QStringLiteral("set-profile ") + parser.profileName());
		::exit(0);
	}

	if (!ignore_already_running && isRunning()) {
		sendMessage(QStringLiteral("alreadyRunning"));
		qWarning() << "Application already running";
		::exit(0);
	}

	if (parser.isSetDebuglevel())
	{
		g_debugLevel = parser.debugLevel();
		m_isDebugLevelObtainedFromCmdArgs = true;
	}

	if (m_isDebugLevelObtainedFromCmdArgs)
		qDebug() << "Debug level" << g_debugLevel;
}

void LightpackApplication::outputMessage(const QString& message) const
{
#ifdef Q_OS_WIN
	QMessageBox::information(NULL, QStringLiteral("Prismatik"), message, QMessageBox::Ok);
#else
	fprintf(stderr, "%s\n", message.toStdString().c_str());
#endif
}

void LightpackApplication::printVersionsSoftwareQtOS() const
{
#	ifdef GIT_REVISION
	qDebug() << "Prismatik:" << VERSION_STR << "rev." << GIT_REVISION;
#	else
	qDebug() << "Prismatik:" << VERSION_STR;
#	endif

	qDebug() << "Build with Qt verison:" << QT_VERSION_STR;
	qDebug() << "Qt version currently in use:" << qVersion();

	qDebug() << "Running on:" << QSysInfo::prettyProductName();

#		ifdef Q_OS_WIN
	if (WinUtils::IsUserAdmin()) {
		qDebug() << "App is running as Admin";
	} else {
		qDebug() << "Not running as Admin";
	}
#		endif
}

bool LightpackApplication::checkSystemTrayAvailability() const
{
#ifdef Q_OS_LINUX
	{
		// When you add lightpack in the Startup in Ubuntu (10.04), tray starts later than the application runs.
		// Check availability tray every second for 20 seconds.
		for (int i = 0; i < 20; i++)
		{
			if (QSystemTrayIcon::isSystemTrayAvailable())
				break;

			QThread::sleep(1);
		}
	}
#endif

	if (QSystemTrayIcon::isSystemTrayAvailable() == false)
	{
		QMessageBox::critical(0, QStringLiteral("Prismatik"), QStringLiteral("I couldn't detect any system tray on this system."));
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Systray couldn't be detected, running in trayless mode";
		return false;
	}
	return true;
}

void LightpackApplication::startApiServer()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Start API server";

	m_apiServer = new ApiServer();
	m_apiServer->setInterface(m_pluginInterface);
	m_apiServerThread = new QThread();

	connect(this, &LightpackApplication::clearColorBuffers, m_apiServer, &ApiServer::clearColorBuffers);

	connect(settings(), &Settings::apiServerSettingsChanged,		m_apiServer, &ApiServer::apiServerSettingsChanged);
	connect(settings(), &Settings::apiKeyChanged, m_apiServer, &ApiServer::updateApiKey);

	connect(settings(), &Settings::lightpackNumberOfLedsChanged,	m_apiServer, &ApiServer::updateApiDeviceNumberOfLeds);
	connect(settings(), &Settings::adalightNumberOfLedsChanged,	m_apiServer, &ApiServer::updateApiDeviceNumberOfLeds);
	connect(settings(), &Settings::ardulightNumberOfLedsChanged,	m_apiServer, &ApiServer::updateApiDeviceNumberOfLeds);
	connect(settings(), &Settings::virtualNumberOfLedsChanged,	m_apiServer, &ApiServer::updateApiDeviceNumberOfLeds);

	if (!m_noGui)
	{
		connect(m_apiServer, &ApiServer::errorOnStartListening, m_settingsWindow, &SettingsWindow::onApiServer_ErrorOnStartListening);
	}

	connect(m_ledDeviceManager, &LedDeviceManager::ledDeviceSetColors,	m_pluginInterface, &LightpackPluginInterface::updateColorsCache,	Qt::QueuedConnection);
	connect(m_ledDeviceManager, &LedDeviceManager::ledDeviceSetSmoothSlowdown,			m_pluginInterface, &LightpackPluginInterface::updateSmoothCache,					Qt::QueuedConnection);
	connect(m_ledDeviceManager, &LedDeviceManager::ledDeviceSetGamma,			m_pluginInterface, &LightpackPluginInterface::updateGammaCache,					Qt::QueuedConnection);
	connect(m_ledDeviceManager, &LedDeviceManager::ledDeviceSetBrightness,			m_pluginInterface, &LightpackPluginInterface::updateBrightnessCache,				Qt::QueuedConnection);

#ifdef SOUNDVIZ_SUPPORT
	connect(settings(), &Settings::soundVisualizerMinColorChanged, m_pluginInterface, &LightpackPluginInterface::updateSoundVizMinColorCache, Qt::QueuedConnection);
	connect(settings(), &Settings::soundVisualizerMaxColorChanged, m_pluginInterface, &LightpackPluginInterface::updateSoundVizMaxColorCache, Qt::QueuedConnection);
	connect(settings(), &Settings::soundVisualizerLiquidModeChanged, m_pluginInterface, &LightpackPluginInterface::updateSoundVizLiquidCache, Qt::QueuedConnection);
#endif

	m_apiServer->firstStart();

	m_apiServer->moveToThread(m_apiServerThread);
	m_apiServerThread->start();
}

void LightpackApplication::startLedDeviceManager()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	m_ledDeviceManager = new LedDeviceManager();
	m_ledDeviceManagerThread = new QThread();

//	connect(settings(), &Settings::connectedDeviceChanged, this, &LightpackApplication::handleConnectedDeviceChange, Qt::DirectConnection);
//	connect(settings(), &Settings::adalightSerialPortNameChanged,				m_ledDeviceManager, &LedDeviceManager::recreateLedDevice, Qt::DirectConnection);
//	connect(settings(), &Settings::adalightSerialPortBaudRateChanged,			m_ledDeviceManager, &LedDeviceManager::recreateLedDevice, Qt::DirectConnection);
//	connect(m_settingsWindow, &SettingsWindow::updateLedsColors,			m_ledDeviceManager, &LedDeviceManager::setColors, Qt::QueuedConnection);
	m_pluginInterface = new LightpackPluginInterface(NULL);

	connect(m_pluginInterface, &LightpackPluginInterface::updateDeviceLockStatus, this, &LightpackApplication::setDeviceLockViaAPI);
	connect(m_pluginInterface, &LightpackPluginInterface::updateLedsColors,	m_ledDeviceManager, &LedDeviceManager::setColors, Qt::QueuedConnection);
	connect(m_pluginInterface, &LightpackPluginInterface::updateGamma,		m_ledDeviceManager, &LedDeviceManager::setGamma,		Qt::QueuedConnection);
	connect(m_pluginInterface, &LightpackPluginInterface::updateBrightness,	m_ledDeviceManager, &LedDeviceManager::setBrightness,		Qt::QueuedConnection);
	connect(m_pluginInterface, &LightpackPluginInterface::updateSmooth,		m_ledDeviceManager, &LedDeviceManager::setSmoothSlowdown, Qt::QueuedConnection);
	connect(m_pluginInterface, &LightpackPluginInterface::requestBacklightStatus,			this, &LightpackApplication::requestBacklightStatus);
	connect(m_pluginInterface, &LightpackPluginInterface::updateBacklight,	this, &LightpackApplication::setBacklightChanged);
//	connect(m_pluginInterface, &LightpackPluginInterface::changeDevice,		m_settingsWindow , SLOT(setDevice(QString)));
//	connect(m_pluginInterface, &LightpackPluginInterface::updateCountLeds,		m_settingsWindow , SLOT(updateUiFromSettings()));
//	connect(m_pluginInterface, &LightpackPluginInterface::updateCountLeds,		this , SLOT(numberOfLedsChanged(int)));

	if (!m_noGui)
	{
		connect(m_pluginInterface, &LightpackPluginInterface::updateLedsColors,	m_settingsWindow, &SettingsWindow::updateVirtualLedsColors);
		connect(m_pluginInterface, &LightpackPluginInterface::updateDeviceLockStatus, m_settingsWindow, &SettingsWindow::setDeviceLockViaAPI);
		connect(m_pluginInterface, &LightpackPluginInterface::updateProfile,						m_settingsWindow, &SettingsWindow::profileSwitch);
		connect(m_pluginInterface, &LightpackPluginInterface::updateStatus,				m_settingsWindow, &SettingsWindow::setBacklightStatus);
	}
	else
	{
		connect(m_pluginInterface, &LightpackPluginInterface::updateProfile,						this, &LightpackApplication::profileSwitch);
		connect(m_pluginInterface, &LightpackPluginInterface::updateStatus,				this, &LightpackApplication::setStatusChanged);
	}

	connect(settings(), &Settings::deviceColorDepthChanged,			m_ledDeviceManager, &LedDeviceManager::setColorDepth,					Qt::QueuedConnection);
	connect(settings(), &Settings::deviceSmoothChanged,				m_ledDeviceManager, &LedDeviceManager::setSmoothSlowdown,				Qt::QueuedConnection);
	connect(settings(), &Settings::deviceRefreshDelayChanged,			m_ledDeviceManager, &LedDeviceManager::setRefreshDelay,					Qt::QueuedConnection);
	connect(settings(), &Settings::deviceUsbPowerLedDisabledChanged, m_ledDeviceManager, &LedDeviceManager::setUsbPowerLedDisabled,			Qt::QueuedConnection);
	connect(settings(), &Settings::deviceGammaChanged,				m_ledDeviceManager, &LedDeviceManager::setGamma,						Qt::QueuedConnection);
	connect(settings(), &Settings::deviceDitheringEnabledChanged,	m_ledDeviceManager, &LedDeviceManager::setDitheringEnabled,			Qt::QueuedConnection);
	connect(settings(), &Settings::deviceBrightnessChanged,			m_ledDeviceManager, &LedDeviceManager::setBrightness,					Qt::QueuedConnection);
	connect(settings(), &Settings::deviceBrightnessCapChanged,		m_ledDeviceManager, &LedDeviceManager::setBrightnessCap,				Qt::QueuedConnection);
	connect(settings(), &Settings::luminosityThresholdChanged,		m_ledDeviceManager, &LedDeviceManager::setLuminosityThreshold,			Qt::QueuedConnection);
	connect(settings(), &Settings::minimumLuminosityEnabledChanged,	m_ledDeviceManager, &LedDeviceManager::setMinimumLuminosityEnabled,	Qt::QueuedConnection);
	connect(settings(), &Settings::ledCoefBlueChanged,			m_ledDeviceManager, &LedDeviceManager::updateWBAdjustments,				Qt::QueuedConnection);
	connect(settings(), &Settings::ledCoefRedChanged,			m_ledDeviceManager, &LedDeviceManager::updateWBAdjustments,				Qt::QueuedConnection);
	connect(settings(), &Settings::ledCoefGreenChanged,		m_ledDeviceManager, &LedDeviceManager::updateWBAdjustments,				Qt::QueuedConnection);


	if (!m_noGui)
	{
		connect(m_settingsWindow, &SettingsWindow::requestFirmwareVersion,			m_ledDeviceManager, &LedDeviceManager::requestFirmwareVersion,							Qt::QueuedConnection);
		connect(m_settingsWindow, &SettingsWindow::switchOffLeds,					m_ledDeviceManager, &LedDeviceManager::switchOffLeds,									Qt::QueuedConnection);
		connect(m_settingsWindow, &SettingsWindow::switchOnLeds,					m_ledDeviceManager, &LedDeviceManager::switchOnLeds,									Qt::QueuedConnection);
		connect(m_ledDeviceManager, &LedDeviceManager::openDeviceSuccess,		m_settingsWindow, &SettingsWindow::ledDeviceOpenSuccess,						Qt::QueuedConnection);
		connect(m_ledDeviceManager, &LedDeviceManager::ioDeviceSuccess,			m_settingsWindow, &SettingsWindow::ledDeviceCallSuccess,						Qt::QueuedConnection);
		connect(m_ledDeviceManager, &LedDeviceManager::firmwareVersion,		m_settingsWindow, &SettingsWindow::ledDeviceFirmwareVersionResult,			Qt::QueuedConnection);
		connect(m_ledDeviceManager, &LedDeviceManager::firmwareVersionUnofficial, m_settingsWindow, &SettingsWindow::ledDeviceFirmwareVersionUnofficialResult,	Qt::QueuedConnection);
		connect(m_ledDeviceManager, &LedDeviceManager::setColors_VirtualDeviceCallback, m_settingsWindow, &SettingsWindow::updateVirtualLedsColors, Qt::QueuedConnection);
	}
	m_ledDeviceManager->moveToThread(m_ledDeviceManagerThread);
	m_ledDeviceManagerThread->start();
	QMetaObject::invokeMethod(m_ledDeviceManager, "init", Qt::QueuedConnection);

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "end";

}

void LightpackApplication::startPluginManager()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_pluginManager = new PluginsManager(NULL);

	connect(this, &LightpackApplication::destroyed,m_pluginManager, &PluginsManager::StopPlugins);

	if (!m_noGui)
	{
		connect(m_settingsWindow, &SettingsWindow::reloadPlugins,m_pluginManager, &PluginsManager::reloadPlugins);
		connect(m_pluginManager, &PluginsManager::updatePlugin,m_settingsWindow, &SettingsWindow::updatePlugin, Qt::QueuedConnection);
		connect(m_pluginManager, &PluginsManager::updatePlugin,m_pluginInterface, &LightpackPluginInterface::updatePlugin, Qt::QueuedConnection);
	}

	m_pluginManager->LoadPlugins(QString(Settings::getApplicationDirPath() + QStringLiteral("Plugins")));
	m_pluginManager->StartPlugins();

}


void LightpackApplication::initGrabManager()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	m_grabManager = new GrabManager(NULL);

	m_moodlampManager = new MoodLampManager(NULL);
	m_moodlampManager->initFromSettings();

#ifdef SOUNDVIZ_SUPPORT
	m_soundManager = SoundManagerBase::create((!m_noGui && m_settingsWindow) ? m_settingsWindow->winId() : 0, NULL);
	if (m_soundManager)
		m_soundManager->initFromSettings();
	else
		qWarning() << Q_FUNC_INFO << "No compatible Sound Manager";
#endif

	connect(settings(), &Settings::grabberTypeChanged,	m_grabManager, &GrabManager::onGrabberTypeChanged,	Qt::QueuedConnection);
	connect(settings(), &Settings::grabSlowdownChanged,						m_grabManager, &GrabManager::onGrabSlowdownChanged,						Qt::QueuedConnection);
	connect(settings(), &Settings::grabAvgColorsEnabledChanged,				m_grabManager, &GrabManager::onGrabAvgColorsEnabledChanged,				Qt::QueuedConnection);
	connect(settings(), &Settings::grabOverBrightenChanged,					m_grabManager, &GrabManager::onGrabOverBrightenChanged,					Qt::QueuedConnection);
	connect(settings(), &Settings::grabApplyBlueLightReductionChanged,				m_grabManager, &GrabManager::onGrabApplyBlueLightReductionChanged,				Qt::QueuedConnection);
	connect(settings(), &Settings::grabApplyColorTemperatureChanged,         m_grabManager, &GrabManager::onGrabApplyColorTemperatureChanged,           Qt::QueuedConnection);
	connect(settings(), &Settings::grabColorTemperatureChanged,               m_grabManager, &GrabManager::onGrabColorTemperatureChanged,                 Qt::QueuedConnection);
	connect(settings(), &Settings::grabGammaChanged,                       m_grabManager, &GrabManager::onGrabGammaChanged,                         Qt::QueuedConnection);
	connect(settings(), &Settings::sendDataOnlyIfColorsChangesChanged,		m_grabManager, &GrabManager::onSendDataOnlyIfColorsEnabledChanged,		Qt::QueuedConnection);
#ifdef D3D10_GRAB_SUPPORT
	connect(settings(), &Settings::dx1011GrabberEnabledChanged,				m_grabManager, &GrabManager::onDx1011GrabberEnabledChanged,				Qt::QueuedConnection);
	connect(settings(), &Settings::dx9GrabberEnabledChanged,					m_grabManager, &GrabManager::onDx9GrabberEnabledChanged,					Qt::QueuedConnection);
#endif

	connect(settings(), &Settings::moodLampColorChanged,					m_moodlampManager, &MoodLampManager::setCurrentColor);
	connect(settings(), &Settings::moodLampSpeedChanged,						m_moodlampManager, &MoodLampManager::setLiquidModeSpeed);
	connect(settings(), &Settings::moodLampLiquidModeChanged,				m_moodlampManager, &MoodLampManager::setLiquidMode);
	connect(settings(), &Settings::moodLampLampChanged,						m_moodlampManager, &MoodLampManager::setCurrentLamp);
	connect(settings(), &Settings::sendDataOnlyIfColorsChangesChanged,		m_moodlampManager, &MoodLampManager::setSendDataOnlyIfColorsChanged);

#ifdef SOUNDVIZ_SUPPORT
	if (m_soundManager)
	{
		connect(settings(), &Settings::soundVisualizerDeviceChanged,				m_soundManager, &SoundManagerBase::setDevice);
		connect(settings(), &Settings::soundVisualizerVisualizerChanged,			m_soundManager, &SoundManagerBase::setVisualizer);
		connect(settings(), &Settings::soundVisualizerMaxColorChanged,			m_soundManager, &SoundManagerBase::setMaxColor);
		connect(settings(), &Settings::soundVisualizerMinColorChanged,			m_soundManager, &SoundManagerBase::setMinColor);
		connect(settings(), &Settings::soundVisualizerLiquidSpeedChanged,			m_soundManager, &SoundManagerBase::setLiquidModeSpeed);
		connect(settings(), &Settings::soundVisualizerLiquidModeChanged,			m_soundManager, &SoundManagerBase::setLiquidMode);
		connect(settings(), &Settings::sendDataOnlyIfColorsChangesChanged,		m_soundManager, &SoundManagerBase::setSendDataOnlyIfColorsChanged);

		connect(m_pluginInterface, &LightpackPluginInterface::updateSoundVizMinColor,			m_soundManager, &SoundManagerBase::setMinColor,								Qt::QueuedConnection);
		connect(m_pluginInterface, &LightpackPluginInterface::updateSoundVizMaxColor,			m_soundManager, &SoundManagerBase::setMaxColor,								Qt::QueuedConnection);
		connect(m_pluginInterface, &LightpackPluginInterface::updateSoundVizLiquid,				m_soundManager, &SoundManagerBase::setLiquidMode,								Qt::QueuedConnection);
	}
#endif

	connect(settings(), &Settings::currentProfileInited,			m_grabManager, &GrabManager::settingsProfileChanged,			Qt::QueuedConnection);

	connect(settings(), &Settings::currentProfileInited,			m_moodlampManager, &MoodLampManager::settingsProfileChanged,			Qt::QueuedConnection);
#ifdef SOUNDVIZ_SUPPORT
	if (m_soundManager)
		connect(settings(), &Settings::currentProfileInited,			m_soundManager, &SoundManagerBase::settingsProfileChanged);
#endif

	// Connections to signals which will be connected to ILedDevice
	if (!m_noGui && m_settingsWindow)
	{
		connect(m_settingsWindow, &SettingsWindow::showLedWidgets, this, &LightpackApplication::showLedWidgets);
		connect(m_settingsWindow, &SettingsWindow::setColoredLedWidget, this, &LightpackApplication::setColoredLedWidget);

		// GrabManager to this
		connect(m_grabManager, &GrabManager::ambilightTimeOfUpdatingColors, m_settingsWindow, &SettingsWindow::refreshAmbilightEvaluated);

#ifdef SOUNDVIZ_SUPPORT
		if (m_soundManager) {
			connect(m_settingsWindow, &SettingsWindow::requestSoundVizDevices, m_soundManager, &SoundManagerBase::requestDeviceList);
			connect(m_soundManager, &SoundManagerBase::deviceList, m_settingsWindow, &SettingsWindow::updateAvailableSoundVizDevices);

			connect(m_settingsWindow, &SettingsWindow::requestSoundVizVisualizers, m_soundManager, &SoundManagerBase::requestVisualizerList);
			connect(m_soundManager, &SoundManagerBase::visualizerList, m_settingsWindow, &SettingsWindow::updateAvailableSoundVizVisualizers);
		}
#endif

		connect(m_settingsWindow, &SettingsWindow::requestMoodLampLamps, m_moodlampManager, &MoodLampManager::requestLampList);
		connect(m_moodlampManager, &MoodLampManager::lampList, m_settingsWindow, &SettingsWindow::updateAvailableMoodLampLamps);
	}

	connect(m_grabManager, &GrabManager::ambilightTimeOfUpdatingColors, m_pluginInterface, &LightpackPluginInterface::refreshAmbilightEvaluated);

	connect(m_grabManager, &GrabManager::updateLedsColors,	m_ledDeviceManager, &LedDeviceManager::setColors, Qt::QueuedConnection);
	connect(m_moodlampManager, &MoodLampManager::updateLedsColors,	m_ledDeviceManager, &LedDeviceManager::setColors, Qt::QueuedConnection);
	if (!m_noGui && m_settingsWindow)
		connect(m_moodlampManager, &MoodLampManager::moodlampFrametime,		m_settingsWindow, &SettingsWindow::refreshAmbilightEvaluated, Qt::QueuedConnection);
	connect(m_ledDeviceManager, &LedDeviceManager::openDeviceSuccess,				m_grabManager, &GrabManager::ledDeviceOpenSuccess, Qt::QueuedConnection);
	connect(m_ledDeviceManager, &LedDeviceManager::ioDeviceSuccess,					m_grabManager, &GrabManager::ledDeviceCallSuccess, Qt::QueuedConnection);
#ifdef SOUNDVIZ_SUPPORT
	if (m_soundManager) {
		connect(m_soundManager, &SoundManagerBase::updateLedsColors,	m_ledDeviceManager, &LedDeviceManager::setColors, Qt::QueuedConnection);
		if (!m_noGui && m_settingsWindow)
			connect(m_soundManager, &SoundManagerBase::visualizerFrametime,	m_settingsWindow, &SettingsWindow::refreshAmbilightEvaluated, Qt::QueuedConnection);
	}
#endif
}

void LightpackApplication::commitData(QSessionManager &sessionManager)
{
	Q_UNUSED(sessionManager);

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Off leds before quit";

	if (m_ledDeviceManager != NULL)
	{
		// Disable signals with new colors
		disconnect(m_settingsWindow, &SettingsWindow::updateLedsColors, m_ledDeviceManager, &LedDeviceManager::setColors);

		// Process all currently pending signals
		QApplication::processEvents(QEventLoop::AllEvents, 1000);

		// Send signal and process it
		QMetaObject::invokeMethod(m_ledDeviceManager, "switchOffLeds", Qt::QueuedConnection);
		QApplication::processEvents(QEventLoop::AllEvents, 1000);
	}
}

void LightpackApplication::profileSwitch(const QString & configName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << configName;
	Settings::loadOrCreateProfile(configName);
}

void LightpackApplication::settingsChanged()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_pluginInterface->changeProfile(Settings::getCurrentProfileName());

	m_grabManager->onSendDataOnlyIfColorsEnabledChanged(Settings::isSendDataOnlyIfColorsChanges());

	m_moodlampManager->setSendDataOnlyIfColorsChanged(Settings::isSendDataOnlyIfColorsChanges());


	m_moodlampManager->setCurrentColor(Settings::getMoodLampColor());
	m_moodlampManager->setLiquidModeSpeed(Settings::getMoodLampSpeed());
	m_moodlampManager->setLiquidMode(Settings::isMoodLampLiquidMode());
	m_moodlampManager->setCurrentLamp(Settings::getMoodLampLamp());

	bool isBacklightEnabled = Settings::isBacklightEnabled();
	bool isCanStart = (isBacklightEnabled && m_deviceLockStatus == DeviceLocked::Unlocked);

//	numberOfLedsChanged(Settings::getNumberOfLeds(Settings::getConnectedDevice()));

#ifdef SOUNDVIZ_SUPPORT
	if (m_soundManager)
		m_soundManager->setSendDataOnlyIfColorsChanged(Settings::isSendDataOnlyIfColorsChanges());
#endif

	switch (Settings::getLightpackMode())
	{
	case Lightpack::AmbilightMode:
		m_grabManager->start(isCanStart);
		m_moodlampManager->start(false);
#ifdef SOUNDVIZ_SUPPORT
		if (m_soundManager)
			m_soundManager->start(false);
#endif
		break;

#ifdef SOUNDVIZ_SUPPORT
	case Lightpack::SoundVisualizeMode:
		m_grabManager->start(false);
		m_moodlampManager->start(false);
		if (m_soundManager)
			m_soundManager->start(isCanStart);
		break;
#endif

	default:
		m_grabManager->start(false);
		m_moodlampManager->start(isCanStart);
#ifdef SOUNDVIZ_SUPPORT
		if (m_soundManager)
			m_soundManager->start(false);
#endif
		break;
	}

	//Force update colors on device for start ping device
	m_grabManager->reset();
	m_moodlampManager->reset();
#ifdef SOUNDVIZ_SUPPORT
	if (m_soundManager)
		m_soundManager->reset();
#endif

}

void LightpackApplication::showLedWidgets(bool visible)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << visible;
	m_grabManager->setVisibleLedWidgets(visible);
}

void LightpackApplication::setColoredLedWidget(bool colored)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << colored;
	m_grabManager->setColoredLedWidgets(colored);
	m_grabManager->setWhiteLedWidgets(!colored);
}

void LightpackApplication::requestBacklightStatus()
{
	//m_apiServer->resultBacklightStatus(m_backlightStatus);
	m_pluginInterface->resultBacklightStatus(m_backlightStatus);
}

/*
 * Settings.cpp
 *
 *	Created on: 22.02.2011
 *		Author: Mike Shatohin (brunql)
 *		Project: Lightpack
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
 *
 *	Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#include "Settings.hpp"

#include <QtDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QSize>
#include <QPoint>
#include <QFileInfo>
#include <QDir>
#include <QUuid>
#include "debug.h"

#define MAIN_CONFIG_FILE_VERSION	"4.0"

namespace
{
inline const WBAdjustment getLedAdjustment(int ledIndex)
{
	using namespace SettingsScope;

	WBAdjustment wba;
	wba.red = Settings::getLedCoefRed(ledIndex);
	wba.green = Settings::getLedCoefGreen(ledIndex);
	wba.blue = Settings::getLedCoefBlue(ledIndex);
	return wba;
}
}

//
// This strings keys and values must be accessible only in current file
//
namespace SettingsScope
{
namespace Main
{
namespace Key
{
// [General]
static const QString MainConfigVersion = "MainConfigVersion";
static const QString ProfileLast = "ProfileLast";
static const QString Language = "Language";
static const QString DebugLevel = "DebugLevel";
static const QString IsExpertModeEnabled = "IsExpertModeEnabled";
static const QString IsKeepLightsOnAfterExit = "IsKeepLightsOnAfterExit";
static const QString IsKeepLightsOnAfterLock = "IsKeepLightsOnAfterLock";
static const QString IsKeepLightsOnAfterSuspend = "IsKeepLightsOnAfterSuspend";
static const QString IsPingDeviceEverySecond = "IsPingDeviceEverySecond";
static const QString IsUpdateFirmwareMessageShown = "IsUpdateFirmwareMessageShown";
static const QString ConnectedDevice = "ConnectedDevice";
static const QString SupportedDevices = "SupportedDevices";
static const QString CheckForUpdates = "CheckForUpdates";
static const QString InstallUpdates = "InstallForUpdates";
static const QString AutoUpdatingVersion = "AutoUpdatingVersion";

// [Hotkeys]
namespace Hotkeys
{
static const QString SettingsPrefix = "HotKeys/";
}

// [API]
namespace Api
{
static const QString IsEnabled = "API/IsEnabled";
static const QString ListenOnlyOnLoInterface = "API/ListenOnlyOnLoInterface";
static const QString Port = "API/Port";
static const QString AuthKey = "API/AuthKey";
}
namespace Adalight
{
static const QString NumberOfLeds = "Adalight/NumberOfLeds";
static const QString ColorSequence = "Adalight/ColorSequence";
static const QString Port = "Adalight/SerialPort";
static const QString BaudRate = "Adalight/BaudRate";
}
namespace Ardulight
{
static const QString NumberOfLeds = "Ardulight/NumberOfLeds";
static const QString ColorSequence = "Ardulight/ColorSequence";
static const QString Port = "Ardulight/SerialPort";
static const QString BaudRate = "Ardulight/BaudRate";
}
namespace AlienFx
{
static const QString NumberOfLeds = "AlienFx/NumberOfLeds";
}
namespace Lightpack
{
static const QString NumberOfLeds = "Lightpack/NumberOfLeds";
}
namespace Virtual
{
static const QString NumberOfLeds = "Virtual/NumberOfLeds";
}
} /*Key*/

namespace Value
{
static const QString MainConfigVersion = MAIN_CONFIG_FILE_VERSION;

namespace ConnectedDevice
{
static const QString LightpackDevice = "Lightpack";
static const QString AlienFxDevice = "AlienFx";
static const QString AdalightDevice = "Adalight";
static const QString ArdulightDevice = "Ardulight";
static const QString VirtualDevice = "Virtual";
}

} /*Value*/
} /*Main*/

namespace Profile
{
namespace Key
{
// [General]
static const QString LightpackMode = "LightpackMode";
static const QString IsBacklightEnabled = "IsBacklightEnabled";
// [Grab]
namespace Grab
{
static const QString Grabber = "Grab/Grabber";
static const QString IsAvgColorsEnabled = "Grab/IsAvgColorsEnabled";
static const QString IsSendDataOnlyIfColorsChanges = "Grab/IsSendDataOnlyIfColorsChanges";
static const QString Slowdown = "Grab/Slowdown";
static const QString LuminosityThreshold = "Grab/LuminosityThreshold";
static const QString OverBrighten = "Grab/OverBrighten";
static const QString IsMinimumLuminosityEnabled = "Grab/IsMinimumLuminosityEnabled";
static const QString IsDx1011GrabberEnabled = "Grab/IsDX1011GrabberEnabled";
static const QString IsDx9GrabbingEnabled = "Grab/IsDX9GrabbingEnabled";
static const QString IsApplyGammaRampEnabled = "Grab/IsApplyGammaRampEnabled";
static const QString IsApplyColorTemperatureEnabled = "Grab/IsApplyColorTemperatureEnabled";
static const QString ColorTemperature = "Grab/ColorTemperature";
static const QString Gamma = "Grab/Gamma";
}
// [MoodLamp]
namespace MoodLamp
{
static const QString IsLiquidMode = "MoodLamp/LiquidMode";
static const QString Color = "MoodLamp/Color";
static const QString Speed = "MoodLamp/Speed";
}
// [SoundVisualizer]
namespace SoundVisualizer
{
static const QString Device = "SoundVisualizer/Device";
static const QString MinColor = "SoundVisualizer/MinColor";
static const QString MaxColor = "SoundVisualizer/MaxColor";
static const QString IsLiquidMode = "SoundVisualizer/LiquidMode";
static const QString LiquidSpeed = "SoundVisualizer/LiquidSpeed";
}
// [Device]
namespace Device
{
static const QString RefreshDelay = "Device/RefreshDelay";
static const QString IsUsbPowerLedDisabled = "Device/IsUsbPowerLedDisabled";
static const QString Smooth = "Device/Smooth";
static const QString Brightness = "Device/Brightness";
static const QString ColorDepth = "Device/ColorDepth";
static const QString Gamma = "Device/Gamma";
}
// [LED_i]
namespace Led
{

static const QString Prefix = "LED_";
static const QString IsEnabled = "IsEnabled";
static const QString Size = "Size";
static const QString Position = "Position";
static const QString CoefRed = "CoefRed";
static const QString CoefGreen = "CoefGreen";
static const QString CoefBlue = "CoefBlue";
}
} /*Key*/

namespace Value
{

namespace LightpackMode
{
static const QString Ambilight = "Ambilight";
static const QString MoodLamp = "MoodLamp";
static const QString SoundVisualizer = "SoundVisualizer";
}

namespace GrabberType
{
static const QString Qt = "Qt";
static const QString QtEachWidget = "QtEachWidget";
static const QString WinAPI = "WinAPI";
static const QString WinAPIEachWidget = "WinAPIEachWidget";
static const QString X11 = "X11";
static const QString D3D9 = "D3D9";
static const QString MacCoreGraphics = "MacCoreGraphics";
static const QString DDupl = "DDupl";
}

} /*Value*/
} /*Profile*/

QMutex Settings::m_mutex;
QSettings * Settings::m_currentProfile;
QSettings * Settings::m_mainConfig; // LightpackMain.conf contains last profile

Settings * Settings::m_this = new Settings();

// Path to directory there store application generated stuff
QString Settings::m_applicationDirPath = "";

QMap<SupportedDevices::DeviceType, QString> Settings::m_devicesTypeToNameMap;
QMap<SupportedDevices::DeviceType, QString> Settings::m_devicesTypeToKeyNumberOfLedsMap;

Settings::Settings() : QObject(NULL) {
	qRegisterMetaType<Grab::GrabberType>("Grab::GrabberType");
	qRegisterMetaType<QColor>("QColor");
	qRegisterMetaType<SupportedDevices::DeviceType>("SupportedDevices::DeviceType");
	qRegisterMetaType<Lightpack::Mode>("Lightpack::Mode");

}

// Desktop should be initialized before call Settings::Initialize()
bool Settings::Initialize( const QString & applicationDirPath, bool isDebugLevelObtainedFromCmdArgs)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if(m_applicationDirPath != "")
		return true;

	m_applicationDirPath = applicationDirPath;

	// Append to the end of dir path '/'
	if (m_applicationDirPath.lastIndexOf('/') != m_applicationDirPath.length() - 1)
		m_applicationDirPath += "/";

	QString mainConfigPath = getMainConfigPath();
	bool settingsWasPresent = QFileInfo(mainConfigPath).exists();

	m_mainConfig = new QSettings(mainConfigPath, QSettings::IniFormat);
	m_mainConfig->setIniCodec("UTF-8");

	setNewOptionMain(Main::Key::MainConfigVersion,		Main::Value::MainConfigVersion /* rewrite */);
	setNewOptionMain(Main::Key::ProfileLast,			Main::ProfileNameDefault);
	setNewOptionMain(Main::Key::Language,				Main::LanguageDefault);
	setNewOptionMain(Main::Key::DebugLevel,				Main::DebugLevelDefault);
	setNewOptionMain(Main::Key::IsExpertModeEnabled,	Main::IsExpertModeEnabledDefault);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterExit, Main::IsKeepLightsOnAfterExit);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterLock, Main::IsKeepLightsOnAfterLock);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterSuspend, Main::IsKeepLightsOnAfterSuspend);
	setNewOptionMain(Main::Key::IsPingDeviceEverySecond,Main::IsPingDeviceEverySecond);
	setNewOptionMain(Main::Key::IsUpdateFirmwareMessageShown, Main::IsUpdateFirmwareMessageShown);
	setNewOptionMain(Main::Key::ConnectedDevice,		Main::ConnectedDeviceDefault);
	setNewOptionMain(Main::Key::SupportedDevices,		Main::SupportedDevices, true /* always rewrite this information to main config */);
	setNewOptionMain(Main::Key::Api::IsEnabled,			Main::Api::IsEnabledDefault);
	setNewOptionMain(Main::Key::Api::ListenOnlyOnLoInterface, Main::Api::ListenOnlyOnLoInterfaceDefault);
	setNewOptionMain(Main::Key::Api::Port,				Main::Api::PortDefault);
	// Generation AuthKey as new UUID
	setNewOptionMain(Main::Key::Api::AuthKey,			Main::Api::AuthKey);

	// Serial device configuration
	setNewOptionMain(Main::Key::Adalight::Port,				Main::Adalight::PortDefault);
	setNewOptionMain(Main::Key::Adalight::BaudRate,			Main::Adalight::BaudRateDefault);
	setNewOptionMain(Main::Key::Adalight::NumberOfLeds,		Main::Adalight::NumberOfLedsDefault);
	setNewOptionMain(Main::Key::Adalight::ColorSequence,	Main::Adalight::ColorSequence);

	setNewOptionMain(Main::Key::Ardulight::Port,			Main::Ardulight::PortDefault);
	setNewOptionMain(Main::Key::Ardulight::BaudRate,		Main::Ardulight::BaudRateDefault);
	setNewOptionMain(Main::Key::Ardulight::NumberOfLeds,	Main::Ardulight::NumberOfLedsDefault);
	setNewOptionMain(Main::Key::Ardulight::ColorSequence,	Main::Ardulight::ColorSequence);

	setNewOptionMain(Main::Key::AlienFx::NumberOfLeds,		Main::AlienFx::NumberOfLedsDefault);
	setNewOptionMain(Main::Key::Lightpack::NumberOfLeds,	Main::Lightpack::NumberOfLedsDefault);
	setNewOptionMain(Main::Key::Virtual::NumberOfLeds,		Main::Virtual::NumberOfLedsDefault);

	setNewOptionMain(Main::Key::CheckForUpdates,			Main::CheckForUpdates);
	setNewOptionMain(Main::Key::InstallUpdates,				Main::InstallUpdates);

	if (isDebugLevelObtainedFromCmdArgs == false)
	{
		bool ok = false;
		int sDebugLevel = valueMain(Main::Key::DebugLevel).toInt(&ok);

		if (ok && sDebugLevel >= 0)
		{
			g_debugLevel = sDebugLevel;
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "debugLevel =" << g_debugLevel;
		} else {
			qWarning() << "DebugLevel in config has an invalid value, set the default" << Main::DebugLevelDefault;
			setValueMain(Main::Key::DebugLevel, Main::DebugLevelDefault);
			g_debugLevel = Main::DebugLevelDefault;
		}
	}

	QString profileLast = getLastProfileName();

	// Load last profile
	m_currentProfile = new QSettings(getProfilesPath() + profileLast + ".ini", QSettings::IniFormat);
	m_currentProfile->setIniCodec("UTF-8");

	DEBUG_LOW_LEVEL << "Settings file:" << m_currentProfile->fileName();

	// Initialize m_devicesMap for mapping DeviceType on DeviceName
	initDevicesMap();

	// Initialize profile with default values without reset exists values
	initCurrentProfile(false);

	// Do changes to settings if necessary
	migrateSettings();

	return settingsWasPresent;
}

//
//	Set all settings in current config to default values
//
void Settings::resetDefaults()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	initCurrentProfile(true /* = reset to default values */);
}

QStringList Settings::findAllProfiles()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QFileInfo setsFile(Settings::getProfilesPath());
	QFileInfoList iniFiles = setsFile.absoluteDir().entryInfoList(QStringList("*.ini"));

	QStringList settingsFiles;
	for(int i=0; i<iniFiles.count(); i++){
		QString compBaseName = iniFiles.at(i).completeBaseName();
		settingsFiles.append(compBaseName);
	}

	return settingsFiles;
}

void Settings::loadOrCreateProfile(const QString & profileName)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << profileName;


	QMutexLocker locker(&m_mutex);
	QString currentProfileFileName = m_currentProfile != NULL ? m_currentProfile->fileName() : NULL;

	if (currentProfileFileName != NULL
		&& currentProfileFileName.compare(getProfilesPath() + profileName + ".ini") == 0)
		return; //nothing to change, profile is already loaded

	if (currentProfileFileName != NULL)
	{
		// Copy current settings to new one
		QString profileNewPath = getProfilesPath() + profileName + ".ini";

		if (currentProfileFileName != profileNewPath)
			QFile::copy(currentProfileFileName, profileNewPath);

		delete m_currentProfile;
	}


	m_currentProfile = new QSettings(getProfilesPath() + profileName + ".ini", QSettings::IniFormat );
	m_currentProfile->setIniCodec("UTF-8");

	locker.unlock();
	initCurrentProfile(false);
	locker.relock();

	DEBUG_LOW_LEVEL << "Settings file:" << m_currentProfile->fileName();

	m_mainConfig->setValue(Main::Key::ProfileLast, profileName);
	locker.unlock();
}

void Settings::renameCurrentProfile(const QString & profileName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "from" << getCurrentProfileName() << "to" << profileName;

	QMutexLocker locker(&m_mutex);

	if (m_currentProfile == NULL)
	{
		qWarning() << "void Settings::renameCurrentConfig(): fail, m_currentProfile not initialized";
		return;
	}

	// Copy current settings to new one
	QString profilesDir = QFileInfo( m_currentProfile->fileName() ).absoluteDir().absolutePath();
	QString profileNewPath = profilesDir + "/" + profileName + ".ini";

	if (m_currentProfile->fileName() != profileNewPath)
	{
		QFile::rename(m_currentProfile->fileName(), profileNewPath);

		delete m_currentProfile;

		// Update m_currentProfile point to new QSettings with configName
		m_currentProfile = new QSettings(getProfilesPath() + profileName + ".ini", QSettings::IniFormat );
		m_currentProfile->setIniCodec("UTF-8");

		DEBUG_LOW_LEVEL << "Settings file renamed:" << m_currentProfile->fileName();

		m_mainConfig->setValue(Main::Key::ProfileLast, profileName);
	}

	m_currentProfile->sync();
	m_this->currentProfileNameChanged(profileName);
}

void Settings::removeCurrentProfile()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QMutexLocker locker(&m_mutex);

	if (m_currentProfile == NULL)
	{
		qWarning() << Q_FUNC_INFO << "current profile not loaded, nothing to remove";
		return;
	}

	bool result = QFile::remove( m_currentProfile->fileName() );

	if (result == false)
	{
		qWarning() << Q_FUNC_INFO << "QFile::remove(" << m_currentProfile->fileName() << ") fail";
		return;
	}

	delete m_currentProfile;
	m_currentProfile = NULL;

	m_mainConfig->setValue(Main::Key::ProfileLast, Main::ProfileNameDefault);

	m_this->currentProfileRemoved();
}

QString Settings::getCurrentProfileName()
{
	QMutexLocker locker(&m_mutex);

	if (m_currentProfile != NULL) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << m_currentProfile->fileName();
	} else {
		qCritical() << Q_FUNC_INFO << ("current profile isn't set!!!");
	}

	return QFileInfo(m_currentProfile->fileName()).completeBaseName();
}

QString Settings::getCurrentProfilePath()
{
	QMutexLocker locker(&m_mutex);

	if (m_currentProfile != NULL) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << m_currentProfile->fileName();
	} else {
		qCritical() << Q_FUNC_INFO << ("current profile isn't set!!!");
	}

	return m_currentProfile->fileName();
}

bool Settings::isProfileLoaded()
{
	QMutexLocker locker(&m_mutex);
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO;
	return m_currentProfile != NULL;
}

QString Settings::getApplicationDirPath()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << m_applicationDirPath;
	return m_applicationDirPath;
}

QString Settings::getMainConfigPath()
{
	QString mainConfPath = m_applicationDirPath + "main.conf";
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << mainConfPath;
	return mainConfPath;
}

QPoint Settings::getDefaultPosition(int ledIndex)
{
	QPoint result;

	if (ledIndex > (MaximumNumberOfLeds::Default - 1))
	{
		int x = (ledIndex - MaximumNumberOfLeds::Default) * 10 /* px */;
		return QPoint(x, 0);
	}

	QRect screen = QApplication::desktop()->screenGeometry();

	int ledsCountDiv2 = MaximumNumberOfLeds::Default / 2;

	if (ledIndex < ledsCountDiv2)
	{
		result.setX(0);
	} else {
		result.setX(screen.width() - Profile::Led::SizeDefault.width());
	}

	int height = ledsCountDiv2 * Profile::Led::SizeDefault.height();

	int y = screen.height() / 2 - height / 2;

	result.setY(y + (ledIndex % ledsCountDiv2) * Profile::Led::SizeDefault.height());

	return result;
}

QString Settings::getLastProfileName()
{
	return valueMain(Main::Key::ProfileLast).toString();
}

QString Settings::getLanguage()
{
	return valueMain(Main::Key::Language).toString();
}

void Settings::setLanguage(const QString & language)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Language, language);
	m_this->languageChanged(language);
}

int Settings::getDebugLevel()
{
	return valueMain(Main::Key::DebugLevel).toInt();
}

void Settings::setDebugLevel(int debugLvl)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::DebugLevel, debugLvl);
	m_this->debugLevelChanged(debugLvl);
}

bool Settings::isApiEnabled()
{
	return valueMain(Main::Key::Api::IsEnabled).toBool();
}

void Settings::setIsApiEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::IsEnabled, isEnabled);
	m_this->apiServerSettingsChanged();
}

bool Settings::isListenOnlyOnLoInterface()
{
	return valueMain(Main::Key::Api::ListenOnlyOnLoInterface).toBool();
}

void Settings::setListenOnlyOnLoInterface(bool localOnly)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::ListenOnlyOnLoInterface, localOnly);
	m_this->apiServerSettingsChanged();
}

int Settings::getApiPort()
{
	return valueMain(Main::Key::Api::Port).toInt();
}

void Settings::setApiPort(int apiPort)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::Port, apiPort);
	m_this->apiServerSettingsChanged();
}

QString Settings::getApiAuthKey()
{
	QString apikey = valueMain(Main::Key::Api::AuthKey).toString();

	return apikey;
}

void Settings::setApiKey(const QString & apiKey)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::AuthKey, apiKey);
	m_this->apiKeyChanged(apiKey);
}

bool Settings::isExpertModeEnabled()
{	
	return valueMain(Main::Key::IsExpertModeEnabled).toBool();
}

void Settings::setExpertModeEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsExpertModeEnabled, isEnabled);
	m_this->expertModeEnabledChanged(isEnabled);
}

bool Settings::isKeepLightsOnAfterExit()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterExit).toBool();
}

void Settings::setKeepLightsOnAfterExit(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterExit, isEnabled);
	m_this->keepLightsOnAfterExitChanged(isEnabled);
}

bool Settings::isKeepLightsOnAfterLock()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterLock).toBool();
}

void Settings::setKeepLightsOnAfterLock(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterLock, isEnabled);
	m_this->keepLightsOnAfterLockChanged(isEnabled);
}

bool Settings::isKeepLightsOnAfterSuspend()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterSuspend).toBool();
}

void Settings::setKeepLightsOnAfterSuspend(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterSuspend, isEnabled);
	m_this->keepLightsOnAfterSuspendChanged(isEnabled);
}

bool Settings::isPingDeviceEverySecond()
{
	return valueMain(Main::Key::IsPingDeviceEverySecond).toBool();
}

void Settings::setPingDeviceEverySecond(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsPingDeviceEverySecond, isEnabled);
	m_this->pingDeviceEverySecondEnabledChanged(isEnabled);
}

bool Settings::isUpdateFirmwareMessageShown()
{
	return valueMain(Main::Key::IsUpdateFirmwareMessageShown).toBool();
}

void Settings::setUpdateFirmwareMessageShown(bool isShown)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsUpdateFirmwareMessageShown, isShown);
	m_this->updateFirmwareMessageShownChanged(isShown);
}

SupportedDevices::DeviceType Settings::getConnectedDevice()
{
	QString deviceName = valueMain(Main::Key::ConnectedDevice).toString();

	if (m_devicesTypeToNameMap.values().contains(deviceName) == false)
	{
		qWarning() << Q_FUNC_INFO << Main::Key::ConnectedDevice << "in main config contains crap or unsupported device,"
					<< "reset it to default value:" << Main::ConnectedDeviceDefault;

		setConnectedDevice(SupportedDevices::DefaultDeviceType);
		deviceName = Main::ConnectedDeviceDefault;
	}

	return m_devicesTypeToNameMap.key(deviceName, SupportedDevices::DefaultDeviceType);
}

void Settings::setConnectedDevice(SupportedDevices::DeviceType device)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	QString deviceName = m_devicesTypeToNameMap.value(device, Main::ConnectedDeviceDefault);

	setValueMain(Main::Key::ConnectedDevice, deviceName);
	m_this->connectedDeviceChanged(device);
}

QString Settings::getConnectedDeviceName()
{
	return m_devicesTypeToNameMap.value(getConnectedDevice(), Main::ConnectedDeviceDefault);
}

void Settings::setConnectedDeviceName(const QString & deviceName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (deviceName == "")
		return; // silent return

	if (m_devicesTypeToNameMap.values().contains(deviceName) == false)
	{
		qCritical() << Q_FUNC_INFO << "Failure during check the device name" << deviceName << "in m_devicesMap. The main config has not changed.";
		return;
	}

	setValueMain(Main::Key::ConnectedDevice, deviceName);
	m_this->connectedDeviceChanged(m_devicesTypeToNameMap.key(deviceName));
}

QStringList Settings::getSupportedDevices()
{
	return Main::SupportedDevices.split(',');
}

QKeySequence Settings::getHotkey(const QString &actionName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	QString key = Main::Key::Hotkeys::SettingsPrefix + actionName;
	if( !m_mainConfig->contains(key)) {
		return QKeySequence();
	}
	QKeySequence returnSequence = QKeySequence( valueMain(key).toString() );
	return returnSequence;
}

void Settings::setHotkey(const QString &actionName, const QKeySequence &keySequence)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	QString key = Main::Key::Hotkeys::SettingsPrefix + actionName;
	QKeySequence oldKeySequence = getHotkey(actionName);
	if( keySequence.isEmpty() ) {
		setValueMain(key, Main::HotKeys::HotkeyDefault);
	} else {
		setValueMain(key, keySequence.toString());
	}
	m_this->hotkeyChanged(actionName, keySequence, oldKeySequence);
}

QString Settings::getAdalightSerialPortName()
{
	return valueMain(Main::Key::Adalight::Port).toString();
}

void Settings::setAdalightSerialPortName(const QString & port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Adalight::Port, port);
	m_this->adalightSerialPortNameChanged(port);
}

int Settings::getAdalightSerialPortBaudRate()
{
	// TODO: validate baudrate reading from settings file
	return valueMain(Main::Key::Adalight::BaudRate).toInt();
}

void Settings::setAdalightSerialPortBaudRate(const QString & baud)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	// TODO: validator
	setValueMain(Main::Key::Adalight::BaudRate, baud);
	m_this->adalightSerialPortBaudRateChanged(baud);
}

QString Settings::getArdulightSerialPortName()
{
	return valueMain(Main::Key::Ardulight::Port).toString();
}

void Settings::setArdulightSerialPortName(const QString & port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Ardulight::Port, port);
	m_this->ardulightSerialPortNameChanged(port);
}

int Settings::getArdulightSerialPortBaudRate()
{
	// TODO: validate baudrate reading from settings file
	return valueMain(Main::Key::Ardulight::BaudRate).toInt();
}

void Settings::setArdulightSerialPortBaudRate(const QString & baud)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	// TODO: validator
	setValueMain(Main::Key::Ardulight::BaudRate, baud);
	m_this->ardulightSerialPortBaudRateChanged(baud);
}


QStringList Settings::getSupportedSerialPortBaudRates()
{
	QStringList list;

	// TODO: Add more baud rates if need it
	list.append("2000000");
	list.append("1500000");
	list.append("1000000");
	list.append("921600");
	list.append("500000");
	list.append("460800");
	list.append("256000");
	list.append("230400");
	list.append("153600");
	list.append("128000");

	list.append("115200");
	list.append("57600");
	list.append("9600");
	return list;
}

bool Settings::isConnectedDeviceUsesSerialPort()
{
	switch (getConnectedDevice())
	{
	case SupportedDevices::DeviceTypeAdalight:
		return true;
	case SupportedDevices::DeviceTypeArdulight:
		return true;
	default:
		return false;
	}
}

void Settings::setNumberOfLeds(SupportedDevices::DeviceType device, int numberOfLeds)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if(getNumberOfLeds(device) == numberOfLeds)
		//nothing to do
		return;

	QString key = m_devicesTypeToKeyNumberOfLedsMap.value(device);

	if (key == "")
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "numberOfLeds ==" << numberOfLeds;
		return;
	}

	setValueMain(key, numberOfLeds);
	{
		using namespace SupportedDevices;
		switch(device) {
			case DeviceTypeLightpack:
			m_this->lightpackNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeAdalight:
			m_this->adalightNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeArdulight:
			m_this->ardulightNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeVirtual:
			m_this->virtualNumberOfLedsChanged(numberOfLeds);
			break;
		default:
			qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "numberOfLeds ==" << numberOfLeds;
		}
	}
}

int Settings::getNumberOfLeds(SupportedDevices::DeviceType device)
{
	QString key = m_devicesTypeToKeyNumberOfLedsMap.value(device);

	if (key == "")
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device;
		return MaximumNumberOfLeds::Default;
	}

	// TODO: validator on maximum number of leds for current 'device'
	return valueMain(key).toInt();
}

void Settings::setColorSequence(SupportedDevices::DeviceType device, QString colorSequence)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << device << colorSequence;
	switch (device)
	{
		case	SupportedDevices::DeviceTypeAdalight:
			setValueMain(Main::Key::Adalight::ColorSequence, colorSequence);
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "ada" << colorSequence;
			break;
		case SupportedDevices::DeviceTypeArdulight:
			setValueMain(Main::Key::Ardulight::ColorSequence, colorSequence);
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "ard" << colorSequence;
			break;
		default:
			qWarning() << Q_FUNC_INFO << "Unsupported device type: " << device << m_devicesTypeToNameMap.value(device);
			return;
	}
	m_this->deviceColorSequenceChanged(colorSequence);
}

QString Settings::getColorSequence(SupportedDevices::DeviceType device)
{
	switch (device)
	{
		case SupportedDevices::DeviceTypeAdalight:
			return valueMain(Main::Key::Adalight::ColorSequence).toString();
			break;
		case SupportedDevices::DeviceTypeArdulight:
			return valueMain(Main::Key::Ardulight::ColorSequence).toString();
			break;
		default:
			qWarning() << Q_FUNC_INFO << "Unsupported device type: " << device << m_devicesTypeToNameMap.value(device);
	}
	return NULL;
}

int Settings::getGrabSlowdown()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return getValidGrabSlowdown(value(Profile::Key::Grab::Slowdown).toInt());
}

void Settings::setGrabSlowdown(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::Slowdown, getValidGrabSlowdown(value));
	m_this->grabSlowdownChanged(value);
}

bool Settings::isBacklightEnabled()
{
	return value(Profile::Key::IsBacklightEnabled).toBool();
}

void Settings::setIsBacklightEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::IsBacklightEnabled, isEnabled);
	m_this->backlightEnabledChanged(isEnabled);
}

bool Settings::isGrabAvgColorsEnabled()
{
	return value(Profile::Key::Grab::IsAvgColorsEnabled).toBool();
}

void Settings::setGrabAvgColorsEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsAvgColorsEnabled, isEnabled);
	m_this->grabAvgColorsEnabledChanged(isEnabled);
}

int Settings::getGrabOverBrighten()
{
	return getValidGrabOverBrighten(value(Profile::Key::Grab::OverBrighten).toInt());
}

void Settings::setGrabOverBrighten(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::OverBrighten, getValidGrabOverBrighten(value));
	m_this->grabOverBrightenChanged(value);
}

bool Settings::isGrabApplyGammaRampEnabled()
{
	return value(Profile::Key::Grab::IsApplyGammaRampEnabled).toBool();
}

void Settings::setGrabApplyGammaRampEnabled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsApplyGammaRampEnabled, value);
	m_this->grabApplyGammaRampChanged(value);
}

bool Settings::isGrabApplyColorTemperatureEnabled()
{
	return value(Profile::Key::Grab::IsApplyColorTemperatureEnabled).toBool();
}
void Settings::setGrabApplyColorTemperatureEnabled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsApplyColorTemperatureEnabled, value);
	m_this->grabApplyColorTemperatureChanged(value);
}
int Settings::getGrabColorTemperature()
{
	return value(Profile::Key::Grab::ColorTemperature).toInt();
}
void Settings::setGrabColorTemperature(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::ColorTemperature, value);
	m_this->grabColorTemperatureChanged(value);
}
double Settings::getGrabGamma()
{
	return value(Profile::Key::Grab::Gamma).toDouble();
}
void Settings::setGrabGamma(double gamma)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::Gamma, gamma);
	m_this->grabGammaChanged(gamma);
}

bool Settings::isSendDataOnlyIfColorsChanges()
{
	return value(Profile::Key::Grab::IsSendDataOnlyIfColorsChanges).toBool();
}

void Settings::setSendDataOnlyIfColorsChanges(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsSendDataOnlyIfColorsChanges, isEnabled);
	m_this->sendDataOnlyIfColorsChangesChanged(isEnabled);
}

int Settings::getLuminosityThreshold()
{
	return getValidLuminosityThreshold(value(Profile::Key::Grab::LuminosityThreshold).toInt());
}

void Settings::setLuminosityThreshold(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::LuminosityThreshold, getValidLuminosityThreshold(value));
	m_this->luminosityThresholdChanged(value);
}

bool Settings::isMinimumLuminosityEnabled()
{
	return value(Profile::Key::Grab::IsMinimumLuminosityEnabled).toBool();
}

void Settings::setMinimumLuminosityEnabled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsMinimumLuminosityEnabled, value);
	m_this->minimumLuminosityEnabledChanged(value);
}

int Settings::getDeviceRefreshDelay()
{
	return getValidDeviceRefreshDelay(value(Profile::Key::Device::RefreshDelay).toInt());
}

void Settings::setDeviceRefreshDelay(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::RefreshDelay, getValidDeviceRefreshDelay(value));
	m_this->deviceRefreshDelayChanged(value);
}

bool Settings::isDeviceUsbPowerLedDisabled() {
	return value(Profile::Key::Device::IsUsbPowerLedDisabled).toBool();
}

void Settings::setDeviceUsbPowerLedDisabled(bool isDisabled) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::IsUsbPowerLedDisabled, isDisabled);
	m_this->deviceUsbPowerLedDisabledChanged(isDisabled);
}

int Settings::getDeviceBrightness()
{
	return getValidDeviceBrightness(value(Profile::Key::Device::Brightness).toInt());
}

void Settings::setDeviceBrightness(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::Brightness, getValidDeviceBrightness(value));
	m_this->deviceBrightnessChanged(value);
}

int Settings::getDeviceSmooth()
{
	return getValidDeviceSmooth(value(Profile::Key::Device::Smooth).toInt());
}

void Settings::setDeviceSmooth(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::Smooth, getValidDeviceSmooth(value));
	m_this->deviceSmoothChanged(value);
}

int Settings::getDeviceColorDepth()
{
	return getValidDeviceColorDepth(value(Profile::Key::Device::ColorDepth).toInt());
}

void Settings::setDeviceColorDepth(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::ColorDepth, getValidDeviceColorDepth(value));
	m_this->deviceColorDepthChanged(value);
}

double Settings::getDeviceGamma()
{
	return getValidDeviceGamma(value(Profile::Key::Device::Gamma).toDouble());
}

void Settings::setDeviceGamma(double gamma)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::Gamma, getValidDeviceGamma(gamma));
	m_this->deviceGammaChanged(gamma);
}

Grab::GrabberType Settings::getGrabberType()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString strGrabber = value(Profile::Key::Grab::Grabber).toString();

#ifdef QT_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::Qt)
		return Grab::GrabberTypeQt;
	if (strGrabber == Profile::Value::GrabberType::QtEachWidget)
		return Grab::GrabberTypeQtEachWidget;
#endif

#ifdef WINAPI_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::WinAPI)
		return Grab::GrabberTypeWinAPI;
#endif
#ifdef WINAPI_EACH_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::WinAPIEachWidget)
		return Grab::GrabberTypeWinAPIEachWidget;
#endif

#ifdef DDUPL_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::DDupl)
		return Grab::GrabberTypeDDupl;
#endif

#ifdef D3D9_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::D3D9)
		return Grab::GrabberTypeD3D9;
#endif

#ifdef X11_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::X11)
		return Grab::GrabberTypeX11;
#endif

#ifdef MAC_OS_CG_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::MacCoreGraphics)
		return Grab::GrabberTypeMacCoreGraphics;
#endif

	qWarning() << Q_FUNC_INFO << Profile::Key::Grab::Grabber << "contains invalid value:" << strGrabber << ", reset it to default:" << Profile::Grab::GrabberDefaultString;
	setGrabberType(Profile::Grab::GrabberDefault);

	return Profile::Grab::GrabberDefault;
}

void Settings::setGrabberType(Grab::GrabberType grabberType)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << grabberType;

	QString strGrabber;
	switch (grabberType)
	{
	case Grab::GrabberTypeQt:
		strGrabber = Profile::Value::GrabberType::Qt;
		break;

	case Grab::GrabberTypeQtEachWidget:
		strGrabber = Profile::Value::GrabberType::QtEachWidget;
		break;

#ifdef WINAPI_GRAB_SUPPORT
	case Grab::GrabberTypeWinAPI:
		strGrabber = Profile::Value::GrabberType::WinAPI;
		break;
	case Grab::GrabberTypeWinAPIEachWidget:
		strGrabber = Profile::Value::GrabberType::WinAPIEachWidget;
		break;
#endif

#ifdef DDUPL_GRAB_SUPPORT
	case Grab::GrabberTypeDDupl:
		strGrabber = Profile::Value::GrabberType::DDupl;
		break;

#endif

#ifdef D3D9_GRAB_SUPPORT
	case Grab::GrabberTypeD3D9:
		strGrabber = Profile::Value::GrabberType::D3D9;
		break;
#endif

#ifdef X11_GRAB_SUPPORT
	case Grab::GrabberTypeX11:
		strGrabber = Profile::Value::GrabberType::X11;
		break;
#endif

#ifdef MAC_OS_CG_GRAB_SUPPORT
	case Grab::GrabberTypeMacCoreGraphics:
		strGrabber = Profile::Value::GrabberType::MacCoreGraphics;
		break;
#endif

	default:
		qWarning() << Q_FUNC_INFO << "Switch on grabberType =" << grabberType << "failed. Reset to default value.";
		strGrabber = Profile::Grab::GrabberDefaultString;
	}
	setValue(Profile::Key::Grab::Grabber, strGrabber);
	m_this->grabberTypeChanged(grabberType);
}

#ifdef D3D10_GRAB_SUPPORT
bool Settings::isDx1011GrabberEnabled() {
	return value(Profile::Key::Grab::IsDx1011GrabberEnabled).toBool();
}

void Settings::setDx1011GrabberEnabled(bool isEnabled) {
	setValue(Profile::Key::Grab::IsDx1011GrabberEnabled, isEnabled);
	m_this->dx1011GrabberEnabledChanged(isEnabled);
}

bool Settings::isDx9GrabbingEnabled() {
	return value(Profile::Key::Grab::IsDx9GrabbingEnabled).toBool();
}

void Settings::setDx9GrabbingEnabled(bool isEnabled) {
	setValue(Profile::Key::Grab::IsDx9GrabbingEnabled, isEnabled);
	m_this->dx9GrabberEnabledChanged(isEnabled);
}
#endif

Lightpack::Mode Settings::getLightpackMode()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString strMode = value(Profile::Key::LightpackMode).toString();

	if (strMode == Profile::Value::LightpackMode::Ambilight)
	{
		return Lightpack::AmbilightMode;
	}
	else if (strMode == Profile::Value::LightpackMode::MoodLamp)
	{
		return Lightpack::MoodLampMode;
	}
#ifdef BASS_SOUND_SUPPORT
	else if (strMode == Profile::Value::LightpackMode::SoundVisualizer)
	{
		return Lightpack::SoundVisualizeMode;
	}
#endif
	else
	{
		qWarning() << Q_FUNC_INFO << "Read fail. Reset to default value = " << Lightpack::Default;

		setLightpackMode(Lightpack::Default);
		return Lightpack::Default;
	}
}

void Settings::setLightpackMode(Lightpack::Mode mode)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << mode;

	if (mode == Lightpack::AmbilightMode)
	{
		setValue(Profile::Key::LightpackMode, Profile::Value::LightpackMode::Ambilight);
		m_this->lightpackModeChanged(mode);
	}
	else if (mode == Lightpack::MoodLampMode)
	{
		setValue(Profile::Key::LightpackMode, Profile::Value::LightpackMode::MoodLamp);
		m_this->lightpackModeChanged(mode);
	}
#ifdef BASS_SOUND_SUPPORT
	else if (mode == Lightpack::SoundVisualizeMode)
	{
		setValue(Profile::Key::LightpackMode, Profile::Value::LightpackMode::SoundVisualizer);
		m_this->lightpackModeChanged(mode);
	}
#endif
	else
	{
		qCritical() << Q_FUNC_INFO << "Invalid value =" << mode;
	}
}

bool Settings::isMoodLampLiquidMode()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return value(Profile::Key::MoodLamp::IsLiquidMode).toBool();
}

void Settings::setMoodLampLiquidMode(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::MoodLamp::IsLiquidMode, value );
	m_this->moodLampLiquidModeChanged(value);
}

QColor Settings::getMoodLampColor()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return QColor(value(Profile::Key::MoodLamp::Color).toString());
}

void Settings::setMoodLampColor(QColor value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value.name();
	setValue(Profile::Key::MoodLamp::Color, value.name() );
	m_this->moodLampColorChanged(value);
}

int Settings::getMoodLampSpeed()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return getValidMoodLampSpeed(value(Profile::Key::MoodLamp::Speed).toInt());
}

void Settings::setMoodLampSpeed(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::MoodLamp::Speed, getValidMoodLampSpeed(value));
	m_this->moodLampSpeedChanged(value);
}

#ifdef BASS_SOUND_SUPPORT
int Settings::getSoundVisualizerDevice()
{
	return value(Profile::Key::SoundVisualizer::Device).toInt();
}

void Settings::setSoundVisualizerDevice(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	setValue(Profile::Key::SoundVisualizer::Device, value);
	m_this->soundVisualizerDeviceChanged(value);
}

QColor Settings::getSoundVisualizerMinColor()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return QColor(value(Profile::Key::SoundVisualizer::MinColor).toString());
}

void Settings::setSoundVisualizerMinColor(QColor value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value.name();
	setValue(Profile::Key::SoundVisualizer::MinColor, value.name());
	m_this->soundVisualizerMinColorChanged(value);
}

QColor Settings::getSoundVisualizerMaxColor()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return QColor(value(Profile::Key::SoundVisualizer::MaxColor).toString());
}

void Settings::setSoundVisualizerMaxColor(QColor value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value.name();
	setValue(Profile::Key::SoundVisualizer::MaxColor, value.name());
	m_this->soundVisualizerMaxColorChanged(value);
}

bool Settings::isSoundVisualizerLiquidMode()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return value(Profile::Key::SoundVisualizer::IsLiquidMode).toBool();
}

void Settings::setSoundVisualizerLiquidMode(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::SoundVisualizer::IsLiquidMode, value);
	m_this->soundVisualizerLiquidModeChanged(value);
}

int Settings::getSoundVisualizerLiquidSpeed()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return getValidSoundVisualizerLiquidSpeed(value(Profile::Key::SoundVisualizer::LiquidSpeed).toInt());
}

void Settings::setSoundVisualizerLiquidSpeed(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::SoundVisualizer::LiquidSpeed, getValidSoundVisualizerLiquidSpeed(value));
	m_this->soundVisualizerLiquidSpeedChanged(value);
}
#endif

QList<WBAdjustment> Settings::getLedCoefs()
{
	QList<WBAdjustment> result;
	const int numOfLeds = getNumberOfLeds(getConnectedDevice());

	for (int led = 0; led < numOfLeds; ++led)
		result.append(getLedAdjustment(led));

	return result;
}

double Settings::getLedCoefRed(int ledIndex)
{
	return getValidLedCoef(ledIndex, Profile::Key::Led::CoefRed);
}

double Settings::getLedCoefGreen(int ledIndex)
{
	return getValidLedCoef(ledIndex, Profile::Key::Led::CoefGreen);
}

double Settings::getLedCoefBlue(int ledIndex)
{
	return getValidLedCoef(ledIndex, Profile::Key::Led::CoefBlue);
}

void Settings::setLedCoefRed(int ledIndex, double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValidLedCoef(ledIndex, Profile::Key::Led::CoefRed, value);
	m_this->ledCoefRedChanged(ledIndex, value);
}

void Settings::setLedCoefGreen(int ledIndex, double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValidLedCoef(ledIndex, Profile::Key::Led::CoefGreen, value);
	m_this->ledCoefGreenChanged(ledIndex, value);
}

void Settings::setLedCoefBlue(int ledIndex, double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValidLedCoef(ledIndex, Profile::Key::Led::CoefBlue, value);
	m_this->ledCoefBlueChanged(ledIndex, value);
}

QSize Settings::getLedSize(int ledIndex)
{
	return value(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + Profile::Key::Led::Size).toSize();
}

void Settings::setLedSize(int ledIndex, QSize size)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + Profile::Key::Led::Size, size);
	m_this->ledSizeChanged(ledIndex, size);
}

QPoint Settings::getLedPosition(int ledIndex)
{
	return value(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + Profile::Key::Led::Position).toPoint();
}

void Settings::setLedPosition(int ledIndex, QPoint position)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + Profile::Key::Led::Position, position);
	m_this->ledPositionChanged(ledIndex, position);
}

bool Settings::isLedEnabled(int ledIndex)
{
	QVariant result = value(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + Profile::Key::Led::IsEnabled);
	if (result.isNull())
		return Profile::Led::IsEnabledDefault;
	else
		return result.toBool();
}

void Settings::setLedEnabled(int ledIndex, bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + Profile::Key::Led::IsEnabled, isEnabled);
	m_this->ledEnabledChanged(ledIndex, isEnabled);
}

int Settings::getValidDeviceRefreshDelay(int value)
{
	if (value < Profile::Device::RefreshDelayMin)
		value = Profile::Device::RefreshDelayMin;
	else if (value > Profile::Device::RefreshDelayMax)
		value = Profile::Device::RefreshDelayMax;
	return value;
}

int Settings::getValidDeviceBrightness(int value)
{
	if (value < Profile::Device::BrightnessMin)
		value = Profile::Device::BrightnessMin;
	else if (value > Profile::Device::BrightnessMax)
		value = Profile::Device::BrightnessMax;
	return value;
}

int Settings::getValidDeviceSmooth(int value)
{
	if (value < Profile::Device::SmoothMin)
		value = Profile::Device::SmoothMin;
	else if (value > Profile::Device::SmoothMax)
		value = Profile::Device::SmoothMax;
	return value;
}

int Settings::getValidDeviceColorDepth(int value)
{
	if (value < Profile::Device::ColorDepthMin)
		value = Profile::Device::ColorDepthMin;
	else if (value > Profile::Device::ColorDepthMax)
		value = Profile::Device::ColorDepthMax;
	return value;
}

double Settings::getValidDeviceGamma(double value)
{
	if (value < Profile::Device::GammaMin)
		value = Profile::Device::GammaMin;
	else if (value > Profile::Device::GammaMax)
		value = Profile::Device::GammaMax;
	return value;
}

int Settings::getValidGrabSlowdown(int value)
{
	if (value < Profile::Grab::SlowdownMin)
		value = Profile::Grab::SlowdownMin;
	else if (value > Profile::Grab::SlowdownMax)
		value = Profile::Grab::SlowdownMax;
	return value;
}

int Settings::getValidMoodLampSpeed(int value)
{
	if (value < Profile::MoodLamp::SpeedMin)
		value = Profile::MoodLamp::SpeedMin;
	else if (value > Profile::MoodLamp::SpeedMax)
		value = Profile::MoodLamp::SpeedMax;
	return value;
}

int Settings::getValidSoundVisualizerLiquidSpeed(int value)
{
	if (value < Profile::SoundVisualizer::LiquidSpeedMin)
		value = Profile::SoundVisualizer::LiquidSpeedMin;
	else if (value > Profile::SoundVisualizer::LiquidSpeedMax)
		value = Profile::SoundVisualizer::LiquidSpeedMax;
	return value;
}

int Settings::getValidLuminosityThreshold(int value)
{
	if (value < Profile::Grab::LuminosityThresholdMin)
		value = Profile::Grab::LuminosityThresholdMin;
	else if (value > Profile::Grab::LuminosityThresholdMax)
		value = Profile::Grab::LuminosityThresholdMax;
	return value;
}

int Settings::getValidGrabOverBrighten(int value)
{
	if (value < Profile::Grab::OverBrightenMin)
		value = Profile::Grab::OverBrightenMin;
	else if (value > Profile::Grab::OverBrightenMax)
		value = Profile::Grab::OverBrightenMax;
	return value;
}

void Settings::setValidLedCoef(int ledIndex, const QString & keyCoef, double coef)
{
	if (coef < Profile::Led::CoefMin || coef > Profile::Led::CoefMax){
		QString error = "Error: outside the valid values (coef < " +
				QString::number(Profile::Led::CoefMin) + " || coef > " + QString::number(Profile::Led::CoefMax) + ").";

		qWarning() << Q_FUNC_INFO << "Settings bad value"
					<< "[" + Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "]"
					<< keyCoef
					<< error
					<< "Convert to double error. Set it to default value" << keyCoef << "=" << Profile::Led::CoefDefault;
		coef = Profile::Led::CoefDefault;		
	}
	Settings::setValue(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + keyCoef, coef);
}

double Settings::getValidLedCoef(int ledIndex, const QString & keyCoef)
{
	bool ok = false;
	double coef = Settings::value(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + keyCoef).toDouble(&ok);
	QString error;
	if (ok == false){
		error = "Error: Convert to double.";
	} else if (coef < Profile::Led::CoefMin || coef > Profile::Led::CoefMax){
		QString error = "Error: outside the valid values (coef < " +
				QString::number(Profile::Led::CoefMin) + " || coef > " + QString::number(Profile::Led::CoefMax) + ").";
	} else {
		// OK
		return coef;
	}
	// Have an error
	qWarning() << Q_FUNC_INFO << "Settings bad value"
				<< "[" + Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "]"
				<< keyCoef
				<< error
				<< "Set it to default value" << keyCoef << "=" << Profile::Led::CoefDefault;
	coef = Profile::Led::CoefDefault;
	Settings::setValue(Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "/" + keyCoef, coef);
	return coef;
}

QString Settings::getProfilesPath() {
	return m_applicationDirPath + "Profiles/";
}

bool Settings::isCheckForUpdatesEnabled() {
	return valueMain(Main::Key::CheckForUpdates).toBool();

}

void Settings::setCheckForUpdatesEnabled(bool isEnabled) {
	setValueMain(Main::Key::CheckForUpdates, isEnabled);
}

bool Settings::isInstallUpdatesEnabled() {
	return valueMain(Main::Key::InstallUpdates).toBool();

}

void Settings::setInstallUpdatesEnabled(bool isEnabled) {
	setValueMain(Main::Key::InstallUpdates, isEnabled);
}

QString Settings::getAutoUpdatingVersion() {
	return valueMain(Main::Key::AutoUpdatingVersion).toString();
}

void Settings::setAutoUpdatingVersion(const QString & version) {
	setValueMain(Main::Key::AutoUpdatingVersion, version);
}

//
//	Check and/or initialize settings
//
void Settings::initCurrentProfile(bool isResetDefault)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << isResetDefault;

	// [General]
	setNewOption(Profile::Key::LightpackMode,						Profile::LightpackModeDefault, isResetDefault);
	setNewOption(Profile::Key::IsBacklightEnabled,					Profile::IsBacklightEnabledDefault, isResetDefault);
	// [Grab]
	setNewOption(Profile::Key::Grab::Grabber,						Profile::Grab::GrabberDefaultString, isResetDefault);
	setNewOption(Profile::Key::Grab::IsAvgColorsEnabled,			Profile::Grab::IsAvgColorsEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::OverBrighten,					Profile::Grab::OverBrightenDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsSendDataOnlyIfColorsChanges, Profile::Grab::IsSendDataOnlyIfColorsChangesDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::Slowdown,						Profile::Grab::SlowdownDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::LuminosityThreshold,			Profile::Grab::LuminosityThresholdDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsMinimumLuminosityEnabled,	Profile::Grab::IsMinimumLuminosityEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsDx1011GrabberEnabled,		Profile::Grab::IsDx1011GrabberEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsDx9GrabbingEnabled,			Profile::Grab::IsDx9GrabbingEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsApplyGammaRampEnabled,		Profile::Grab::IsApplyGammaRampEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsApplyColorTemperatureEnabled,Profile::Grab::IsApplyColorTemperatureEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::ColorTemperature,              Profile::Grab::ColorTemperatureDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::Gamma,                         Profile::Grab::GammaDefault, isResetDefault);
	// [MoodLamp]
	setNewOption(Profile::Key::MoodLamp::IsLiquidMode,				Profile::MoodLamp::IsLiquidModeDefault, isResetDefault);
	setNewOption(Profile::Key::MoodLamp::Color,						Profile::MoodLamp::ColorDefault, isResetDefault);
	setNewOption(Profile::Key::MoodLamp::Speed,						Profile::MoodLamp::SpeedDefault, isResetDefault);
#ifdef BASS_SOUND_SUPPORT
	// [SoundVisualizer]
	setNewOption(Profile::Key::SoundVisualizer::Device,				Profile::SoundVisualizer::DeviceDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::MinColor,			Profile::SoundVisualizer::MinColorDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::MaxColor,			Profile::SoundVisualizer::MaxColorDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::IsLiquidMode,		Profile::SoundVisualizer::IsLiquidModeDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::LiquidSpeed,		Profile::SoundVisualizer::LiquidSpeedDefault, isResetDefault);
#endif
	// [Device]
	setNewOption(Profile::Key::Device::RefreshDelay,				Profile::Device::RefreshDelayDefault, isResetDefault);
	setNewOption(Profile::Key::Device::IsUsbPowerLedDisabled,		Profile::Device::IsUsbPowerLedDisabled, isResetDefault);
	setNewOption(Profile::Key::Device::Brightness,					Profile::Device::BrightnessDefault, isResetDefault);
	setNewOption(Profile::Key::Device::Smooth,						Profile::Device::SmoothDefault, isResetDefault);
	setNewOption(Profile::Key::Device::Gamma,						Profile::Device::GammaDefault, isResetDefault);
	setNewOption(Profile::Key::Device::ColorDepth,					Profile::Device::ColorDepthDefault, isResetDefault);


	QPoint ledPosition;

	for (int i = 0; i < MaximumNumberOfLeds::AbsoluteMaximum; i++)
	{
		ledPosition = getDefaultPosition(i);


		setNewOption(Profile::Key::Led::Prefix + QString::number(i + 1) + "/" + Profile::Key::Led::IsEnabled,
						Profile::Led::IsEnabledDefault, isResetDefault);
		setNewOption(Profile::Key::Led::Prefix + QString::number(i + 1) + "/" + Profile::Key::Led::Position,
						ledPosition, isResetDefault);
		setNewOption(Profile::Key::Led::Prefix + QString::number(i + 1) + "/" + Profile::Key::Led::Size,
						Profile::Led::SizeDefault, isResetDefault);

		setNewOption(Profile::Key::Led::Prefix + QString::number(i + 1) + "/" + Profile::Key::Led::CoefRed,
						Profile::Led::CoefDefault, isResetDefault);
		setNewOption(Profile::Key::Led::Prefix + QString::number(i + 1) + "/" + Profile::Key::Led::CoefGreen,
						Profile::Led::CoefDefault, isResetDefault);
		setNewOption(Profile::Key::Led::Prefix + QString::number(i + 1) + "/" + Profile::Key::Led::CoefBlue,
						Profile::Led::CoefDefault, isResetDefault);
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "led";
	QMutexLocker locker(&m_mutex);
	m_currentProfile->sync();
	locker.unlock();
	m_this->currentProfileInited(getCurrentProfileName());
}


void Settings::setNewOption(const QString & name, const QVariant & value,
										bool isForceSetOption, QSettings * settings /*= m_currentProfile*/)
{
	QMutexLocker locker(&m_mutex);

	if (isForceSetOption)
	{
		settings->setValue(name, value);
	} else {
		if (settings->contains(name) == false)
		{
			DEBUG_LOW_LEVEL << "Settings:"<< name << "not found."
					<< "Set it to default value: " << value.toString();

			settings->setValue(name, value);
		}
		// else option exists do nothing
	}
}

void Settings::setNewOptionMain(const QString & name, const QVariant & value, bool isForceSetOption /*= false*/)
{
	setNewOption(name, value, isForceSetOption, m_mainConfig);
}

void Settings::setValueMain(const QString & key, const QVariant & value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << key << value;

	QMutexLocker locker(&m_mutex);

	if (m_mainConfig == NULL)
	{
		qWarning() << Q_FUNC_INFO << "m_mainConfig == NULL";
		return;
	}
	m_mainConfig->setValue(key, value);
}

QVariant Settings::valueMain(const QString & key)
{
	QVariant value;
	{
		QMutexLocker locker(&m_mutex);

		if (m_mainConfig == NULL) {
			qWarning() << Q_FUNC_INFO << "m_mainConfig == NULL";
			return QVariant();
		}

		value = m_mainConfig->value(key);
	}
	DEBUG_MID_LEVEL << Q_FUNC_INFO << key << "= " << value;
	return value;
}

void Settings::setValue(const QString & key, const QVariant & value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << key << "= " << value;

	QMutexLocker locker(&m_mutex);

	if (m_currentProfile == NULL)
	{
		qWarning() << Q_FUNC_INFO << "m_currentProfile == NULL";
		return;
	}
	m_currentProfile->setValue(key, value);
}

QVariant Settings::value(const QString & key)
{
	QVariant value;
	{
		QMutexLocker locker(&m_mutex);

		if (m_currentProfile == NULL) {
			qWarning() << Q_FUNC_INFO << "m_currentProfile == NULL";
			return QVariant();
		}

		value = m_currentProfile->value(key);
	}
	DEBUG_MID_LEVEL << Q_FUNC_INFO << key << "= " << value;
	return value;
}

void Settings::initDevicesMap()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeAdalight]	= Main::Value::ConnectedDevice::AdalightDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeArdulight] = Main::Value::ConnectedDevice::ArdulightDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeLightpack] = Main::Value::ConnectedDevice::LightpackDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeVirtual]	= Main::Value::ConnectedDevice::VirtualDevice;

	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeAdalight]	= Main::Key::Adalight::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeArdulight] = Main::Key::Ardulight::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeLightpack] = Main::Key::Lightpack::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeVirtual]	= Main::Key::Virtual::NumberOfLeds;

#ifdef ALIEN_FX_SUPPORTED
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeAlienFx]	= Main::Value::ConnectedDevice::AlienFxDevice;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeAlienFx]	= Main::Key::AlienFx::NumberOfLeds;
#endif
}

/*
 * --------------------------- Migration --------------------------------
 */


void Settings::migrateSettings()
{

	if (valueMain(Main::Key::MainConfigVersion).toString() == "1.0") {

		if (getConnectedDevice() == SupportedDevices::DeviceTypeLightpack){

			int remap[] = {3, 4, 2, 1, 0, 5, 6, 7, 8, 9};

			int ledCount = getNumberOfLeds(SupportedDevices::DeviceTypeLightpack);
			QMap<int, LedInfo> ledInfoMap;
			for (int i = 0; i < ledCount; i++){
				LedInfo ledInfo;
				ledInfo.isEnabled = isLedEnabled(i);
				ledInfo.position = getLedPosition(i);
				ledInfo.size = getLedSize(i);
				ledInfo.wbRed = getLedCoefRed(i);
				ledInfo.wbGreen = getLedCoefGreen(i);
				ledInfo.wbBlue = getLedCoefBlue(i);
				ledInfoMap.insert(remap[i], ledInfo);
			}
			QList<LedInfo> remappedLeds = ledInfoMap.values();
			for (int i = 0; i < remappedLeds.size(); ++i) {
				Settings::setLedEnabled(i, remappedLeds[i].isEnabled);
				Settings::setLedPosition(i, remappedLeds[i].position);
				Settings::setLedSize(i, remappedLeds[i].size);
				Settings::setLedCoefRed(i, remappedLeds[i].wbRed);
				Settings::setLedCoefGreen(i, remappedLeds[i].wbGreen);
				Settings::setLedCoefBlue(i, remappedLeds[i].wbBlue);
			}
		}
		setValueMain(Main::Key::MainConfigVersion, "2.0");
	}
	if (valueMain(Main::Key::MainConfigVersion).toString() == "2.0") {
		// Disable ApiAuth by default
		setApiKey("");

		// Remove obsolete keys
		QString authEnabledKey = "API/IsAuthEnabled";
		if(m_mainConfig->contains(authEnabledKey)) {
			m_mainConfig->remove(authEnabledKey);
		}

		setValueMain(Main::Key::MainConfigVersion, "3.0");
	}
	if (valueMain(Main::Key::MainConfigVersion).toString() == "3.0") {
#ifdef WINAPI_GRAB_SUPPORT
		Settings::setGrabberType(Grab::GrabberTypeWinAPI);
#endif

#ifdef X11_GRAB_SUPPORT
		Settings::setGrabberType(Grab::GrabberTypeX11);
#endif

#ifdef MAC_OS_CG_GRAB_SUPPORT
		Settings::setGrabberType(Grab::GrabberTypeMacCoreGraphics);
#endif

		setValueMain(Main::Key::MainConfigVersion, "4.0");
	}
}
} /*SettingsScope*/

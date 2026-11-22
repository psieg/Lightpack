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
#include <QKeySequence>
#include <QSize>
#include <QPoint>
#include <QFileInfo>
#include <QDir>
#include <QUuid>
#include <QScreen>
#include <QGuiApplication>
#ifdef Q_OS_WIN
#if !defined NOMINMAX
#define NOMINMAX
#endif
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include "debug.h"

#define MAIN_CONFIG_FILE_VERSION	"4.0"

namespace
{
bool isPathReparseOrSymlink(const QString &rawPath)
{
	if (rawPath.isEmpty())
		return false;
	QFileInfo fi(rawPath);
	if (fi.isSymLink())
		return true;
#ifdef Q_OS_WIN
	QString nativePath = QDir::toNativeSeparators(QDir::cleanPath(rawPath));
	while (nativePath.length() > 3 && nativePath.endsWith(QLatin1Char('\\')))
		nativePath.chop(1);
	const DWORD attrs = GetFileAttributesW(reinterpret_cast<LPCWSTR>(nativePath.utf16()));
	if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
		return true;
#endif
	return false;
}

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
static const QString MainConfigVersion = QStringLiteral("MainConfigVersion");
static const QString ProfileLast = QStringLiteral("ProfileLast");
static const QString Language = QStringLiteral("Language");
static const QString DebugLevel = QStringLiteral("DebugLevel");
static const QString IsKeepLightsOnAfterExit = QStringLiteral("IsKeepLightsOnAfterExit");
static const QString IsKeepLightsOnAfterLock = QStringLiteral("IsKeepLightsOnAfterLock");
static const QString IsKeepLightsOnAfterSuspend = QStringLiteral("IsKeepLightsOnAfterSuspend");
static const QString IsKeepLightsOnAfterScreenOff = QStringLiteral("IsKeepLightsOnAfterScreenOff");
static const QString IsPingDeviceEverySecond = QStringLiteral("IsPingDeviceEverySecond");
static const QString IsUpdateFirmwareMessageShown = QStringLiteral("IsUpdateFirmwareMessageShown");
static const QString ConnectedDevice = QStringLiteral("ConnectedDevice");
static const QString SupportedDevices = QStringLiteral("SupportedDevices");
static const QString CheckForUpdates = QStringLiteral("CheckForUpdates");
static const QString InstallUpdates = QStringLiteral("InstallForUpdates");
static const QString AutoUpdatingVersion = QStringLiteral("AutoUpdatingVersion");

// [Hotkeys]
namespace Hotkeys
{
static const QString SettingsPrefix = QStringLiteral("HotKeys/");
}

// [API]
namespace Api
{
static const QString IsEnabled = QStringLiteral("API/IsEnabled");
static const QString ListenOnlyOnLoInterface = QStringLiteral("API/ListenOnlyOnLoInterface");
static const QString Port = QStringLiteral("API/Port");
static const QString AuthKey = QStringLiteral("API/AuthKey");
}
namespace Adalight
{
static const QString NumberOfLeds = QStringLiteral("Adalight/NumberOfLeds");
static const QString ColorSequence = QStringLiteral("Adalight/ColorSequence");
static const QString Port = QStringLiteral("Adalight/SerialPort");
static const QString BaudRate = QStringLiteral("Adalight/BaudRate");
static const QString LedMilliAmps = QStringLiteral("Adalight/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Adalight/PowerSupplyAmps");
}
namespace Ardulight
{
static const QString NumberOfLeds = QStringLiteral("Ardulight/NumberOfLeds");
static const QString ColorSequence = QStringLiteral("Ardulight/ColorSequence");
static const QString Port = QStringLiteral("Ardulight/SerialPort");
static const QString BaudRate = QStringLiteral("Ardulight/BaudRate");
static const QString LedMilliAmps = QStringLiteral("Ardulight/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Ardulight/PowerSupplyAmps");
}
namespace AlienFx
{
static const QString NumberOfLeds = QStringLiteral("AlienFx/NumberOfLeds");
static const QString LedMilliAmps = QStringLiteral("AlienFx/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("AlienFx/PowerSupplyAmps");
}
namespace Lightpack
{
static const QString NumberOfLeds = QStringLiteral("Lightpack/NumberOfLeds");
static const QString LedMilliAmps = QStringLiteral("Lightpack/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Lightpack/PowerSupplyAmps");
}
namespace Virtual
{
static const QString NumberOfLeds = QStringLiteral("Virtual/NumberOfLeds");
static const QString LedMilliAmps = QStringLiteral("Virtual/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Virtual/PowerSupplyAmps");
}
namespace Drgb
{
static const QString NumberOfLeds = QStringLiteral("Drgb/NumberOfLeds");
static const QString Address = QStringLiteral("Drgb/Address");
static const QString Port = QStringLiteral("Drgb/Port");
static const QString Timeout = QStringLiteral("Drgb/Timeout");
static const QString LedMilliAmps = QStringLiteral("Drgb/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Drgb/PowerSupplyAmps");
}
namespace Dnrgb
{
static const QString NumberOfLeds = QStringLiteral("Dnrgb/NumberOfLeds");
static const QString Address = QStringLiteral("Dnrgb/Address");
static const QString Port = QStringLiteral("Dnrgb/Port");
static const QString Timeout = QStringLiteral("Dnrgb/Timeout");
static const QString LedMilliAmps = QStringLiteral("Dnrgb/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Dnrgb/PowerSupplyAmps");
}
namespace Warls
{
static const QString NumberOfLeds = QStringLiteral("Warls/NumberOfLeds");
static const QString Address = QStringLiteral("Warls/Address");
static const QString Port = QStringLiteral("Warls/Port");
static const QString Timeout = QStringLiteral("Warls/Timeout");
static const QString LedMilliAmps = QStringLiteral("Warls/LedMilliAmps");
static const QString PowerSupplyAmps = QStringLiteral("Warls/PowerSupplyAmps");
}
} /*Key*/

namespace Value
{
static const QString MainConfigVersion = QStringLiteral(MAIN_CONFIG_FILE_VERSION);

namespace ConnectedDevice
{
static const QString LightpackDevice = QStringLiteral("Lightpack");
static const QString AlienFxDevice = QStringLiteral("AlienFx");
static const QString AdalightDevice = QStringLiteral("Adalight");
static const QString ArdulightDevice = QStringLiteral("Ardulight");
static const QString VirtualDevice = QStringLiteral("Virtual");
static const QString DrgbDevice = QStringLiteral("DRGB");
static const QString DnrgbDevice = QStringLiteral("DNRGB");
static const QString WarlsDevice = QStringLiteral("WARLS");
}

} /*Value*/
} /*Main*/

namespace Profile
{
namespace Key
{
// [General]
static const QString LightpackMode = QStringLiteral("LightpackMode");
static const QString IsBacklightEnabled = QStringLiteral("IsBacklightEnabled");
static const QString CoordinateFormat = QStringLiteral("CoordinateFormat");
// [Grab]
namespace Grab
{
static const QString Grabber = QStringLiteral("Grab/Grabber");
static const QString IsAvgColorsEnabled = QStringLiteral("Grab/IsAvgColorsEnabled");
static const QString IsSendDataOnlyIfColorsChanges = QStringLiteral("Grab/IsSendDataOnlyIfColorsChanges");
static const QString Slowdown = QStringLiteral("Grab/Slowdown");
static const QString LuminosityThreshold = QStringLiteral("Grab/LuminosityThreshold");
static const QString OverBrighten = QStringLiteral("Grab/OverBrighten");
static const QString IsMinimumLuminosityEnabled = QStringLiteral("Grab/IsMinimumLuminosityEnabled");
static const QString IsDx1011GrabberEnabled = QStringLiteral("Grab/IsDX1011GrabberEnabled");
static const QString IsDx9GrabbingEnabled = QStringLiteral("Grab/IsDX9GrabbingEnabled");
static const QString IsApplyBlueLightReductionEnabled = QStringLiteral("Grab/IsApplyGammaRampEnabled");
static const QString IsApplyColorTemperatureEnabled = QStringLiteral("Grab/IsApplyColorTemperatureEnabled");
static const QString ColorTemperature = QStringLiteral("Grab/ColorTemperature");
static const QString Gamma = QStringLiteral("Grab/Gamma");
}
// [MoodLamp]
namespace MoodLamp
{
static const QString IsLiquidMode = QStringLiteral("MoodLamp/LiquidMode");
static const QString Color = QStringLiteral("MoodLamp/Color");
static const QString Speed = QStringLiteral("MoodLamp/Speed");
static const QString Lamp = QStringLiteral("MoodLamp/Lamp");
}
// [SoundVisualizer]
namespace SoundVisualizer
{
static const QString Device = QStringLiteral("SoundVisualizer/Device");
static const QString Visualizer = QStringLiteral("SoundVisualizer/Visualizer");
static const QString MinColor = QStringLiteral("SoundVisualizer/MinColor");
static const QString MaxColor = QStringLiteral("SoundVisualizer/MaxColor");
static const QString IsLiquidMode = QStringLiteral("SoundVisualizer/LiquidMode");
static const QString LiquidSpeed = QStringLiteral("SoundVisualizer/LiquidSpeed");
}
// [Device]
namespace Device
{
static const QString RefreshDelay = QStringLiteral("Device/RefreshDelay");
static const QString IsUsbPowerLedDisabled = QStringLiteral("Device/IsUsbPowerLedDisabled");
static const QString Smooth = QStringLiteral("Device/Smooth");
static const QString Brightness = QStringLiteral("Device/Brightness");
static const QString BrightnessCap = QStringLiteral("Device/BrightnessCap");
static const QString ColorDepth = QStringLiteral("Device/ColorDepth");
static const QString Gamma = QStringLiteral("Device/Gamma");
static const QString IsDitheringEnabled = QStringLiteral("Device/IsDitheringEnabled");
}
// [LED_i]
namespace Led
{

static const QString Prefix = QStringLiteral("LED_");
static const QString IsEnabled = QStringLiteral("IsEnabled");
static const QString Size = QStringLiteral("Size");
static const QString Position = QStringLiteral("Position");
static const QString CoefRed = QStringLiteral("CoefRed");
static const QString CoefGreen = QStringLiteral("CoefGreen");
static const QString CoefBlue = QStringLiteral("CoefBlue");
}
} /*Key*/

namespace Value
{

namespace CoordinateFormat
{
static const QString Logical = QStringLiteral("Logical");
}

namespace LightpackMode
{
static const QString Ambilight = QStringLiteral("Ambilight");
static const QString MoodLamp = QStringLiteral("MoodLamp");
static const QString SoundVisualizer = QStringLiteral("SoundVisualizer");
}

namespace GrabberType
{
static const QString Qt = QStringLiteral("Qt");
static const QString QtEachWidget = QStringLiteral("QtEachWidget");
static const QString WinAPI = QStringLiteral("WinAPI");
static const QString WinAPIEachWidget = QStringLiteral("WinAPIEachWidget");
static const QString X11 = QStringLiteral("X11");
static const QString D3D9 = QStringLiteral("D3D9");
static const QString MacCoreGraphics = QStringLiteral("MacCoreGraphics");
static const QString MacAVFoundation = QStringLiteral("MacAVFoundation");
static const QString DDupl = QStringLiteral("DDupl");
}

} /*Value*/
} /*Profile*/

QMutex Settings::m_mutex;
QSettings * Settings::m_currentProfile;
QSettings * Settings::m_mainConfig; // LightpackMain.conf contains last profile

Settings * Settings::m_this = new Settings();

// Path to directory there store application generated stuff
QString Settings::m_configDirPath = QLatin1String("");

QMap<SupportedDevices::DeviceType, QString> Settings::m_devicesTypeToNameMap;
QMap<SupportedDevices::DeviceType, QString> Settings::m_devicesTypeToKeyNumberOfLedsMap;
QMap<SupportedDevices::DeviceType, QString> Settings::m_devicesTypeToKeyLedMilliAmpsMap;
QMap<SupportedDevices::DeviceType, QString> Settings::m_devicesTypeToKeyPowerSupplyAmpsMap;

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

	if(!m_configDirPath.isEmpty())
		return true;

	if (isPathReparseOrSymlink(applicationDirPath))
	{
		qCritical() << Q_FUNC_INFO << "Refusing config directory that is a symlink/reparse point:" << applicationDirPath;
		return false;
	}

	m_configDirPath = applicationDirPath;

	// Append to the end of dir path '/'
	if (m_configDirPath.lastIndexOf('/') != m_configDirPath.length() - 1)
		m_configDirPath += QStringLiteral("/");

	QString mainConfigPath = getMainConfigPath();
	if (isPathReparseOrSymlink(mainConfigPath) || isPathReparseOrSymlink(getProfilesPath()))
	{
		qCritical() << Q_FUNC_INFO << "Refusing main.conf or Profiles path that is a symlink/reparse point";
		m_configDirPath.clear();
		return false;
	}
	bool settingsWasPresent = QFileInfo::exists(mainConfigPath);

	m_mainConfig = new QSettings(mainConfigPath, QSettings::IniFormat);
	#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	m_mainConfig->setIniCodec("UTF-8");
	#endif

	setNewOptionMain(Main::Key::MainConfigVersion,		Main::Value::MainConfigVersion /* rewrite */);
	setNewOptionMain(Main::Key::ProfileLast,			Main::ProfileNameDefault);
	setNewOptionMain(Main::Key::Language,				Main::LanguageDefault);
	setNewOptionMain(Main::Key::DebugLevel,				Main::DebugLevelDefault);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterExit, Main::IsKeepLightsOnAfterExit);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterLock, Main::IsKeepLightsOnAfterLock);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterSuspend, Main::IsKeepLightsOnAfterSuspend);
	setNewOptionMain(Main::Key::IsKeepLightsOnAfterScreenOff, Main::IsKeepLightsOnAfterScreenOff);
	setNewOptionMain(Main::Key::IsPingDeviceEverySecond,Main::IsPingDeviceEverySecond);
	setNewOptionMain(Main::Key::IsUpdateFirmwareMessageShown, Main::IsUpdateFirmwareMessageShown);
	setNewOptionMain(Main::Key::ConnectedDevice,		Main::ConnectedDeviceDefault);
	setNewOptionMain(Main::Key::SupportedDevices,		Main::SupportedDevices, true /* always rewrite this information to main config */);
	setNewOptionMain(Main::Key::Api::IsEnabled,			Main::Api::IsEnabledDefault);
	setNewOptionMain(Main::Key::Api::ListenOnlyOnLoInterface, Main::Api::ListenOnlyOnLoInterfaceDefault);
	setNewOptionMain(Main::Key::Api::Port,				Main::Api::PortDefault);
	// Auto-generate a UUID AuthKey for brand-new installations while keeping empty default for existing main.conf files
	const QString defaultApiAuthKey = settingsWasPresent ? Main::Api::AuthKey : QUuid::createUuid().toString();
	setNewOptionMain(Main::Key::Api::AuthKey,			defaultApiAuthKey);

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
	setNewOptionMain(Main::Key::Drgb::NumberOfLeds,			Main::Drgb::NumberOfLedsDefault);
	setNewOptionMain(Main::Key::Dnrgb::NumberOfLeds,		Main::Dnrgb::NumberOfLedsDefault);
	setNewOptionMain(Main::Key::Warls::NumberOfLeds,		Main::Warls::NumberOfLedsDefault);

	setNewOptionMain(Main::Key::Adalight::LedMilliAmps,		Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::Ardulight::LedMilliAmps,	Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::AlienFx::LedMilliAmps,		Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::Lightpack::LedMilliAmps,	Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::Virtual::LedMilliAmps,		Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::Drgb::LedMilliAmps,			Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::Dnrgb::LedMilliAmps,		Main::Device::LedMilliAmpsDefault);
	setNewOptionMain(Main::Key::Warls::LedMilliAmps,		Main::Device::LedMilliAmpsDefault);

	setNewOptionMain(Main::Key::Adalight::PowerSupplyAmps,	Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::Ardulight::PowerSupplyAmps,	Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::AlienFx::PowerSupplyAmps,	Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::Lightpack::PowerSupplyAmps,	Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::Virtual::PowerSupplyAmps,	Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::Drgb::PowerSupplyAmps,		Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::Dnrgb::PowerSupplyAmps,		Main::Device::PowerSupplyAmpsDefault);
	setNewOptionMain(Main::Key::Warls::PowerSupplyAmps,		Main::Device::PowerSupplyAmpsDefault);

	setNewOptionMain(Main::Key::Drgb::Address,              Main::Drgb::AddressDefault);
	setNewOptionMain(Main::Key::Drgb::Port,                 Main::Drgb::PortDefault);
	setNewOptionMain(Main::Key::Drgb::Timeout,              Main::Drgb::TimeoutDefault);

	setNewOptionMain(Main::Key::Dnrgb::Address,             Main::Dnrgb::AddressDefault);
	setNewOptionMain(Main::Key::Dnrgb::Port,                Main::Dnrgb::PortDefault);
	setNewOptionMain(Main::Key::Dnrgb::Timeout,             Main::Dnrgb::TimeoutDefault);

	setNewOptionMain(Main::Key::Warls::Address,             Main::Warls::AddressDefault);
	setNewOptionMain(Main::Key::Warls::Port,                Main::Warls::PortDefault);
	setNewOptionMain(Main::Key::Warls::Timeout,             Main::Warls::TimeoutDefault);

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
	if (!isValidProfileName(profileLast))
	{
		qWarning() << Q_FUNC_INFO << "ProfileLast contains an invalid profile name, resetting to default";
		profileLast = Main::ProfileNameDefault;
		setValueMain(Main::Key::ProfileLast, profileLast);
	}

	// Load last profile
	m_currentProfile = new QSettings(QStringLiteral("%1%2.ini").arg(getProfilesPath(), profileLast), QSettings::IniFormat);
	#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	m_currentProfile->setIniCodec("UTF-8");
	#endif

	DEBUG_LOW_LEVEL << "Settings file:" << m_currentProfile->fileName();

	migrateProfileCoordinates(m_currentProfile);

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
	QFileInfoList iniFiles = setsFile.absoluteDir().entryInfoList(QStringList(QStringLiteral("*.ini")));

	QStringList settingsFiles;
	settingsFiles.reserve(iniFiles.count());
	for(int i=0; i<iniFiles.count(); i++){
		QString compBaseName = iniFiles.at(i).completeBaseName();
		settingsFiles.append(compBaseName);
	}

	return settingsFiles;
}

bool Settings::isValidProfileName(const QString & profileName)
{
	if (profileName.isEmpty() || profileName.length() > 128)
		return false;

	if (profileName != profileName.trimmed() || profileName.endsWith(QLatin1Char('.')))
		return false;

	if (profileName == QStringLiteral(".") || profileName == QStringLiteral(".."))
		return false;

	for (const QChar ch : profileName)
	{
		const ushort u = ch.unicode();
		if (u < 0x20 || u == 0x7f)
			return false;
		switch (u)
		{
		case '/':
		case '\\':
		case ':':
		case '*':
		case '?':
		case '"':
		case '<':
		case '>':
		case '|':
		case ';':
			return false;
		default:
			break;
		}
	}

	const int dotIdx = profileName.indexOf(QLatin1Char('.'));
	const QString stem = (dotIdx >= 0 ? profileName.left(dotIdx) : profileName).toUpper();
	if (stem == QStringLiteral("CON") || stem == QStringLiteral("PRN") ||
		stem == QStringLiteral("AUX") || stem == QStringLiteral("NUL"))
	{
		return false;
	}
	if ((stem.startsWith(QStringLiteral("COM")) || stem.startsWith(QStringLiteral("LPT"))) &&
		stem.length() == 4 && stem[3].isDigit() && stem[3] != QLatin1Char('0'))
	{
		return false;
	}

	if (!m_configDirPath.isEmpty())
	{
		if (isPathReparseOrSymlink(m_configDirPath) || isPathReparseOrSymlink(getProfilesPath()))
		{
			return false;
		}
		const QString profilesRoot = QDir::cleanPath(QDir(getProfilesPath()).absolutePath());
		const QString rawCandidate = QStringLiteral("%1%2.ini").arg(getProfilesPath(), profileName);
		if (isPathReparseOrSymlink(rawCandidate))
		{
			return false;
		}
		const QString candidateFile = QDir::cleanPath(QFileInfo(rawCandidate).absoluteFilePath());
		const QString candidateParent = QDir::cleanPath(QFileInfo(candidateFile).absolutePath());
		if (candidateParent.compare(profilesRoot,
#ifdef Q_OS_WIN
			Qt::CaseInsensitive
#else
			Qt::CaseSensitive
#endif
			) != 0)
		{
			return false;
		}
	}

	return true;
}

void Settings::loadOrCreateProfile(const QString & profileName)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << profileName;

	if (!isValidProfileName(profileName))
	{
		qWarning() << Q_FUNC_INFO << "Rejected invalid profile name:" << profileName;
		return;
	}

	QMutexLocker locker(&m_mutex);
	QString currentProfileFileName;
	if (m_currentProfile != NULL)
		currentProfileFileName = m_currentProfile->fileName();

	const QString profileNewPath = QStringLiteral("%1%2.ini").arg(getProfilesPath(), profileName);

	if (!currentProfileFileName.isEmpty()
		&& currentProfileFileName.compare(profileNewPath) == 0)
		return; //nothing to change, profile is already loaded

	if (!currentProfileFileName.isEmpty())
	{
		// Copy current settings to new one
		if (currentProfileFileName != profileNewPath)
			QFile::copy(currentProfileFileName, profileNewPath);

		delete m_currentProfile;
	}

	m_currentProfile = new QSettings(profileNewPath, QSettings::IniFormat);
	#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	m_currentProfile->setIniCodec("UTF-8");
	#endif

	migrateProfileCoordinates(m_currentProfile);

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

	if (!isValidProfileName(profileName))
	{
		qWarning() << Q_FUNC_INFO << "Rejected invalid profile name:" << profileName;
		return;
	}

	QMutexLocker locker(&m_mutex);

	if (m_currentProfile == NULL)
	{
		qWarning() << "void Settings::renameCurrentConfig(): fail, m_currentProfile not initialized";
		return;
	}

	// Copy current settings to new one
	QString profilesDir = QFileInfo( m_currentProfile->fileName() ).absoluteDir().absolutePath();
	QString profileNewPath = QStringLiteral("%1/%2.ini").arg(profilesDir, profileName);

	if (m_currentProfile->fileName() != profileNewPath)
	{
		QFile::rename(m_currentProfile->fileName(), profileNewPath);

		delete m_currentProfile;

		// Update m_currentProfile point to new QSettings with configName
		m_currentProfile = new QSettings(QStringLiteral("%1%2.ini").arg(getProfilesPath(), profileName), QSettings::IniFormat);
		#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
		m_currentProfile->setIniCodec("UTF-8");
		#endif

		DEBUG_LOW_LEVEL << "Settings file renamed:" << m_currentProfile->fileName();

		m_mainConfig->setValue(Main::Key::ProfileLast, profileName);
	}

	m_currentProfile->sync();
	emit m_this->currentProfileNameChanged(profileName);
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

	if (isPathReparseOrSymlink(m_currentProfile->fileName()))
	{
		qWarning() << Q_FUNC_INFO << "Refusing to remove symlink/reparse profile:" << m_currentProfile->fileName();
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

	emit m_this->currentProfileRemoved();
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
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << m_configDirPath;
	return m_configDirPath;
}

QString Settings::getMainConfigPath()
{
	QString mainConfPath = m_configDirPath + QStringLiteral("main.conf");
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

	QRect screen = QGuiApplication::primaryScreen()->geometry();

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
	emit m_this->languageChanged(language);
}

int Settings::getDebugLevel()
{
	return valueMain(Main::Key::DebugLevel).toInt();
}

void Settings::setDebugLevel(int debugLvl)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::DebugLevel, debugLvl);
	emit m_this->debugLevelChanged(debugLvl);
}

bool Settings::isApiEnabled()
{
	return valueMain(Main::Key::Api::IsEnabled).toBool();
}

void Settings::setIsApiEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::IsEnabled, isEnabled);
	emit m_this->apiServerSettingsChanged();
}

bool Settings::isListenOnlyOnLoInterface()
{
	return valueMain(Main::Key::Api::ListenOnlyOnLoInterface).toBool();
}

void Settings::setListenOnlyOnLoInterface(bool localOnly)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::ListenOnlyOnLoInterface, localOnly);
	emit m_this->apiServerSettingsChanged();
}

int Settings::getApiPort()
{
	return valueMain(Main::Key::Api::Port).toInt();
}

void Settings::setApiPort(int apiPort)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Api::Port, apiPort);
	emit m_this->apiServerSettingsChanged();
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
	emit m_this->apiKeyChanged(apiKey);
}

bool Settings::isKeepLightsOnAfterExit()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterExit).toBool();
}

void Settings::setKeepLightsOnAfterExit(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterExit, isEnabled);
	emit m_this->keepLightsOnAfterExitChanged(isEnabled);
}

bool Settings::isKeepLightsOnAfterLock()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterLock).toBool();
}

void Settings::setKeepLightsOnAfterLock(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterLock, isEnabled);
	emit m_this->keepLightsOnAfterLockChanged(isEnabled);
}

bool Settings::isKeepLightsOnAfterSuspend()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterSuspend).toBool();
}

void Settings::setKeepLightsOnAfterSuspend(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterSuspend, isEnabled);
	emit m_this->keepLightsOnAfterSuspendChanged(isEnabled);
}

bool Settings::isKeepLightsOnAfterScreenOff()
{
	return valueMain(Main::Key::IsKeepLightsOnAfterScreenOff).toBool();
}

void Settings::setKeepLightsOnAfterScreenOff(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsKeepLightsOnAfterScreenOff, isEnabled);
	emit m_this->keepLightsOnAfterScreenOffChanged(isEnabled);
}

bool Settings::isPingDeviceEverySecond()
{
	return valueMain(Main::Key::IsPingDeviceEverySecond).toBool();
}

void Settings::setPingDeviceEverySecond(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsPingDeviceEverySecond, isEnabled);
	emit m_this->pingDeviceEverySecondEnabledChanged(isEnabled);
}

bool Settings::isUpdateFirmwareMessageShown()
{
	return valueMain(Main::Key::IsUpdateFirmwareMessageShown).toBool();
}

void Settings::setUpdateFirmwareMessageShown(bool isShown)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::IsUpdateFirmwareMessageShown, isShown);
	emit m_this->updateFirmwareMessageShownChanged(isShown);
}

SupportedDevices::DeviceType Settings::getConnectedDevice()
{
	QString deviceName = valueMain(Main::Key::ConnectedDevice).toString();

	if (std::find(m_devicesTypeToNameMap.cbegin(), m_devicesTypeToNameMap.cend(), deviceName) == m_devicesTypeToNameMap.cend())
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
	emit m_this->connectedDeviceChanged(device);
}

QString Settings::getConnectedDeviceName()
{
	return m_devicesTypeToNameMap.value(getConnectedDevice(), Main::ConnectedDeviceDefault);
}

void Settings::setConnectedDeviceName(const QString & deviceName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (deviceName.isEmpty())
		return; // silent return

	// if (m_devicesTypeToNameMap.values().contains(deviceName) == false)
	if (std::find(m_devicesTypeToNameMap.cbegin(), m_devicesTypeToNameMap.cend(), deviceName) == m_devicesTypeToNameMap.cend())
	{
		qCritical() << Q_FUNC_INFO << "Failure during check the device name" << deviceName << "in m_devicesMap. The main config has not changed.";
		return;
	}

	setValueMain(Main::Key::ConnectedDevice, deviceName);
	emit m_this->connectedDeviceChanged(m_devicesTypeToNameMap.key(deviceName));
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
	emit m_this->hotkeyChanged(actionName, keySequence, oldKeySequence);
}

QString Settings::getAdalightSerialPortName()
{
	return valueMain(Main::Key::Adalight::Port).toString();
}

void Settings::setAdalightSerialPortName(const QString & port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Adalight::Port, port);
	emit m_this->adalightSerialPortNameChanged(port);
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
	emit m_this->adalightSerialPortBaudRateChanged(baud);
}

QString Settings::getArdulightSerialPortName()
{
	return valueMain(Main::Key::Ardulight::Port).toString();
}

void Settings::setArdulightSerialPortName(const QString & port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Ardulight::Port, port);
	emit m_this->ardulightSerialPortNameChanged(port);
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
	emit m_this->ardulightSerialPortBaudRateChanged(baud);
}

QString Settings::getDrgbAddress()
{
	return valueMain(Main::Key::Drgb::Address).toString();
}

void Settings::setDrgbAddress(const QString& address)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Drgb::Address, address);
	emit m_this->drgbAddressChanged(address);
}

QString Settings::getDrgbPort()
{
	return valueMain(Main::Key::Drgb::Port).toString();
}

void Settings::setDrgbPort(const QString& port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Drgb::Port, port);
	emit m_this->drgbPortChanged(port);
}

int Settings::getDrgbTimeout()
{
	return valueMain(Main::Key::Drgb::Timeout).toInt();
}

void Settings::setDrgbTimeout(const int timeout)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Drgb::Timeout, timeout);
	emit m_this->drgbTimeoutChanged(timeout);
}

QString Settings::getDnrgbAddress()
{
	return valueMain(Main::Key::Dnrgb::Address).toString();
}

void Settings::setDnrgbAddress(const QString& address)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Dnrgb::Address, address);
	emit m_this->dnrgbAddressChanged(address);
}

QString Settings::getDnrgbPort()
{
	return valueMain(Main::Key::Dnrgb::Port).toString();
}

void Settings::setDnrgbPort(const QString& port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Dnrgb::Port, port);
	emit m_this->dnrgbPortChanged(port);
}

int Settings::getDnrgbTimeout()
{
	return valueMain(Main::Key::Dnrgb::Timeout).toInt();
}

void Settings::setDnrgbTimeout(const int timeout)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Dnrgb::Timeout, timeout);
	emit m_this->dnrgbTimeoutChanged(timeout);
}

QString Settings::getWarlsAddress()
{
	return valueMain(Main::Key::Warls::Address).toString();
}

void Settings::setWarlsAddress(const QString& address)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Warls::Address, address);
	emit m_this->warlsAddressChanged(address);
}

QString Settings::getWarlsPort()
{
	return valueMain(Main::Key::Warls::Port).toString();
}

void Settings::setWarlsPort(const QString& port)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Warls::Port, port);
	emit m_this->warlsPortChanged(port);
}

int Settings::getWarlsTimeout()
{
	return valueMain(Main::Key::Warls::Timeout).toInt();
}

void Settings::setWarlsTimeout(const int timeout)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValueMain(Main::Key::Warls::Timeout, timeout);
	emit m_this->warlsTimeoutChanged(timeout);
}

QStringList Settings::getSupportedSerialPortBaudRates()
{
	QStringList list;

	// TODO: Add more baud rates if need it
	list.append(QStringLiteral("2000000"));
	list.append(QStringLiteral("1500000"));
	list.append(QStringLiteral("1000000"));
	list.append(QStringLiteral("921600"));
	list.append(QStringLiteral("500000"));
	list.append(QStringLiteral("460800"));
	list.append(QStringLiteral("256000"));
	list.append(QStringLiteral("230400"));
	list.append(QStringLiteral("153600"));
	list.append(QStringLiteral("128000"));

	list.append(QStringLiteral("115200"));
	list.append(QStringLiteral("57600"));
	list.append(QStringLiteral("9600"));
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

	if (key.isEmpty())
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "numberOfLeds ==" << numberOfLeds;
		return;
	}

	setValueMain(key, numberOfLeds);
	{
		using namespace SupportedDevices;
		switch(device) {
			case DeviceTypeLightpack:
			emit m_this->lightpackNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeAdalight:
			emit m_this->adalightNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeArdulight:
			emit m_this->ardulightNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeVirtual:
			emit m_this->virtualNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeDrgb:
			emit m_this->drgbNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeDnrgb:
			emit m_this->dnrgbNumberOfLedsChanged(numberOfLeds);
			break;

			case DeviceTypeWarls:
			emit m_this->warlsNumberOfLedsChanged(numberOfLeds);
			break;
		default:
			qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "numberOfLeds ==" << numberOfLeds;
		}
	}
}

int Settings::getNumberOfLeds(SupportedDevices::DeviceType device)
{
	QString key = m_devicesTypeToKeyNumberOfLedsMap.value(device);

	if (key.isEmpty())
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device;
		return MaximumNumberOfLeds::Default;
	}

	// TODO: validator on maximum number of leds for current 'device'
	return valueMain(key).toInt();
}

void Settings::setDeviceLedMilliAmps(const SupportedDevices::DeviceType device, const int mAmps)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if(getDeviceLedMilliAmps(device) == mAmps)
		//nothing to do
		return;

	const QString& key = m_devicesTypeToKeyLedMilliAmpsMap.value(device);

	if (key.isEmpty())
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "LedMilliAmps ==" << mAmps;
		return;
	}

	setValueMain(key, mAmps);
	{
		using namespace SupportedDevices;
		switch(device) {
			case DeviceTypeLightpack:
			emit m_this->lightpackLedMilliAmpsChanged(mAmps);
			break;

			case DeviceTypeAdalight:
			emit m_this->adalightLedMilliAmpsChanged(mAmps);
			break;

			case DeviceTypeArdulight:
			emit m_this->ardulightLedMilliAmpsChanged(mAmps);
			break;

			case DeviceTypeVirtual:
			emit m_this->virtualLedMilliAmpsChanged(mAmps);
			break;

			case DeviceTypeDrgb:
			emit m_this->drgbLedMilliAmpsChanged(mAmps);
			break;

			case DeviceTypeDnrgb:
			emit m_this->dnrgbLedMilliAmpsChanged(mAmps);
			break;

			case DeviceTypeWarls:
			emit m_this->warlsLedMilliAmpsChanged(mAmps);
			break;
		default:
			qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "LedMilliAmps ==" << mAmps;
		}
	}
}

int Settings::getDeviceLedMilliAmps(const SupportedDevices::DeviceType device)
{
	const QString& key = m_devicesTypeToKeyLedMilliAmpsMap.value(device);

	if (key.isEmpty())
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device;
		return Main::Device::LedMilliAmpsDefault;
	}

	// TODO: validator on maximum number of leds for current 'device'
	return valueMain(key).toInt();
}


void Settings::setDevicePowerSupplyAmps(const SupportedDevices::DeviceType device, const double amps)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if(getDevicePowerSupplyAmps(device) == amps)
		//nothing to do
		return;

	const QString& key = m_devicesTypeToKeyPowerSupplyAmpsMap.value(device);

	if (key.isEmpty())
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "PowerSupplyAmps ==" << amps;
		return;
	}

	setValueMain(key, amps);
	{
		using namespace SupportedDevices;
		switch(device) {
			case DeviceTypeLightpack:
			emit m_this->lightpackPowerSupplyAmpsChanged(amps);
			break;

			case DeviceTypeAdalight:
			emit m_this->adalightPowerSupplyAmpsChanged(amps);
			break;

			case DeviceTypeArdulight:
			emit m_this->ardulightPowerSupplyAmpsChanged(amps);
			break;

			case DeviceTypeVirtual:
			emit m_this->virtualPowerSupplyAmpsChanged(amps);
			break;

			case DeviceTypeDrgb:
			emit m_this->drgbPowerSupplyAmpsChanged(amps);
			break;

			case DeviceTypeDnrgb:
			emit m_this->dnrgbPowerSupplyAmpsChanged(amps);
			break;

			case DeviceTypeWarls:
			emit m_this->warlsPowerSupplyAmpsChanged(amps);
			break;
		default:
			qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device << "PowerSupplyAmps ==" << amps;
		}
	}
}

double Settings::getDevicePowerSupplyAmps(const SupportedDevices::DeviceType device)
{
	const QString& key = m_devicesTypeToKeyPowerSupplyAmpsMap.value(device);

	if (key.isEmpty())
	{
		qCritical() << Q_FUNC_INFO << "Device type not recognized, device ==" << device;
		return Main::Device::PowerSupplyAmpsDefault;
	}

	// TODO: validator on maximum number of leds for current 'device'
	return valueMain(key).toDouble();
}

void Settings::setColorSequence(SupportedDevices::DeviceType device, const QString& colorSequence)
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
	emit m_this->deviceColorSequenceChanged(colorSequence);
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
	emit m_this->grabSlowdownChanged(value);
}

bool Settings::isBacklightEnabled()
{
	return value(Profile::Key::IsBacklightEnabled).toBool();
}

void Settings::setIsBacklightEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::IsBacklightEnabled, isEnabled);
	emit m_this->backlightEnabledChanged(isEnabled);
}

bool Settings::isGrabAvgColorsEnabled()
{
	return value(Profile::Key::Grab::IsAvgColorsEnabled).toBool();
}

void Settings::setGrabAvgColorsEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsAvgColorsEnabled, isEnabled);
	emit m_this->grabAvgColorsEnabledChanged(isEnabled);
}

int Settings::getGrabOverBrighten()
{
	return getValidGrabOverBrighten(value(Profile::Key::Grab::OverBrighten).toInt());
}

void Settings::setGrabOverBrighten(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::OverBrighten, getValidGrabOverBrighten(value));
	emit m_this->grabOverBrightenChanged(value);
}

bool Settings::isGrabApplyBlueLightReductionEnabled()
{
	return value(Profile::Key::Grab::IsApplyBlueLightReductionEnabled).toBool();
}

void Settings::setGrabApplyBlueLightReductionEnabled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsApplyBlueLightReductionEnabled, value);
	emit m_this->grabApplyBlueLightReductionChanged(value);
}

bool Settings::isGrabApplyColorTemperatureEnabled()
{
	return value(Profile::Key::Grab::IsApplyColorTemperatureEnabled).toBool();
}
void Settings::setGrabApplyColorTemperatureEnabled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsApplyColorTemperatureEnabled, value);
	emit m_this->grabApplyColorTemperatureChanged(value);
}
int Settings::getGrabColorTemperature()
{
	return value(Profile::Key::Grab::ColorTemperature).toInt();
}
void Settings::setGrabColorTemperature(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::ColorTemperature, value);
	emit m_this->grabColorTemperatureChanged(value);
}
double Settings::getGrabGamma()
{
	return value(Profile::Key::Grab::Gamma).toDouble();
}
void Settings::setGrabGamma(double gamma)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::Gamma, gamma);
	emit m_this->grabGammaChanged(gamma);
}

bool Settings::isSendDataOnlyIfColorsChanges()
{
	return value(Profile::Key::Grab::IsSendDataOnlyIfColorsChanges).toBool();
}

void Settings::setSendDataOnlyIfColorsChanges(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsSendDataOnlyIfColorsChanges, isEnabled);
	emit m_this->sendDataOnlyIfColorsChangesChanged(isEnabled);
}

int Settings::getLuminosityThreshold()
{
	return getValidLuminosityThreshold(value(Profile::Key::Grab::LuminosityThreshold).toInt());
}

void Settings::setLuminosityThreshold(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::LuminosityThreshold, getValidLuminosityThreshold(value));
	emit m_this->luminosityThresholdChanged(value);
}

bool Settings::isMinimumLuminosityEnabled()
{
	return value(Profile::Key::Grab::IsMinimumLuminosityEnabled).toBool();
}

void Settings::setMinimumLuminosityEnabled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Grab::IsMinimumLuminosityEnabled, value);
	emit m_this->minimumLuminosityEnabledChanged(value);
}

int Settings::getDeviceRefreshDelay()
{
	return getValidDeviceRefreshDelay(value(Profile::Key::Device::RefreshDelay).toInt());
}

void Settings::setDeviceRefreshDelay(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::RefreshDelay, getValidDeviceRefreshDelay(value));
	emit m_this->deviceRefreshDelayChanged(value);
}

bool Settings::isDeviceUsbPowerLedDisabled() {
	return value(Profile::Key::Device::IsUsbPowerLedDisabled).toBool();
}

void Settings::setDeviceUsbPowerLedDisabled(bool isDisabled) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::IsUsbPowerLedDisabled, isDisabled);
	emit m_this->deviceUsbPowerLedDisabledChanged(isDisabled);
}

int Settings::getDeviceBrightness()
{
	return getValidDeviceBrightness(value(Profile::Key::Device::Brightness).toInt());
}

void Settings::setDeviceBrightness(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::Brightness, getValidDeviceBrightness(value));
	emit m_this->deviceBrightnessChanged(value);
}

int Settings::getDeviceBrightnessCap()
{
	return getValidDeviceBrightnessCap(value(Profile::Key::Device::BrightnessCap).toInt());
}

void Settings::setDeviceBrightnessCap(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::BrightnessCap, getValidDeviceBrightnessCap(value));
	emit m_this->deviceBrightnessCapChanged(value);
}

int Settings::getDeviceSmooth()
{
	return getValidDeviceSmooth(value(Profile::Key::Device::Smooth).toInt());
}

void Settings::setDeviceSmooth(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::Smooth, getValidDeviceSmooth(value));
	emit m_this->deviceSmoothChanged(value);
}

int Settings::getDeviceColorDepth()
{
	return getValidDeviceColorDepth(value(Profile::Key::Device::ColorDepth).toInt());
}

void Settings::setDeviceColorDepth(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::ColorDepth, getValidDeviceColorDepth(value));
	emit m_this->deviceColorDepthChanged(value);
}

double Settings::getDeviceGamma()
{
	return getValidDeviceGamma(value(Profile::Key::Device::Gamma).toDouble());
}

void Settings::setDeviceGamma(double gamma)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::Gamma, getValidDeviceGamma(gamma));
	emit m_this->deviceGammaChanged(gamma);
}

bool Settings::isDeviceDitheringEnabled()
{
	return value(Profile::Key::Device::IsDitheringEnabled).toBool();
}

void Settings::setDeviceDitheringEnabled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::Device::IsDitheringEnabled, isEnabled);
	emit m_this->deviceDitheringEnabledChanged(isEnabled);
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
#ifdef MAC_OS_AV_GRAB_SUPPORT
	if (strGrabber == Profile::Value::GrabberType::MacAVFoundation)
		return Grab::GrabberTypeMacAVFoundation;
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
#ifdef MAC_OS_AV_GRAB_SUPPORT
	case Grab::GrabberTypeMacAVFoundation:
		strGrabber = Profile::Value::GrabberType::MacAVFoundation;
		break;
#endif

	default:
		qWarning() << Q_FUNC_INFO << "Switch on grabberType =" << grabberType << "failed. Reset to default value.";
		strGrabber = Profile::Grab::GrabberDefaultString;
	}
	setValue(Profile::Key::Grab::Grabber, strGrabber);
	emit m_this->grabberTypeChanged(grabberType);
}

#ifdef D3D10_GRAB_SUPPORT
bool Settings::isDx1011GrabberEnabled() {
	return value(Profile::Key::Grab::IsDx1011GrabberEnabled).toBool();
}

void Settings::setDx1011GrabberEnabled(bool isEnabled) {
	setValue(Profile::Key::Grab::IsDx1011GrabberEnabled, isEnabled);
	emit m_this->dx1011GrabberEnabledChanged(isEnabled);
}

bool Settings::isDx9GrabbingEnabled() {
	return value(Profile::Key::Grab::IsDx9GrabbingEnabled).toBool();
}

void Settings::setDx9GrabbingEnabled(bool isEnabled) {
	setValue(Profile::Key::Grab::IsDx9GrabbingEnabled, isEnabled);
	emit m_this->dx9GrabberEnabledChanged(isEnabled);
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
#ifdef SOUNDVIZ_SUPPORT
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
		emit m_this->lightpackModeChanged(mode);
	}
	else if (mode == Lightpack::MoodLampMode)
	{
		setValue(Profile::Key::LightpackMode, Profile::Value::LightpackMode::MoodLamp);
		emit m_this->lightpackModeChanged(mode);
	}
#ifdef SOUNDVIZ_SUPPORT
	else if (mode == Lightpack::SoundVisualizeMode)
	{
		setValue(Profile::Key::LightpackMode, Profile::Value::LightpackMode::SoundVisualizer);
		emit m_this->lightpackModeChanged(mode);
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
	emit m_this->moodLampLiquidModeChanged(value);
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
	emit m_this->moodLampColorChanged(value);
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
	emit m_this->moodLampSpeedChanged(value);
}

int Settings::getMoodLampLamp()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return value(Profile::Key::MoodLamp::Lamp).toInt();
}

void Settings::setMoodLampLamp(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(Profile::Key::MoodLamp::Lamp, value);
	emit m_this->moodLampLampChanged(value);
}

#ifdef SOUNDVIZ_SUPPORT
int Settings::getSoundVisualizerDevice()
{
	return value(Profile::Key::SoundVisualizer::Device).toInt();
}

void Settings::setSoundVisualizerDevice(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	setValue(Profile::Key::SoundVisualizer::Device, value);
	emit m_this->soundVisualizerDeviceChanged(value);
}

int Settings::getSoundVisualizerVisualizer()
{
	return value(Profile::Key::SoundVisualizer::Visualizer).toInt();
}

void Settings::setSoundVisualizerVisualizer(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	setValue(Profile::Key::SoundVisualizer::Visualizer, value);
	emit m_this->soundVisualizerVisualizerChanged(value);
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
	emit m_this->soundVisualizerMinColorChanged(value);
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
	emit m_this->soundVisualizerMaxColorChanged(value);
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
	emit m_this->soundVisualizerLiquidModeChanged(value);
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
	emit m_this->soundVisualizerLiquidSpeedChanged(value);
}
#endif

QList<WBAdjustment> Settings::getLedCoefs()
{
	QList<WBAdjustment> result;
	const int numOfLeds = getNumberOfLeds(getConnectedDevice());

	result.reserve(numOfLeds);
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
	emit m_this->ledCoefRedChanged(ledIndex, value);
}

void Settings::setLedCoefGreen(int ledIndex, double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValidLedCoef(ledIndex, Profile::Key::Led::CoefGreen, value);
	emit m_this->ledCoefGreenChanged(ledIndex, value);
}

void Settings::setLedCoefBlue(int ledIndex, double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValidLedCoef(ledIndex, Profile::Key::Led::CoefBlue, value);
	emit m_this->ledCoefBlueChanged(ledIndex, value);
}

QSize Settings::getLedSize(int ledIndex)
{
	return value(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), Profile::Key::Led::Size)).toSize();
}

void Settings::setLedSize(int ledIndex, QSize size)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), Profile::Key::Led::Size), size);
	emit m_this->ledSizeChanged(ledIndex, size);
}

QPoint Settings::getLedPosition(int ledIndex)
{
	return value(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), Profile::Key::Led::Position)).toPoint();
}

void Settings::setLedPosition(int ledIndex, QPoint position)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), Profile::Key::Led::Position), position);
	emit m_this->ledPositionChanged(ledIndex, position);
}

bool Settings::isLedEnabled(int ledIndex)
{

	QVariant result = value(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), Profile::Key::Led::IsEnabled));
	if (result.isNull())
		return Profile::Led::IsEnabledDefault;
	else
		return result.toBool();
}

void Settings::setLedEnabled(int ledIndex, bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	setValue(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), Profile::Key::Led::IsEnabled), isEnabled);
	emit m_this->ledEnabledChanged(ledIndex, isEnabled);
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

int Settings::getValidDeviceBrightnessCap(int value)
{
	if (value < Profile::Device::BrightnessCapMin)
		value = Profile::Device::BrightnessCapMin;
	else if (value > Profile::Device::BrightnessCapMax)
		value = Profile::Device::BrightnessCapMax;
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
		const QString error = QStringLiteral("Error: outside the valid values (coef < %1 || coef > %2)")
			.arg(QString::number(Profile::Led::CoefMin), QString::number(Profile::Led::CoefMax));

		qWarning() << Q_FUNC_INFO << "Settings bad value"
					<< "[" + Profile::Key::Led::Prefix + QString::number(ledIndex + 1) + "]"
					<< keyCoef
					<< error
					<< "Convert to double error. Set it to default value" << keyCoef << "=" << Profile::Led::CoefDefault;
		coef = Profile::Led::CoefDefault;
	}
	Settings::setValue(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), keyCoef), coef);
}

double Settings::getValidLedCoef(int ledIndex, const QString & keyCoef)
{
	bool ok = false;
	double coef = Settings::value(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), keyCoef)).toDouble(&ok);
	QString error;
	if (ok == false){
		error = QStringLiteral("Error: Convert to double.");
	} else if (coef < Profile::Led::CoefMin || coef > Profile::Led::CoefMax){
		error = QStringLiteral("Error: outside the valid values (coef < %1 || coef > %2)")
					.arg(QString::number(Profile::Led::CoefMin), QString::number(Profile::Led::CoefMax));
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
	Settings::setValue(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(ledIndex + 1), keyCoef), coef);
	return coef;
}

QString Settings::getProfilesPath() {
	return QStringLiteral("%1Profiles/").arg(m_configDirPath);
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	setNewOption(Profile::Key::CoordinateFormat,					Profile::Value::CoordinateFormat::Logical, isResetDefault);
#endif
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
	setNewOption(Profile::Key::Grab::IsApplyBlueLightReductionEnabled,		Profile::Grab::IsApplyBlueLightReductionEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::IsApplyColorTemperatureEnabled,Profile::Grab::IsApplyColorTemperatureEnabledDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::ColorTemperature,              Profile::Grab::ColorTemperatureDefault, isResetDefault);
	setNewOption(Profile::Key::Grab::Gamma,                         Profile::Grab::GammaDefault, isResetDefault);
	// [MoodLamp]
	setNewOption(Profile::Key::MoodLamp::IsLiquidMode,				Profile::MoodLamp::IsLiquidModeDefault, isResetDefault);
	setNewOption(Profile::Key::MoodLamp::Color,						Profile::MoodLamp::ColorDefault, isResetDefault);
	setNewOption(Profile::Key::MoodLamp::Speed,						Profile::MoodLamp::SpeedDefault, isResetDefault);
	setNewOption(Profile::Key::MoodLamp::Lamp,						Profile::MoodLamp::LampDefault, isResetDefault);
#ifdef SOUNDVIZ_SUPPORT
	// [SoundVisualizer]
	setNewOption(Profile::Key::SoundVisualizer::Device,				Profile::SoundVisualizer::DeviceDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::Visualizer,			Profile::SoundVisualizer::VisualizerDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::MinColor,			Profile::SoundVisualizer::MinColorDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::MaxColor,			Profile::SoundVisualizer::MaxColorDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::IsLiquidMode,		Profile::SoundVisualizer::IsLiquidModeDefault, isResetDefault);
	setNewOption(Profile::Key::SoundVisualizer::LiquidSpeed,		Profile::SoundVisualizer::LiquidSpeedDefault, isResetDefault);
#endif
	// [Device]
	setNewOption(Profile::Key::Device::RefreshDelay,				Profile::Device::RefreshDelayDefault, isResetDefault);
	setNewOption(Profile::Key::Device::IsUsbPowerLedDisabled,		Profile::Device::IsUsbPowerLedDisabledDefault, isResetDefault);
	setNewOption(Profile::Key::Device::Brightness,					Profile::Device::BrightnessDefault, isResetDefault);
	setNewOption(Profile::Key::Device::BrightnessCap,				Profile::Device::BrightnessCapDefault, isResetDefault);
	setNewOption(Profile::Key::Device::Smooth,						Profile::Device::SmoothDefault, isResetDefault);
	setNewOption(Profile::Key::Device::Gamma,						Profile::Device::GammaDefault, isResetDefault);
	setNewOption(Profile::Key::Device::ColorDepth,					Profile::Device::ColorDepthDefault, isResetDefault);
	setNewOption(Profile::Key::Device::IsDitheringEnabled,			Profile::Device::IsDitheringEnabledDefault, isResetDefault);


	QPoint ledPosition;

	for (int i = 0; i < MaximumNumberOfLeds::AbsoluteMaximum; i++)
	{
		ledPosition = getDefaultPosition(i);


		setNewOption(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::IsEnabled),
						Profile::Led::IsEnabledDefault, isResetDefault);
		setNewOption(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::Position),
						ledPosition, isResetDefault);
		setNewOption(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::Size),
						Profile::Led::SizeDefault, isResetDefault);

		setNewOption(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::CoefRed),
						Profile::Led::CoefDefault, isResetDefault);
		setNewOption(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::CoefGreen),
						Profile::Led::CoefDefault, isResetDefault);
		setNewOption(QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::CoefBlue),
						Profile::Led::CoefDefault, isResetDefault);
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "led";
	QMutexLocker locker(&m_mutex);
	m_currentProfile->sync();
	locker.unlock();
	emit m_this->currentProfileInited(getCurrentProfileName());
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

	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeAdalight] = Main::Value::ConnectedDevice::AdalightDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeArdulight] = Main::Value::ConnectedDevice::ArdulightDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeLightpack] = Main::Value::ConnectedDevice::LightpackDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeVirtual] = Main::Value::ConnectedDevice::VirtualDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeDrgb] = Main::Value::ConnectedDevice::DrgbDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeDnrgb] = Main::Value::ConnectedDevice::DnrgbDevice;
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeWarls] = Main::Value::ConnectedDevice::WarlsDevice;

	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeAdalight] = Main::Key::Adalight::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeArdulight] = Main::Key::Ardulight::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeLightpack] = Main::Key::Lightpack::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeVirtual] = Main::Key::Virtual::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeDrgb] = Main::Key::Drgb::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeDnrgb] = Main::Key::Dnrgb::NumberOfLeds;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeWarls] = Main::Key::Warls::NumberOfLeds;

	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeAdalight] = Main::Key::Adalight::LedMilliAmps;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeArdulight] = Main::Key::Ardulight::LedMilliAmps;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeLightpack] = Main::Key::Lightpack::LedMilliAmps;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeVirtual] = Main::Key::Virtual::LedMilliAmps;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeDrgb] = Main::Key::Drgb::LedMilliAmps;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeDnrgb] = Main::Key::Dnrgb::LedMilliAmps;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeWarls] = Main::Key::Warls::LedMilliAmps;

	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeAdalight] = Main::Key::Adalight::PowerSupplyAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeArdulight] = Main::Key::Ardulight::PowerSupplyAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeLightpack] = Main::Key::Lightpack::PowerSupplyAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeVirtual] = Main::Key::Virtual::PowerSupplyAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeDrgb] = Main::Key::Drgb::PowerSupplyAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeDnrgb] = Main::Key::Dnrgb::PowerSupplyAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeWarls] = Main::Key::Warls::PowerSupplyAmps;
#ifdef ALIEN_FX_SUPPORTED
	m_devicesTypeToNameMap[SupportedDevices::DeviceTypeAlienFx] = Main::Value::ConnectedDevice::AlienFxDevice;
	m_devicesTypeToKeyNumberOfLedsMap[SupportedDevices::DeviceTypeAlienFx] = Main::Key::AlienFx::NumberOfLeds;
	m_devicesTypeToKeyLedMilliAmpsMap[SupportedDevices::DeviceTypeAlienFx] = Main::Key::AlienFx::LedMilliAmps;
	m_devicesTypeToKeyPowerSupplyAmpsMap[SupportedDevices::DeviceTypeAlienFx] = Main::Key::AlienFx::PowerSupplyAmps;
#endif
}

/*
 * --------------------------- Migration --------------------------------
 */


void Settings::migrateSettings()
{

	if (valueMain(Main::Key::MainConfigVersion).toString() == QStringLiteral("1.0")) {

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
		setValueMain(Main::Key::MainConfigVersion, QStringLiteral("2.0"));
	}
	if (valueMain(Main::Key::MainConfigVersion).toString() == QStringLiteral("2.0")) {
		// Disable ApiAuth by default
		setApiKey(QLatin1String(""));

		// Remove obsolete keys
		QString authEnabledKey = QStringLiteral("API/IsAuthEnabled");
		if(m_mainConfig->contains(authEnabledKey)) {
			m_mainConfig->remove(authEnabledKey);
		}

		setValueMain(Main::Key::MainConfigVersion, QStringLiteral("3.0"));
	}
	if (valueMain(Main::Key::MainConfigVersion).toString() == QStringLiteral("3.0")) {
#ifdef WINAPI_GRAB_SUPPORT
		Settings::setGrabberType(Grab::GrabberTypeWinAPI);
#endif

#ifdef X11_GRAB_SUPPORT
		Settings::setGrabberType(Grab::GrabberTypeX11);
#endif

#if defined(MAC_OS_AV_GRAB_SUPPORT)
		Settings::setGrabberType(Grab::GrabberTypeMacAVFoundation);
#elif defined(MAC_OS_CG_GRAB_SUPPORT)
		Settings::setGrabberType(Grab::GrabberTypeMacCoreGraphics);
#endif

		setValueMain(Main::Key::MainConfigVersion, QStringLiteral("4.0"));
	}
}

void Settings::migrateProfileCoordinates(QSettings *profile)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	if (!profile || !QFile::exists(profile->fileName()))
		return;

	QString coordFormat = profile->value(Profile::Key::CoordinateFormat).toString();
	if (coordFormat == Profile::Value::CoordinateFormat::Logical) {
		return;
	}

#if defined(Q_OS_MACOS)
	// On macOS, NSHighResolutionCapable=True in Info.plist meant Qt5 already stored LED coordinates in logical points.
	profile->setValue(Profile::Key::CoordinateFormat, Profile::Value::CoordinateFormat::Logical);
	profile->sync();
	return;
#else
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Migrating profile coordinates to Qt6 logical DIP:" << profile->fileName();

	// Backup original profile file before modifying (only if writable and backup doesn't already exist)
	QString backupFileName = profile->fileName() + QStringLiteral(".qt5bak");
	if (profile->isWritable() && !QFile::exists(backupFileName)) {
		QFile::copy(profile->fileName(), backupFileName);
	}

	for (int i = 0; i < MaximumNumberOfLeds::AbsoluteMaximum; ++i) {
		QString posKey = QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::Position);
		QString sizeKey = QStringLiteral("%1%2/%3").arg(Profile::Key::Led::Prefix, QString::number(i + 1), Profile::Key::Led::Size);

		if (!profile->contains(posKey) || !profile->contains(sizeKey))
			continue;

		QPoint physicalPos = profile->value(posKey).toPoint();
		QSize physicalSize = profile->value(sizeKey).toSize();

		if (physicalSize.isEmpty())
			continue;

		QPoint physicalCenter = physicalPos + QPoint(physicalSize.width() / 2, physicalSize.height() / 2);
		QPoint logicalPos = physicalPos;
		QSize logicalSize = physicalSize;

#if defined(Q_OS_WIN)
		HMONITOR hMon = MonitorFromPoint(POINT{ physicalCenter.x(), physicalCenter.y() }, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEX mi;
		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		QScreen *targetScreen = nullptr;
		QPoint screenPhysicalOrigin;
		if (hMon && GetMonitorInfo(hMon, &mi)) {
			QString devName = QString::fromWCharArray(mi.szDevice);
			for (QScreen *s : QGuiApplication::screens()) {
				if (s->name() == devName) {
					targetScreen = s;
					screenPhysicalOrigin = QPoint(mi.rcMonitor.left, mi.rcMonitor.top);
					break;
				}
			}
		}
		if (!targetScreen) {
			targetScreen = QGuiApplication::primaryScreen();
			if (targetScreen)
				screenPhysicalOrigin = targetScreen->geometry().topLeft();
		}

		if (targetScreen) {
			qreal dpr = targetScreen->devicePixelRatio();
			if (dpr > 0.0) {
				QPoint physicalOffset = physicalPos - screenPhysicalOrigin;
				logicalPos = targetScreen->geometry().topLeft() + QPoint(qRound(physicalOffset.x() / dpr), qRound(physicalOffset.y() / dpr));
				logicalSize = QSize(qMax(1, qRound(physicalSize.width() / dpr)), qMax(1, qRound(physicalSize.height() / dpr)));
			}
		}
#else
		QScreen *targetScreen = nullptr;
		for (QScreen *s : QGuiApplication::screens()) {
			qreal dpr = s->devicePixelRatio();
			QRect physicalScreenRect(
				s->geometry().topLeft(),
				QSize(qRound(s->geometry().width() * dpr), qRound(s->geometry().height() * dpr))
			);
			if (physicalScreenRect.contains(physicalCenter)) {
				targetScreen = s;
				break;
			}
		}
		if (!targetScreen) {
			targetScreen = QGuiApplication::primaryScreen();
		}
		if (targetScreen) {
			qreal dpr = targetScreen->devicePixelRatio();
			if (dpr > 0.0) {
				QPoint screenPhysicalTopLeft = targetScreen->geometry().topLeft();
				QPoint physicalOffset = physicalPos - screenPhysicalTopLeft;
				logicalPos = targetScreen->geometry().topLeft() + QPoint(qRound(physicalOffset.x() / dpr), qRound(physicalOffset.y() / dpr));
				logicalSize = QSize(qMax(1, qRound(physicalSize.width() / dpr)), qMax(1, qRound(physicalSize.height() / dpr)));
			}
		}
#endif

		profile->setValue(posKey, logicalPos);
		profile->setValue(sizeKey, logicalSize);
	}

	profile->setValue(Profile::Key::CoordinateFormat, Profile::Value::CoordinateFormat::Logical);
	profile->sync();
#endif
#else
	Q_UNUSED(profile)
#endif
}
} /*SettingsScope*/

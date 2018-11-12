/*
 * Settings.hpp
 *
 *	Created on: 29.07.2010
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

#pragma once

#include <QSettings>
#include <QVariant>
#include <QMutex>
#include <QColor>

#include "SettingsDefaults.hpp"
#include "enums.hpp"
#include "../common/defs.h"
#include "debug.h"
#include "types.h"

namespace SettingsScope
{

struct LedInfo {
	bool isEnabled;
	QPoint position;
	QSize size;
	double wbRed;
	double wbGreen;
	double wbBlue;
};

/*!
	Provides access to persistent settings.
*/
class Settings : public QObject
{
	Q_OBJECT

public:
	Settings();

	/*!
		* \brief Initialize reads settings file or create a new one and initializes it with default settings
		* \param applicationDirPath
		* \param isSetDebugLevelFromConfig
		* \return is settings file was present before initialization
		*/
	static bool Initialize(const QString & applicationDirPath, bool isSetDebugLevelFromConfig);
	static void resetDefaults();
	static const Settings * settingsSingleton() { return m_this; }
	static bool isPresent(const QString & applicationDirPath);

	static QStringList findAllProfiles();
	static void loadOrCreateProfile(const QString & configName);
	static void renameCurrentProfile(const QString & configName);
	static void removeCurrentProfile();
	static bool isProfileLoaded();

	static QString getCurrentProfileName();
	/*!
		use with caution: if there is no profile loaded then it will throw access violation exception
		\see SettingsScope#Settings#getProfilesPath()
		\return QString path to current profile
	*/
	static QString getCurrentProfilePath();
	static QString getProfilesPath();
	static QString getApplicationDirPath();
	static QString getMainConfigPath();
	static QPoint getDefaultPosition(int ledIndex);

	// Main
	static QString getLastProfileName();
	static QString getLanguage();
	static void setLanguage(const QString & language);
	static int getDebugLevel();
	static void setDebugLevel(int debugLvl);
	static bool isApiEnabled();
	static void setIsApiEnabled(bool isEnabled);
	static bool isListenOnlyOnLoInterface();
	static void setListenOnlyOnLoInterface(bool localOnly);
	static int getApiPort();
	static void setApiPort(int apiPort);
	static QString getApiAuthKey();
	static void setApiKey(const QString & apiKey);
	static void setIsApiAuthEnabled(bool isEnabled);
	static bool isExpertModeEnabled();
	static void setExpertModeEnabled(bool isEnabled);
	static bool isKeepLightsOnAfterExit();
	static void setKeepLightsOnAfterExit(bool isEnabled);
	static bool isKeepLightsOnAfterLock();
	static void setKeepLightsOnAfterLock(bool isEnabled);
	static bool isKeepLightsOnAfterSuspend();
	static void setKeepLightsOnAfterSuspend(bool isEnabled);
	static bool isPingDeviceEverySecond();
	static void setPingDeviceEverySecond(bool isEnabled);
	static bool isUpdateFirmwareMessageShown();
	static void setUpdateFirmwareMessageShown(bool isShown);
	static SupportedDevices::DeviceType getConnectedDevice();
	static void setConnectedDevice(SupportedDevices::DeviceType device);
	static QString getConnectedDeviceName();
	static void setConnectedDeviceName(const QString & deviceName);
	static QStringList getSupportedDevices();
	static QKeySequence getHotkey(const QString &actionName);
	static void setHotkey(const QString &actionName, const QKeySequence &keySequence);
	static QString getAdalightSerialPortName();
	static void setAdalightSerialPortName(const QString & port);
	static int getAdalightSerialPortBaudRate();
	static void setAdalightSerialPortBaudRate(const QString & baud);
	static QString getArdulightSerialPortName();
	static void setArdulightSerialPortName(const QString & port);
	static int getArdulightSerialPortBaudRate();
	static void setArdulightSerialPortBaudRate(const QString & baud);
	static QStringList getSupportedSerialPortBaudRates();
	static bool isConnectedDeviceUsesSerialPort();
	// [Adalight | Ardulight | Lightpack | ... | Virtual]
	static void setNumberOfLeds(SupportedDevices::DeviceType device, int numberOfLeds);
	static int getNumberOfLeds(SupportedDevices::DeviceType device);

	static void setColorSequence(SupportedDevices::DeviceType device, QString colorSequence);
	static QString getColorSequence(SupportedDevices::DeviceType device);

	// Profile
	static int getGrabSlowdown();
	static void setGrabSlowdown(int value);
	static bool isBacklightEnabled();
	static void setIsBacklightEnabled(bool isEnabled);
	static bool isGrabAvgColorsEnabled();
	static void setGrabAvgColorsEnabled(bool isEnabled);
	static int getGrabOverBrighten();
	static void setGrabOverBrighten(int value);
	static bool isGrabApplyGammaRampEnabled();
	static void setGrabApplyGammaRampEnabled(bool value);
	static bool isGrabApplyColorTemperatureEnabled();
	static void setGrabApplyColorTemperatureEnabled(bool value);
	static int getGrabColorTemperature();
	static void setGrabColorTemperature(int value);
	static double getGrabGamma();
	static void setGrabGamma(double gamma);
	static bool isSendDataOnlyIfColorsChanges();
	static void setSendDataOnlyIfColorsChanges(bool isEnabled);
	static int getLuminosityThreshold();
	static void setLuminosityThreshold(int value);
	static bool isMinimumLuminosityEnabled();
	static void setMinimumLuminosityEnabled(bool value);
	// [Device]
	static int getDeviceRefreshDelay();
	static void setDeviceRefreshDelay(int value);
	static bool isDeviceUsbPowerLedDisabled();
	static void setDeviceUsbPowerLedDisabled(bool isDisabled);
	static int getDeviceBrightness();
	static void setDeviceBrightness(int value);
	static int getDeviceSmooth();
	static void setDeviceSmooth(int value);
	static int getDeviceColorDepth();
	static void setDeviceColorDepth(int value);
	static double getDeviceGamma();
	static void setDeviceGamma(double gamma);

	static Grab::GrabberType getGrabberType();
	static void setGrabberType(Grab::GrabberType grabMode);

#ifdef D3D10_GRAB_SUPPORT
	static bool isDx1011GrabberEnabled();
	static void setDx1011GrabberEnabled(bool isEnabled);
	static bool isDx9GrabbingEnabled();
	static void setDx9GrabbingEnabled(bool isEnabled);
#endif

	static Lightpack::Mode getLightpackMode();
	static void setLightpackMode(Lightpack::Mode mode);
	static bool isMoodLampLiquidMode();
	static void setMoodLampLiquidMode(bool isLiquidMode);
	static QColor getMoodLampColor();
	static void setMoodLampColor(QColor color);
	static int getMoodLampSpeed();
	static void setMoodLampSpeed(int value);

#ifdef BASS_SOUND_SUPPORT
	static int getSoundVisualizerDevice();
	static void setSoundVisualizerDevice(int value);
	static QColor getSoundVisualizerMinColor();
	static void setSoundVisualizerMinColor(QColor color);
	static QColor getSoundVisualizerMaxColor();
	static void setSoundVisualizerMaxColor(QColor color);
	static bool isSoundVisualizerLiquidMode();
	static void setSoundVisualizerLiquidMode(bool isLiquidMode);
	static int getSoundVisualizerLiquidSpeed();
	static void setSoundVisualizerLiquidSpeed(int value);
#endif

	static QList<WBAdjustment> getLedCoefs();

	static double getLedCoefRed(int ledIndex);
	static double getLedCoefGreen(int ledIndex);
	static double getLedCoefBlue(int ledIndex);

	static void setLedCoefRed(int ledIndex, double value);
	static void setLedCoefGreen(int ledIndex, double value);
	static void setLedCoefBlue(int ledIndex, double value);

	static QSize getLedSize(int ledIndex);
	static void setLedSize(int ledIndex, QSize size);
	static QPoint getLedPosition(int ledIndex);
	static void setLedPosition(int ledIndex, QPoint position);
	static bool isLedEnabled(int ledIndex);
	static void setLedEnabled(int ledIndex, bool isEnabled);

	static bool isCheckForUpdatesEnabled();
	static void setCheckForUpdatesEnabled(bool isEnabled);
	static bool isInstallUpdatesEnabled();
	static void setInstallUpdatesEnabled(bool isEnabled);
	static QString getAutoUpdatingVersion();
	static void setAutoUpdatingVersion(const QString & version);

private:		
	static int getValidDeviceRefreshDelay(int value);
	static int getValidDeviceBrightness(int value);
	static int getValidDeviceSmooth(int value);
	static int getValidDeviceColorDepth(int value);
	static double getValidDeviceGamma(double value);
	static int getValidGrabSlowdown(int value);
	static int getValidMoodLampSpeed(int value);
	static int getValidSoundVisualizerLiquidSpeed(int value);
	static int getValidLuminosityThreshold(int value);
	static int getValidGrabOverBrighten(int value);
	static void setValidLedCoef(int ledIndex, const QString & keyCoef, double coef);
	static double getValidLedCoef(int ledIndex, const QString & keyCoef);

	static void initCurrentProfile(bool isResetDefault);
	static void initDevicesMap();

	static void migrateSettings();


public:
	static void setNewOption(const QString & name, const QVariant & value,
							bool isForceSetOption = false, QSettings * settings = m_currentProfile);
	static void setNewOptionMain(const QString & name, const QVariant & value,
								bool isForceSetOption = false);
	// forwarding to m_mainConfig object
	static void setValueMain(const QString & key, const QVariant & value);
	static QVariant valueMain(const QString & key);
	// forwarding to m_currentProfile object
	static void setValue(const QString & key, const QVariant & value);
	static QVariant value(const QString & key);

signals:
	void currentProfileNameChanged(const QString &);
	void currentProfileRemoved();
	void currentProfileInited(const QString &);
	void apiServerSettingsChanged();
	void apiKeyChanged(const QString &);
	void expertModeEnabledChanged(bool);
	void keepLightsOnAfterExitChanged(bool isEnabled);
	void keepLightsOnAfterLockChanged(bool isEnabled);
	void keepLightsOnAfterSuspendChanged(bool isEnabled);
	void pingDeviceEverySecondEnabledChanged(bool);

	void languageChanged(const QString &);
	void debugLevelChanged(int);
	void updateFirmwareMessageShownChanged(bool isShown);
	void connectedDeviceChanged(const SupportedDevices::DeviceType device);
	void hotkeyChanged(const QString &actionName, const QKeySequence & newKeySequence, const QKeySequence &oldKeySequence);
	void adalightSerialPortNameChanged(const QString & port);
	void adalightSerialPortBaudRateChanged(const QString & baud);
	void ardulightSerialPortNameChanged(const QString & port);
	void ardulightSerialPortBaudRateChanged(const QString & baud);
	void lightpackNumberOfLedsChanged(int numberOfLeds);
	void adalightNumberOfLedsChanged(int numberOfLeds);
	void ardulightNumberOfLedsChanged(int numberOfLeds);
	void virtualNumberOfLedsChanged(int numberOfLeds);
	void grabSlowdownChanged(int value);
	void backlightEnabledChanged(bool isEnabled);
	void grabAvgColorsEnabledChanged(bool isEnabled);
	void grabOverBrightenChanged(int value);
	void grabApplyGammaRampChanged(bool isEnabled);
	void grabApplyColorTemperatureChanged(bool isEnabled);
	void grabColorTemperatureChanged(int value);
	void grabGammaChanged(double value);
	void sendDataOnlyIfColorsChangesChanged(bool isEnabled);
	void luminosityThresholdChanged(int value);
	void minimumLuminosityEnabledChanged(bool value);
	void deviceRefreshDelayChanged(int value);
	void deviceUsbPowerLedDisabledChanged(bool isDisabled);
	void deviceBrightnessChanged(int value);
	void deviceSmoothChanged(int value);
	void deviceColorDepthChanged(int value);
	void deviceGammaChanged(double gamma);
	void deviceColorSequenceChanged(QString value);
	void grabberTypeChanged(const Grab::GrabberType grabMode);
#ifdef D3D10_GRAB_SUPPORT
	void dx1011GrabberEnabledChanged(bool isEnabled);
	void dx9GrabberEnabledChanged(bool isEnabled);
#endif
	void lightpackModeChanged(const Lightpack::Mode mode);
	void moodLampLiquidModeChanged(bool isLiquidMode);
	void moodLampColorChanged(const QColor color);
	void moodLampSpeedChanged(int value);
#ifdef BASS_SOUND_SUPPORT
	void soundVisualizerDeviceChanged(int value);
	void soundVisualizerMinColorChanged(const QColor color);
	void soundVisualizerMaxColorChanged(const QColor color);
	void soundVisualizerLiquidModeChanged(bool isLiquidMode);
	void soundVisualizerLiquidSpeedChanged(int value);
#endif
	void ledCoefRedChanged(int ledIndex, double value);
	void ledCoefGreenChanged(int ledIndex, double value);
	void ledCoefBlueChanged(int ledIndex, double value);
	void ledSizeChanged(int ledIndex, const QSize &size);
	void ledPositionChanged(int ledIndex, const QPoint &position);
	void ledEnabledChanged(int ledIndex, bool isEnabled);

private:
	static QMutex m_mutex; // for thread-safe access to QSettings* variables
	static QSettings * m_currentProfile; // using profile
	static QSettings * m_mainConfig;		// store last used profile name, locale and so on
	static QString m_applicationDirPath; // path to store app generated stuff
	static Settings *m_this;
	static QMap<SupportedDevices::DeviceType, QString> m_devicesTypeToNameMap;
	static QMap<SupportedDevices::DeviceType, QString> m_devicesTypeToKeyNumberOfLedsMap;
};
} /*SettingsScope*/

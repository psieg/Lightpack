/*
 * SettingsWindow.hpp
 *
 *	Created on: 26.07.2010
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

#include <QListWidgetItem>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QLabel>
#include "Settings.hpp"
#include "GrabManager.hpp"
#include "MoodLampManager.hpp"
#ifdef BASS_SOUND_SUPPORT
#include "SoundManager.hpp"
#endif
#include "ColorButton.hpp"
#include "enums.hpp"


namespace Ui {
	class SettingsWindow;
}

class GrabManager; // forward declaration
class Plugin;
class SysTrayIcon;

class SettingsWindow : public QMainWindow {
	Q_OBJECT
public:
	SettingsWindow(QWidget *parent = 0);
	SettingsWindow(bool noGUI);
	~SettingsWindow();

public:
	void startBacklight();
	void createTrayIcon();
	void connectSignalsSlots();
	QWidget* getSettingBox();

signals:
	void switchOffLeds();
	void switchOnLeds();
	void showLedWidgets(bool visible);
	void setColoredLedWidget(bool colored);
	void updateLedsColors(const QList<QRgb> &);
	void updateRefreshDelay(int value);
	void updateColorDepth(int value);
	void updateSlowdown(int value);
	void updateGamma(double value);
	void updateBrightness(int percent);
	void requestFirmwareVersion();
#ifdef BASS_SOUND_SUPPORT
	void requestSoundVizDevices();
#endif
	void recreateLedDevice();
	void resultBacklightStatus(Backlight::Status);
	void backlightStatusChanged(Backlight::Status);
	void enableApiServer(bool isEnabled);
	void enableApiAuth(bool isEnabled);
	void updateApiPort(int port);
	void updateApiKey(QString key);
	void updateApiDeviceNumberOfLeds(int value);
	void reloadPlugins();

public slots:
	void ledDeviceOpenSuccess(bool isSuccess);
	void ledDeviceCallSuccess(bool isSuccess);
	void ledDeviceFirmwareVersionResult(const QString & fwVersion);
	void ledDeviceFirmwareVersionUnofficialResult(const int version);
	void refreshAmbilightEvaluated(double updateResultMs);
	void updateUiFromSettings();

	void setDeviceLockViaAPI(DeviceLocked::DeviceLockStatus status,	QList<QString> modules);
	void setBacklightStatus(Backlight::Status);
	void setModeChanged(Lightpack::Mode);
	void backlightOn(); /* using in actions */
	void backlightOff(); /* using in actions */
	void profilesLoadAll();
	void profileSwitch(const QString & configName);
	void handleProfileLoaded(const QString & configName);
	void profileSwitchCombobox(QString profile);
	void updateVirtualLedsColors(const QList<QRgb> & colors);
	void requestBacklightStatus();
	void onApiServer_ErrorOnStartListening(QString errorMessage);
	void onPingDeviceEverySecond_Toggled(bool state);
	void processMessage(const QString &message);

#ifdef BASS_SOUND_SUPPORT
	void updateAvailableSoundVizDevices(const QList<SoundManagerDeviceInfo> & devices, int recommended);
#endif

	void updatePlugin(QList<Plugin*> plugins);

	void onFocus();
	void onBlur();
	void quit(); /* using in actions */
	void showSettings(); /* using in actions */
	void hideSettings(); /* used in closeEvent(..) */
	void toggleSettings(); /* using in iconActivated(..) */

protected:
	virtual void changeEvent(QEvent *e);
	virtual void closeEvent(QCloseEvent *event);

private slots:
	void onLightpackModes_currentIndexChanged(int index);
	void onLightpackModeChanged(Lightpack::Mode);
	void onMoodLampColor_changed(QColor color);
	void onMoodLampSpeed_valueChanged(int value);
	void onMoodLampLiquidMode_Toggled(bool isLiquidMode);
#ifdef BASS_SOUND_SUPPORT
	void onSoundVizDevice_currentIndexChanged(int index);
	void onSoundVizMinColor_changed(QColor color);
	void onSoundVizMaxColor_changed(QColor color);
	void onSoundVizLiquidMode_Toggled(bool isLiquidMode);
	void onSoundVizLiquidSpeed_valueChanged(int value);
#endif
	void showAbout(); /* using in actions */
	void onPostInit();
	void checkOutdatedGrabber();

	void scrollThanks();

	void updateTrayAndActionStates();

	void changePage(int page);

	void toggleBacklight();
	void nextProfile();
	void prevProfile();

	void onGrabberChanged();
	void onGrabSlowdown_valueChanged(int value);
	void onGrabIsAvgColors_toggled(bool state);
	void onGrabOverBrighten_valueChanged(int value);
	void onGrabApplyGammaRamp_toggled(bool state);
	void onGrabApplyColorTemperature_toggled(bool state);
	void onGrabColorTemperature_valueChanged(int value);
	void onGrabGamma_valueChanged(double value);
	void onSliderGrabGamma_valueChanged(int value);
	void onLuminosityThreshold_valueChanged(int value);
	void onMinimumLumosity_toggled(bool value);

	void onDeviceRefreshDelay_valueChanged(int value);
	void onDisableUsbPowerLed_toggled(bool state);
	void onDeviceSmooth_valueChanged(int value);
	void onDeviceBrightness_valueChanged(int value);
	void onDeviceColorDepth_valueChanged(int value);
	void onDeviceGammaCorrection_valueChanged(double value);
	void onSliderDeviceGammaCorrection_valueChanged(int value);
	void onDeviceSendDataOnlyIfColorsChanged_toggled(bool state);
	void onDx1011CaptureEnabledChanged(bool isEnabled);
	void onDx9CaptureEnabledChanged(bool isEnabled);

	void onDontShowLedWidgets_Toggled(bool checked);
	void onSetColoredLedWidgets(bool checked);
	void onSetWhiteLedWidgets(bool checked);

	void openCurrentProfile();

	void profileRename();
	void profileTraySwitch(const QString &profileName);
	void profileNew();
	void profileResetToDefaultCurrent();
	void profileDeleteCurrent();
	void settingsProfileChanged_UpdateUI(const QString &profileName);

	void loadTranslation(const QString & language);

	void onExpertModeEnabled_Toggled(bool isEnabled);
	void onEnableApi_Toggled(bool isEnabled);
	void onListenOnlyOnLoInterface_Toggled(bool localOnly);
	void onApiKey_EditingFinished();
	void onGenerateNewApiKey_Clicked();
	void onSetApiPort_Clicked();
	void onLoggingLevel_valueChanged(int value);

	void on_pushButton_LightpackSmoothnessHelp_clicked();
	void on_pushButton_LightpackColorDepthHelp_clicked();
	void on_pushButton_LightpackRefreshDelayHelp_clicked();

	void on_pushButton_GammaCorrectionHelp_clicked();

	void on_pushButton_lumosityThresholdHelp_clicked();

	void on_pushButton_grabApplyColorTemperatureHelp_clicked();
	void on_pushButton_grabGammaHelp_clicked();

	void on_pushButton_grabOverBrightenHelp_clicked();

	void on_pushButton_AllPluginsHelp_clicked();

	void pluginSwitch(int index);
	void on_list_Plugins_itemClicked(QListWidgetItem*);
	void on_pushButton_ReloadPlugins_clicked();
	void MoveUpPlugin();
	void MoveDownPlugin();

	void onKeepLightsAfterExit_Toggled(bool isEnabled);
	void onKeepLightsAfterLock_Toggled(bool isEnabled);
	void onKeepLightsAfterSuspend_Toggled(bool isEnabled);

	void on_pbRunConfigurationWizard_clicked();

	void onCheckBox_checkForUpdates_Toggled(bool isEnabled);
	void onCheckBox_installUpdates_Toggled(bool isEnabled);

private:
	void updateExpertModeWidgetsVisibility();
	void updateDeviceTabWidgetsVisibility();
	void setDeviceTabWidgetsVisibility(DeviceTab::Options options);
	void syncLedDeviceWithSettingsWindow();
	int getLigtpackFirmwareVersionMajor();

	void updateStatusBar();

	void initLanguages();
	void initPixmapCache();

	void openFile(const QString &filePath);

	void initGrabbersRadioButtonsVisibility();
	void initVirtualLeds(int ledsCount);

	void adjustSizeAndMoveCenter();

	void setupHotkeys();
	void registerHotkey(const QString &actionName, const QString &description, const QString &hotkey);

	void setFirmwareVersion(const QString &firmwareVersion);
	void versionsUpdate();

	void savePriorityPlugin();
	void showHelpOf(QObject *object);
	QString getSlotName(const QString &actionName);
	QString getPluginName(const Plugin *plugin) const;


private:
	Ui::SettingsWindow *ui;
	// Main backlight status for all modes (Grab, MoodLamp, etc.)
	Backlight::Status m_backlightStatus;
	DeviceLocked::DeviceLockStatus m_deviceLockStatus;
	QList<QString> m_deviceLockKey;
	QString m_deviceLockModule;

	QTimer m_smoothScrollTimer;

	Grab::GrabberType getSelectedGrabberType();

	bool isDx1011CaptureEnabled();

	QList<QLabel *> m_labelsGrabbedColors;


	bool m_isHotkeySelectionChanging;
	SysTrayIcon *m_trayIcon;

	QLabel *m_labelStatusIcon;
	QLabel *labelProfile;
	QLabel *labelDevice;
	QLabel *labelFPS;

	QCache<QString, QPixmap> m_pixmapCache;

	QTranslator *m_translator;

	QString m_deviceFirmwareVersion;
	static const QString DeviceFirmvareVersionUndef;
	static const QString LightpackDownloadsPageUrl;
	static const int GrabModeIndex;
	static const int MoodLampModeIndex;
#ifdef BASS_SOUND_SUPPORT
	static const int SoundVisualizeModeIndex;
#endif

	QString fimwareVersion;

	QList<Plugin*> _plugins;
	static bool toPriority(Plugin* s1 , Plugin* s2 );

	bool updatingFromSettings = false;
};


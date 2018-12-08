/*
 * SettingsWindow.cpp
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

#include <QtAlgorithms>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDesktopWidget>
#include "LightpackApplication.hpp"

#include "SettingsWindow.hpp"
#include "ui_SettingsWindow.h"

#include "Settings.hpp"
#include "ColorButton.hpp"
#include "LedDeviceManager.hpp"
#include "enums.hpp"
#include "debug.h"
#include "Plugin.hpp"
#include "systrayicon/SysTrayIcon.hpp"
#include "version.h"
#include <QStringBuilder>
#include <QScrollBar>
#include <QMessageBox>


using namespace SettingsScope;

// ----------------------------------------------------------------------------
// Lightpack settings window
// ----------------------------------------------------------------------------

const QString SettingsWindow::DeviceFirmvareVersionUndef = "undef";
const QString SettingsWindow::LightpackDownloadsPageUrl = "http://code.google.com/p/lightpack/downloads/list";

// Indexes of supported modes listed in ui->comboBox_Modes and ui->stackedWidget_Modes
const int SettingsWindow::GrabModeIndex = 0;
const int SettingsWindow::MoodLampModeIndex = 1;
#ifdef BASS_SOUND_SUPPORT
const int SettingsWindow::SoundVisualizeModeIndex = 2;
#endif

SettingsWindow::SettingsWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::SettingsWindow),
	m_deviceFirmwareVersion(DeviceFirmvareVersionUndef)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "thread id: " << this->thread()->currentThreadId();

	m_trayIcon = NULL;

	ui->setupUi(this);

	ui->tabWidget->setCurrentIndex(0);

	setWindowFlags(Qt::Window |
					Qt::CustomizeWindowHint |
					Qt::WindowCloseButtonHint );
	setFocus(Qt::OtherFocusReason);

#ifdef Q_OS_LINUX
	ui->listWidget->setSpacing(0);
	ui->listWidget->setGridSize(QSize(115, 85));
#endif

	// Check windows reserved symbols in profile input name
	QRegExpValidator *validatorProfileName = new QRegExpValidator(QRegExp("[^<>:\"/\\|?*]*"), this);
	ui->comboBox_Profiles->lineEdit()->setValidator(validatorProfileName);

	QRegExpValidator *validatorApiKey = new QRegExpValidator(QRegExp("[a-zA-Z0-9{}_-]*"), this);
	ui->lineEdit_ApiKey->setValidator(validatorApiKey);

	// hide main tabbar
	QTabBar* tabBar=ui->tabWidget->findChild<QTabBar*>();
	tabBar->hide();
	// hide plugin settings tabbar
	tabBar=ui->tabDevices->findChild<QTabBar*>();
	tabBar->hide();

	initPixmapCache();


	m_labelStatusIcon = new QLabel(statusBar());
	m_labelStatusIcon->setStyleSheet("QLabel{margin-right: .5em}");
	m_labelStatusIcon->setPixmap(Settings::isBacklightEnabled() ? *(m_pixmapCache["on16"]) : *(m_pixmapCache["off16"]));
	labelProfile = new QLabel(statusBar());
	labelProfile->setStyleSheet("margin-left:1em");
	labelDevice = new QLabel(statusBar());
	labelFPS	= new QLabel(statusBar());

	statusBar()->setStyleSheet("QStatusBar{border-top: 1px solid; border-color: palette(midlight);} QLabel{margin:0.2em}");
	statusBar()->setSizeGripEnabled(false);
	statusBar()->addWidget(labelProfile, 4);
	statusBar()->addWidget(labelDevice, 4);
	statusBar()->addWidget(labelFPS, 4);
	statusBar()->addWidget(m_labelStatusIcon, 0);

	ui->checkBox_DisableUsbPowerLed->setVisible(false);

	updateStatusBar();

#ifdef BASS_SOUND_SUPPORT
	ui->label_licenseAndCredits->setText(ui->label_licenseAndCredits->text() + tr(" The sound visualizer uses the <a href=\"http://un4seen.com/\"><span style=\" text-decoration: underline; color:#0000ff;\">BASS</span></a> library."));
#else
	ui->comboBox_LightpackModes->removeItem(2);
#endif

#ifndef Q_OS_WIN
	ui->checkBox_GrabApplyGammaRamp->setVisible(false);
	ui->checkBox_installUpdates->setVisible(false);
#endif

	initGrabbersRadioButtonsVisibility();
	initLanguages();
	initVirtualLeds(Settings::getNumberOfLeds(SupportedDevices::DeviceTypeVirtual));

	loadTranslation(Settings::getLanguage());

	if (Settings::isBacklightEnabled())
	{
		m_backlightStatus = Backlight::StatusOn;
	} else {
		m_backlightStatus = Backlight::StatusOff;
	}

	emit backlightStatusChanged(m_backlightStatus);

	m_deviceLockStatus = DeviceLocked::Unlocked;

	adjustSize();
	resize(minimumSize());

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "initialized";
}

void SettingsWindow::changePage(int page)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << page;

	ui->tabWidget->setCurrentIndex(page);
	if (page == 5) {
		ui->textBrowser->verticalScrollBar()->setValue(0);
		m_smoothScrollTimer.setInterval(100);
		m_smoothScrollTimer.start();
	} else {
		m_smoothScrollTimer.stop();
	}

}

SettingsWindow::~SettingsWindow()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (m_trayIcon)
		delete m_trayIcon;

	delete ui;
}

void SettingsWindow::connectSignalsSlots()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

//	if (m_trayIcon!=NULL)
//	{
//		connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayIcon_Activated(QSystemTrayIcon::ActivationReason)));
//		connect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(onTrayIcon_MessageClicked()));
//	}
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "tr";

	connect(ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(changePage(int)));
	connect(ui->spinBox_GrabSlowdown, SIGNAL(valueChanged(int)), this, SLOT(onGrabSlowdown_valueChanged(int)));
	connect(ui->spinBox_LuminosityThreshold, SIGNAL(valueChanged(int)), this, SLOT(onLuminosityThreshold_valueChanged(int)));
	connect(ui->radioButton_MinimumLuminosity, SIGNAL(toggled(bool)), this, SLOT(onMinimumLumosity_toggled(bool)));
	connect(ui->radioButton_LuminosityDeadZone, SIGNAL(toggled(bool)), this, SLOT(onMinimumLumosity_toggled(bool)));
	connect(ui->checkBox_GrabIsAvgColors, SIGNAL(toggled(bool)), this, SLOT(onGrabIsAvgColors_toggled(bool)));
	connect(ui->spinBox_GrabOverBrighten, SIGNAL(valueChanged(int)), this, SLOT(onGrabOverBrighten_valueChanged(int)));
	connect(ui->checkBox_GrabApplyGammaRamp, SIGNAL(toggled(bool)), this, SLOT(onGrabApplyGammaRamp_toggled(bool)));

	connect(ui->radioButton_GrabWidgetsDontShow, SIGNAL(toggled(bool)), this, SLOT( onDontShowLedWidgets_Toggled(bool)));
	connect(ui->radioButton_Colored, SIGNAL(toggled(bool)), this, SLOT(onSetColoredLedWidgets(bool)));
	connect(ui->radioButton_White, SIGNAL(toggled(bool)), this, SLOT(onSetWhiteLedWidgets(bool)));

	connect(ui->radioButton_LiquidColorMoodLampMode, SIGNAL(toggled(bool)), this, SLOT(onMoodLampLiquidMode_Toggled(bool)));
	connect(ui->horizontalSlider_MoodLampSpeed, SIGNAL(valueChanged(int)), this, SLOT(onMoodLampSpeed_valueChanged(int)));

	// Main options
	connect(ui->comboBox_LightpackModes, SIGNAL(currentIndexChanged(int)), this, SLOT(onLightpackModes_currentIndexChanged(int)));
	connect(ui->comboBox_Language, SIGNAL(activated(QString)), this, SLOT(loadTranslation(QString)));
	connect(ui->pushButton_EnableDisableDevice, SIGNAL(clicked()), this, SLOT(toggleBacklight()));

	// Device options
	connect(ui->spinBox_DeviceRefreshDelay, SIGNAL(valueChanged(int)), this, SLOT(onDeviceRefreshDelay_valueChanged(int)));
	connect(ui->checkBox_DisableUsbPowerLed, SIGNAL(toggled(bool)), this, SLOT(onDisableUsbPowerLed_toggled(bool)));
	connect(ui->spinBox_DeviceSmooth, SIGNAL(valueChanged(int)), this, SLOT(onDeviceSmooth_valueChanged(int)));
	connect(ui->spinBox_DeviceBrightness, SIGNAL(valueChanged(int)), this, SLOT(onDeviceBrightness_valueChanged(int)));
	connect(ui->spinBox_DeviceColorDepth, SIGNAL(valueChanged(int)), this, SLOT(onDeviceColorDepth_valueChanged(int)));
	connect(ui->doubleSpinBox_DeviceGamma, SIGNAL(valueChanged(double)), this, SLOT(onDeviceGammaCorrection_valueChanged(double)));
	connect(ui->horizontalSlider_GammaCorrection, SIGNAL(valueChanged(int)), this, SLOT(onSliderDeviceGammaCorrection_valueChanged(int)));
	connect(ui->checkBox_SendDataOnlyIfColorsChanges, SIGNAL(toggled(bool)), this, SLOT(onDeviceSendDataOnlyIfColorsChanged_toggled(bool)));

	// Open Settings file
	connect(ui->commandLinkButton_OpenSettings, SIGNAL(clicked()), this, SLOT(openCurrentProfile()));

	// Connect profile signals to this slots
	connect(ui->comboBox_Profiles->lineEdit(), SIGNAL(editingFinished()) /* or returnPressed() */, this, SLOT(profileRename()));
	connect(ui->comboBox_Profiles, SIGNAL(currentIndexChanged(QString)), this, SLOT(profileSwitch(QString)));

	connect(Settings::settingsSingleton(), SIGNAL(currentProfileInited(const QString &)), this, SLOT(handleProfileLoaded(QString)), Qt::QueuedConnection);

	// connect(Settings::settingsSingleton(), SIGNAL(hotkeyChanged(QString,QKeySequence,QKeySequence)), this, SLOT(onHotkeyChanged(QString,QKeySequence,QKeySequence)));
	connect(Settings::settingsSingleton(), SIGNAL(lightpackModeChanged(Lightpack::Mode)), this, SLOT(onLightpackModeChanged(Lightpack::Mode)));

	connect(ui->pushButton_ProfileNew, SIGNAL(clicked()), this, SLOT(profileNew()));
	connect(ui->pushButton_ProfileResetToDefault, SIGNAL(clicked()), this, SLOT(profileResetToDefaultCurrent()));
	connect(ui->pushButton_DeleteProfile, SIGNAL(clicked()), this, SLOT(profileDeleteCurrent()));

	connect(ui->pushButton_SelectColorMoodLamp, SIGNAL(colorChanged(QColor)), this, SLOT(onMoodLampColor_changed(QColor)));
#ifdef BASS_SOUND_SUPPORT
	connect(ui->comboBox_SoundVizDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(onSoundVizDevice_currentIndexChanged(int)));
	connect(ui->pushButton_SelectColorSoundVizMin, SIGNAL(colorChanged(QColor)), this, SLOT(onSoundVizMinColor_changed(QColor)));
	connect(ui->pushButton_SelectColorSoundVizMax, SIGNAL(colorChanged(QColor)), this, SLOT(onSoundVizMaxColor_changed(QColor)));
	connect(ui->radioButton_SoundVizLiquidMode, SIGNAL(toggled(bool)), this, SLOT(onSoundVizLiquidMode_Toggled(bool)));
	connect(ui->horizontalSlider_SoundVizLiquidSpeed, SIGNAL(valueChanged(int)), this, SLOT(onSoundVizLiquidSpeed_valueChanged(int)));
#endif
	connect(ui->checkBox_ExpertModeEnabled, SIGNAL(toggled(bool)), this, SLOT(onExpertModeEnabled_Toggled(bool)));
	connect(ui->checkBox_KeepLightsOnAfterExit, SIGNAL(toggled(bool)), this, SLOT(onKeepLightsAfterExit_Toggled(bool)));
	connect(ui->checkBox_KeepLightsOnAfterLockComputer, SIGNAL(toggled(bool)), this, SLOT(onKeepLightsAfterLock_Toggled(bool)));
	connect(ui->checkBox_KeepLightsOnAfterSuspend, SIGNAL(toggled(bool)), this, SLOT(onKeepLightsAfterSuspend_Toggled(bool)));

	// Dev tab
#ifdef WINAPI_GRAB_SUPPORT
	connect(ui->radioButton_GrabWinAPI, SIGNAL(toggled(bool)), this, SLOT(onGrabberChanged()));
#endif
#ifdef DDUPL_GRAB_SUPPORT
	connect(ui->radioButton_GrabDDupl, SIGNAL(toggled(bool)), this, SLOT(onGrabberChanged()));
#endif
#ifdef X11_GRAB_SUPPORT
	connect(ui->radioButton_GrabX11, SIGNAL(toggled(bool)), this, SLOT(onGrabberChanged()));
#endif
#ifdef MAC_OS_CG_GRAB_SUPPORT
	connect(ui->radioButton_GrabMacCoreGraphics, SIGNAL(toggled(bool)), this, SLOT(onGrabberChanged()));
#endif
#ifdef D3D10_GRAB_SUPPORT
	connect(ui->checkBox_EnableDx1011Capture, SIGNAL(toggled(bool)), this, SLOT(onDx1011CaptureEnabledChanged(bool)));
	connect(ui->checkBox_EnableDx9Capture, SIGNAL(toggled(bool)), this, SLOT(onDx9CaptureEnabledChanged(bool)));
#endif


	// Dev tab configure API (port, apikey)
	connect(ui->groupBox_Api, SIGNAL(toggled(bool)), this, SLOT(onEnableApi_Toggled(bool)));
	connect(ui->checkBox_listenOnlyOnLoInterface, SIGNAL(toggled(bool)), this, SLOT(onListenOnlyOnLoInterface_Toggled(bool)));
	connect(ui->lineEdit_ApiPort, SIGNAL(editingFinished()), this, SLOT(onSetApiPort_Clicked()));
	//connect(ui->checkBox_IsApiAuthEnabled, SIGNAL(toggled(bool)), this, SLOT(onIsApiAuthEnabled_Toggled(bool)));
	connect(ui->pushButton_GenerateNewApiKey, SIGNAL(clicked()), this, SLOT(onGenerateNewApiKey_Clicked()));
	connect(ui->lineEdit_ApiKey, SIGNAL(editingFinished()), this, SLOT(onApiKey_EditingFinished()));

	connect(ui->spinBox_LoggingLevel, SIGNAL(valueChanged(int)), this, SLOT(onLoggingLevel_valueChanged(int)));
	connect(ui->checkBox_PingDeviceEverySecond, SIGNAL(toggled(bool)), this, SLOT(onPingDeviceEverySecond_Toggled(bool)));

	//Plugins
	//	connected during setupUi by name:
	//	connect(ui->list_Plugins,SIGNAL(currentRowChanged(int)),this,SLOT(on_list_Plugins_itemClicked(QListWidgetItem *)));
	connect(ui->pushButton_UpPriority, SIGNAL(clicked()), this, SLOT(MoveUpPlugin()));
	connect(ui->pushButton_DownPriority, SIGNAL(clicked()), this, SLOT(MoveDownPlugin()));

	// About page
	connect(&m_smoothScrollTimer, SIGNAL(timeout()), this, SLOT(scrollThanks()));
	connect(ui->checkBox_checkForUpdates, SIGNAL(toggled(bool)), this, SLOT(onCheckBox_checkForUpdates_Toggled(bool)));
	connect(ui->checkBox_installUpdates, SIGNAL(toggled(bool)), this, SLOT(onCheckBox_installUpdates_Toggled(bool)));
}

// ----------------------------------------------------------------------------
// Events
// ----------------------------------------------------------------------------

void SettingsWindow::changeEvent(QEvent *e)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << e->type();

	int currentPage = 0;
	QListWidgetItem * item = NULL;
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:

		currentPage = ui->listWidget->currentRow();
		updatingFromSettings = true;
		ui->retranslateUi(this);
		updatingFromSettings = false;
		if (m_trayIcon)
			m_trayIcon->retranslateUi();

		setWindowTitle(tr("Prismatik: %1").arg(ui->comboBox_Profiles->lineEdit()->text()));

		ui->listWidget->addItem("dirty hack");
		item = ui->listWidget->takeItem(ui->listWidget->count()-1);
		delete item;
		ui->listWidget->setCurrentRow(currentPage);


		ui->comboBox_Language->setItemText(0, tr("System default"));

		updateStatusBar();

		break;
	default:
		break;
	}
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if (m_trayIcon && m_trayIcon->isVisible()) {
		// Just hide settings
		hideSettings();
		event->ignore();
	}
	else {
		// terminate application if we're running in "trayless" mode
		QApplication::quit();
	}
}

void SettingsWindow::onFocus()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (!ui->radioButton_GrabWidgetsDontShow->isChecked() && ui->comboBox_LightpackModes->currentIndex() == GrabModeIndex) {
		emit showLedWidgets(true);
	}
}

void SettingsWindow::onBlur()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	emit showLedWidgets(false);
}

void SettingsWindow::onExpertModeEnabled_Toggled(bool isEnabled)
{
	Settings::setExpertModeEnabled(isEnabled);
	updateExpertModeWidgetsVisibility();
}

void SettingsWindow::updateExpertModeWidgetsVisibility()
{	
	if(Settings::isExpertModeEnabled()) {
		ui->listWidget->setItemHidden(ui->listWidget->item(4),false);
	} else {
		ui->listWidget->setItemHidden(ui->listWidget->item(4),true);
	}

	updateDeviceTabWidgetsVisibility();
}

void SettingsWindow::updateStatusBar() {
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	this->labelProfile->setText(tr("Profile: %1").arg(Settings::getCurrentProfileName()));
	this->labelDevice->setText(tr("Device: %1").arg(Settings::getConnectedDeviceName()));
	this->labelFPS->setText(tr("FPS: %1").arg(""));
}

void SettingsWindow::updateDeviceTabWidgetsVisibility()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	SupportedDevices::DeviceType connectedDevice = Settings::getConnectedDevice();

	switch (connectedDevice)
	{
	case SupportedDevices::DeviceTypeVirtual:
		ui->tabDevices->show();
		ui->tabDevices->setCurrentWidget(ui->tabDeviceVirtual);
		// Sync Virtual Leds count with NumberOfLeds field
		initVirtualLeds(Settings::getNumberOfLeds(SupportedDevices::DeviceTypeVirtual));
		break;

	case SupportedDevices::DeviceTypeLightpack:
		ui->tabDevices->show();
		ui->tabDevices->setCurrentWidget(ui->tabDeviceLightpack);
		// Sync Virtual Leds count with NumberOfLeds field
		break;

	default:
		ui->tabDevices->hide();
//		qCritical() << Q_FUNC_INFO << "Fail. Unknown connectedDevice ==" << connectedDevice;
		break;
	}
	setDeviceTabWidgetsVisibility(DeviceTab::Lightpack);

}

void SettingsWindow::setDeviceTabWidgetsVisibility(DeviceTab::Options options)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << options;

#ifdef QT_NO_DEBUG
	int majorVersion = getLigtpackFirmwareVersionMajor();
	bool isShowOldSettings = (majorVersion == 4 || majorVersion == 5) && Settings::isExpertModeEnabled();

	// Show color depth only if lightpack hw4.x or hw5.x
	ui->label_DeviceColorDepth->setVisible(isShowOldSettings);
	ui->horizontalSlider_DeviceColorDepth->setVisible(isShowOldSettings);
	ui->spinBox_DeviceColorDepth->setVisible(isShowOldSettings);
	ui->pushButton_LightpackColorDepthHelp->setVisible(isShowOldSettings);

	ui->label_DeviceRefreshDelay->setVisible(isShowOldSettings);
	ui->horizontalSlider_DeviceRefreshDelay->setVisible(isShowOldSettings);
	ui->spinBox_DeviceRefreshDelay->setVisible(isShowOldSettings);
	ui->pushButton_LightpackRefreshDelayHelp->setVisible(isShowOldSettings);
#endif
}

void SettingsWindow::syncLedDeviceWithSettingsWindow()
{
	emit updateBrightness(Settings::getDeviceBrightness());
	emit updateGamma(Settings::getDeviceGamma());
}

int SettingsWindow::getLigtpackFirmwareVersionMajor()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	if (Settings::getConnectedDevice() != SupportedDevices::DeviceTypeLightpack)
		return -1;

	if (m_deviceFirmwareVersion == DeviceFirmvareVersionUndef)
		return -1;

	bool ok = false;
	double version = m_deviceFirmwareVersion.toDouble(&ok);

	if (!ok)
	{
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "Convert to double fail. Device firmware version =" << m_deviceFirmwareVersion;
		return -1;
	}

	int majorVersion = (int)version;

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Prismatik major version:" << majorVersion;

	return majorVersion;
}

void SettingsWindow::onPostInit() {
	updateUiFromSettings();
	this->requestFirmwareVersion();
#ifdef BASS_SOUND_SUPPORT
	this->requestSoundVizDevices();
#endif

	if (m_trayIcon) {
		bool updateJustFailed = false;
		if (!Settings::getAutoUpdatingVersion().isEmpty()) {
			if (Settings::getAutoUpdatingVersion() != VERSION_STR) {
				m_trayIcon->showMessage(tr("Prismatik was updated"), tr("Successfully updated to version %1.").arg(VERSION_STR));
			} else {
				m_trayIcon->showMessage(
					tr("Prismatik update failed"), 
					tr("There was a problem installing the update. You are still on version %1.").arg(VERSION_STR),
					QSystemTrayIcon::Critical);
				updateJustFailed = true;
			}
			Settings::setAutoUpdatingVersion("");
		}

		if (Settings::isCheckForUpdatesEnabled() && !updateJustFailed)
			QTimer::singleShot(10000, m_trayIcon, SLOT(checkUpdate()));
	}


	QTimer::singleShot(50, this, SLOT(checkOutdatedGrabber()));
}

void SettingsWindow::checkOutdatedGrabber() {
#ifdef Q_OS_WIN
	if ((QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS10 || QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS8_1 || QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS8)
		&& Settings::getGrabberType() == Grab::GrabberTypeWinAPI
		&& !Settings::isExpertModeEnabled()) {
		if (QMessageBox::warning(
			this,
			tr("Prismatik Grabber Update"),
			tr("The profile '") + Settings::getCurrentProfileName() + tr("' is using the outdated WinAPI grabber.\n")
			+ tr("Do you want to switch to the new Desktop Duplication grabber?\nNote: You can disable this message by enabling expert mode."),
			QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No)
			== QMessageBox::StandardButton::Yes) {
			ui->radioButton_GrabDDupl->setChecked(true);
		}
	}
#endif
}

void SettingsWindow::onEnableApi_Toggled(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << isEnabled;

	Settings::setIsApiEnabled(isEnabled);

}

void SettingsWindow::onListenOnlyOnLoInterface_Toggled(bool localOnly)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << localOnly;
	Settings::setListenOnlyOnLoInterface(localOnly);
}

void SettingsWindow::onApiKey_EditingFinished()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString apikey = ui->lineEdit_ApiKey->text();

	Settings::setApiKey(apikey);
}

void SettingsWindow::onSetApiPort_Clicked()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << ui->lineEdit_ApiPort->text();

	bool ok;
	int port = ui->lineEdit_ApiPort->text().toInt(&ok);

	if (ok)
	{
		Settings::setApiPort(port);
		emit updateApiPort(port);

		ui->lineEdit_ApiPort->setStyleSheet(this->styleSheet());
		ui->lineEdit_ApiPort->setToolTip("");
	} else {
		QString errorMessage = "Convert to 'int' fail";

		ui->lineEdit_ApiPort->setStyleSheet("background-color:red;");
		ui->lineEdit_ApiPort->setToolTip(errorMessage);

		qWarning() << Q_FUNC_INFO << errorMessage << "port:" << ui->lineEdit_ApiPort->text();
	}
}

void SettingsWindow::onGenerateNewApiKey_Clicked()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString generatedApiKey = QUuid::createUuid().toString();

	ui->lineEdit_ApiKey->setText(generatedApiKey);

	Settings::setApiKey(generatedApiKey);
}

void SettingsWindow::onLoggingLevel_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	// WARNING: Multithreading bug here with g_debugLevel
	g_debugLevel = value;

	Settings::setDebugLevel(value);
}

void SettingsWindow::setDeviceLockViaAPI(DeviceLocked::DeviceLockStatus status,	QList<QString> modules)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << status;
	m_deviceLockStatus = status;
	m_deviceLockKey = modules;


	if (m_deviceLockStatus == DeviceLocked::Unlocked)
	{
		syncLedDeviceWithSettingsWindow();
	}

	startBacklight();
}

void SettingsWindow::setModeChanged(Lightpack::Mode mode)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << mode;
	updateUiFromSettings();
}

void SettingsWindow::setBacklightStatus(Backlight::Status status)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if (m_backlightStatus != Backlight::StatusDeviceError
			|| status == Backlight::StatusOff)
	{
		m_backlightStatus = status;
		emit backlightStatusChanged(m_backlightStatus);
	}

	startBacklight();
}

void SettingsWindow::backlightOn()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_backlightStatus = Backlight::StatusOn;
	emit backlightStatusChanged(m_backlightStatus);
	startBacklight();
}

void SettingsWindow::backlightOff()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_backlightStatus = Backlight::StatusOff;
	emit backlightStatusChanged(m_backlightStatus);
	startBacklight();
}

void SettingsWindow::toggleBacklight()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	switch (m_backlightStatus)
	{
	case Backlight::StatusOn:
		m_backlightStatus = Backlight::StatusOff;
		break;
	case Backlight::StatusOff:
		m_backlightStatus = Backlight::StatusOn;
		break;
	case Backlight::StatusDeviceError:
		m_backlightStatus = Backlight::StatusOff;
		break;
	default:
		qWarning() << Q_FUNC_INFO << "m_backlightStatus contains crap =" << m_backlightStatus;
		break;
	}

	emit backlightStatusChanged(m_backlightStatus);

	startBacklight();
}

void SettingsWindow::startBacklight()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "m_backlightStatus =" << m_backlightStatus
					<< "m_deviceLockStatus =" << m_deviceLockStatus;

	if(ui->list_Plugins->count()>0)
		{
			int count = ui->list_Plugins->count();
			for(int index = 0; index < count; index++)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "check session key";
				QListWidgetItem * item = ui->list_Plugins->item(index);
				int indexPlugin =item->data(Qt::UserRole).toUInt();
				QString key = _plugins[indexPlugin]->Guid();
				if (m_deviceLockKey.contains(key))
				{
					if (m_deviceLockStatus != DeviceLocked::Api	&& m_deviceLockKey.indexOf(key)==0)
						m_deviceLockModule = _plugins[indexPlugin]->Name();
					if (Settings::isExpertModeEnabled())
						item->setText(getPluginName(_plugins[indexPlugin])+" (Lock)");
				}
				else
					item->setText(getPluginName(_plugins[indexPlugin]));
			}
		}


	if (m_deviceLockKey.count()==0)
		m_deviceLockModule = "";

	updateTrayAndActionStates();
}

void SettingsWindow::nextProfile()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	QStringList profiles = Settings::findAllProfiles();
	QString profile = Settings::getCurrentProfileName();

	int curIndex = profiles.indexOf(profile);
	int newIndex = (curIndex == profiles.count() - 1) ? 0 : curIndex + 1;

	Settings::loadOrCreateProfile(profiles[newIndex]);
}

void SettingsWindow::prevProfile()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	QStringList profiles = Settings::findAllProfiles();
	QString profile = Settings::getCurrentProfileName();

	int curIndex = profiles.indexOf(profile);
	int newIndex = (curIndex == 0) ? profiles.count() - 1 : curIndex - 1;

	Settings::loadOrCreateProfile(profiles[newIndex]);
}


void SettingsWindow::updateTrayAndActionStates()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	if (m_trayIcon== NULL) return;

	switch (m_backlightStatus)
	{
	case Backlight::StatusOn:
		ui->pushButton_EnableDisableDevice->setIcon(QIcon(*m_pixmapCache["off16"]));
		ui->pushButton_EnableDisableDevice->setText("	" + tr("Turn lights OFF"));

		if (m_deviceLockStatus == DeviceLocked::Api)
		{
			m_labelStatusIcon->setPixmap(*m_pixmapCache["lock16"]);
			if (m_trayIcon)
				m_trayIcon->setStatus(SysTrayIcon::StatusLockedByApi);
		} else
			if (m_deviceLockStatus == DeviceLocked::Plugin)
			{
				m_labelStatusIcon->setPixmap(*m_pixmapCache["lock16"]);
				if (m_trayIcon)
					m_trayIcon->setStatus(SysTrayIcon::StatusLockedByPlugin, &m_deviceLockModule);
			} else
				if (m_deviceLockStatus == DeviceLocked::ApiPersist)
				{
					m_labelStatusIcon->setPixmap(*m_pixmapCache["persist16"]);
					if (m_trayIcon)
						m_trayIcon->setStatus(SysTrayIcon::StatusApiPersist);
				} else {
						m_labelStatusIcon->setPixmap(*m_pixmapCache["on16"]);
						if (m_trayIcon)
							m_trayIcon->setStatus(SysTrayIcon::StatusOn);
				}
		break;

	case Backlight::StatusOff:
		m_labelStatusIcon->setPixmap(*m_pixmapCache["off16"]);
		ui->pushButton_EnableDisableDevice->setIcon(QIcon(*m_pixmapCache["on16"]));
		ui->pushButton_EnableDisableDevice->setText("	" + tr("Turn lights ON"));
		if (m_trayIcon)
			m_trayIcon->setStatus(SysTrayIcon::StatusOff);
		break;

	case Backlight::StatusDeviceError:
		m_labelStatusIcon->setPixmap(*m_pixmapCache["error16"]);
		ui->pushButton_EnableDisableDevice->setIcon(QIcon(*m_pixmapCache["off16"]));
		ui->pushButton_EnableDisableDevice->setText("	" + tr("Turn lights OFF"));
		if (m_trayIcon)
			m_trayIcon->setStatus(SysTrayIcon::StatusError);
		break;
	default:
		qWarning() << Q_FUNC_INFO << "m_backlightStatus = " << m_backlightStatus;
		break;
	}
	if (m_trayIcon)
		m_labelStatusIcon->setToolTip(m_trayIcon->toolTip());
}

void SettingsWindow::initGrabbersRadioButtonsVisibility()
{
#ifndef WINAPI_GRAB_SUPPORT
	ui->radioButton_GrabWinAPI->setVisible(false);
#else
	ui->radioButton_GrabWinAPI->setChecked(true);
#endif
#ifndef DDUPL_GRAB_SUPPORT
	ui->radioButton_GrabDDupl->setVisible(false);
#endif
#ifndef D3D10_GRAB_SUPPORT
	ui->checkBox_EnableDx1011Capture->setVisible(false);
	ui->checkBox_EnableDx9Capture->setVisible(false);
#endif
#ifndef X11_GRAB_SUPPORT
	ui->radioButton_GrabX11->setVisible(false);
#else
	ui->radioButton_GrabX11->setChecked(true);
#endif
#ifndef MAC_OS_CG_GRAB_SUPPORT
	ui->radioButton_GrabMacCoreGraphics->setVisible(false);
#else
	ui->radioButton_GrabMacCoreGraphics->setChecked(true);
#endif
}

// ----------------------------------------------------------------------------
// Show grabbed colors in another GUI
// ----------------------------------------------------------------------------

void SettingsWindow::initVirtualLeds(int virtualLedsCount)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << virtualLedsCount;

	// Remove all virtual leds from grid layout
	for (int i = 0; i < m_labelsGrabbedColors.count(); i++)
	{
		ui->gridLayout_VirtualLeds->removeWidget(m_labelsGrabbedColors[i]);
		m_labelsGrabbedColors[i]->deleteLater();
	}

	m_labelsGrabbedColors.clear();

	for (int i = 0; i < virtualLedsCount; i++)
	{
		QLabel *label = new QLabel(this);
		label->setText(QString::number(i + 1));
		label->setAlignment(Qt::AlignCenter);
		label->setAutoFillBackground(true);

		if (m_backlightStatus == Backlight::StatusOff)
		{
			// If status off fill labels black:
			QPalette pal = label->palette();
			pal.setBrush(QPalette::Window, QBrush(Qt::black));
			label->setPalette(pal);
		}

		m_labelsGrabbedColors.append(label);

		int row = i / 10;
		int col = i % 10;

		ui->gridLayout_VirtualLeds->addWidget(label, row, col);
	}

	ui->frame_VirtualLeds->update();
}

void SettingsWindow::updateVirtualLedsColors(const QList<QRgb> & colors)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO;

	if (colors.count() != m_labelsGrabbedColors.count())
	{
		qWarning() << Q_FUNC_INFO << "colors.count()" << colors.count() << "!=" << "m_labelsGrabbedColors.count()" << m_labelsGrabbedColors.count() << "."
					<< "Cancel updating virtual colors." << sender();
		return;
	}

	for (int i = 0; i < colors.count(); i++)
	{
		QLabel *label = m_labelsGrabbedColors[i];
		QColor color(colors[i]);

		QPalette pal = label->palette();
		pal.setBrush(QPalette::Window, QBrush(color));
		label->setPalette(pal);
	}
}

void SettingsWindow::requestBacklightStatus()
{
	emit resultBacklightStatus(m_backlightStatus);
}

void SettingsWindow::onApiServer_ErrorOnStartListening(QString errorMessage)
{
	ui->lineEdit_ApiPort->setStyleSheet("background-color:red;");
	ui->lineEdit_ApiPort->setToolTip(errorMessage);
}

void SettingsWindow::onPingDeviceEverySecond_Toggled(bool state)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;

	Settings::setPingDeviceEverySecond(state);

	// Force update colors on device for start ping device
	//	m_grabManager->reset();
	//	m_moodlampManager->reset();
}

void SettingsWindow::processMessage(const QString &message)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << message;

	if (message == "on")
		setBacklightStatus(Backlight::StatusOn);
	else if (message == "off")
		setBacklightStatus(Backlight::StatusOff);
	else if (message.startsWith("set-profile ")) {
		QString profile = message.mid(12);
		profileSwitch(profile);
	} else if (message == "quitForWizard") {
		qWarning() << "Wizard was started, quitting!";
		LightpackApplication::quit();
	} else if (m_trayIcon != NULL) {
		qWarning(qPrintable(message));
		m_trayIcon->showMessage(SysTrayIcon::MessageAnotherInstance);
	}
}

#ifdef BASS_SOUND_SUPPORT
void SettingsWindow::updateAvailableSoundVizDevices(const QList<SoundManagerDeviceInfo> & devices, int recommended)
{
	ui->comboBox_SoundVizDevice->blockSignals(true);
	ui->comboBox_SoundVizDevice->clear();
	int selectedDevice = Settings::getSoundVisualizerDevice();
	if (selectedDevice == -1) selectedDevice = recommended;
	int selectIndex = -1;
	for (int i = 0; i < devices.size(); i++) {
		ui->comboBox_SoundVizDevice->addItem(devices[i].name, devices[i].id);
		if (devices[i].id == selectedDevice) {
			selectIndex = i;
		}
	}
	ui->comboBox_SoundVizDevice->setCurrentIndex(selectIndex);
	ui->comboBox_SoundVizDevice->blockSignals(false);
}
#endif

// ----------------------------------------------------------------------------
// Show / Hide settings and about windows
// ----------------------------------------------------------------------------

void SettingsWindow::showAbout()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	emit requestFirmwareVersion();

	ui->tabWidget->setCurrentWidget(ui->tabAbout);
	this->show();

	m_smoothScrollTimer.setInterval(100);
	connect(&m_smoothScrollTimer, SIGNAL(timeout()), this, SLOT(scrollThanks()));
	m_smoothScrollTimer.start();
}

void SettingsWindow::scrollThanks()
{
	QScrollBar *scrollBar = this->ui->textBrowser->verticalScrollBar();

	scrollBar->setValue(scrollBar->value() + 1);

}

void SettingsWindow::showSettings()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	this->show();
	raise();
	this->activateWindow();
}

void SettingsWindow::hideSettings()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	emit showLedWidgets(false);

	this->hide();
}

void SettingsWindow::toggleSettings()
{
	if(this->isVisible())
		hideSettings();
	else
		showSettings();
}


// ----------------------------------------------------------------------------
// Public slots
// ----------------------------------------------------------------------------

void SettingsWindow::ledDeviceOpenSuccess(bool isSuccess)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << isSuccess;

	if (isSuccess)
	{
		// Device just connected and for updating colors
		// we should reset previous saved states
		//m_grabManager->reset();
		// m_moodlampManager->reset();
	}

	ledDeviceCallSuccess(isSuccess);
}

void SettingsWindow::ledDeviceCallSuccess(bool isSuccess)
{	
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << isSuccess << m_backlightStatus << sender();

	// If Backlight::StatusOff then nothings changed

	if (isSuccess == false)
	{
		if (m_backlightStatus == Backlight::StatusOn) {
			m_backlightStatus = Backlight::StatusDeviceError;
			updateTrayAndActionStates();
		}
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Backlight::StatusDeviceError";
	} else {
		if (m_backlightStatus == Backlight::StatusDeviceError) {
			m_backlightStatus = Backlight::StatusOn;
			updateTrayAndActionStates();
		}
	}
}

void SettingsWindow::ledDeviceFirmwareVersionResult(const QString & fwVersion)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << fwVersion;

	m_deviceFirmwareVersion = fwVersion;

	QString aboutDialogFirmwareString = m_deviceFirmwareVersion;

	if (Settings::getConnectedDevice() == SupportedDevices::DeviceTypeLightpack)
	{
		if (m_deviceFirmwareVersion == "5.0" || m_deviceFirmwareVersion == "4.3")
		{
			aboutDialogFirmwareString += QString(" ") +
					"(<a href=\"" + LightpackDownloadsPageUrl + "\">" +
					tr("update firmware") +
					"</a>)";

			if (Settings::isUpdateFirmwareMessageShown() == false)
			{
				if (m_trayIcon!=NULL)
					m_trayIcon->showMessage(SysTrayIcon::MessageUpdateFirmware);
				Settings::setUpdateFirmwareMessageShown(true);
			}
		}
	}

	this->setFirmwareVersion(aboutDialogFirmwareString);

	updateDeviceTabWidgetsVisibility();
}

void SettingsWindow::ledDeviceFirmwareVersionUnofficialResult(const int version) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << version;

	// Here we handle certain options that have to be hidden/made visible depending on our version
	ui->checkBox_DisableUsbPowerLed->setVisible(version >= 1);
}

void SettingsWindow::refreshAmbilightEvaluated(double updateResultMs)
{	
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << updateResultMs;

	double hz = 0;

	if (updateResultMs != 0)
		hz = 1000.0 / updateResultMs; /* ms to hz */
	
	double maxHz = 1000.0 / ui->spinBox_GrabSlowdown->value(); // cap with display refresh rate?

	QString fpsText = QString::number(hz, 'f', 0) + " / " + QString::number(maxHz, 'f', 0);

	ui->label_GrabFrequency_value->setText(fpsText);

	this->labelFPS->setText(tr("FPS: ") + fpsText);
}

// ----------------------------------------------------------------------------
// UI handlers
// ----------------------------------------------------------------------------

void SettingsWindow::onGrabberChanged()
{
	if (!updatingFromSettings) {
		Grab::GrabberType grabberType = getSelectedGrabberType();

		if (grabberType != Settings::getGrabberType()) {
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "GrabberType: " << grabberType << ", isDx1011CaptureEnabled: " << isDx1011CaptureEnabled();
			Settings::setGrabberType(grabberType);
		}
	}
}

void SettingsWindow::onDx1011CaptureEnabledChanged(bool isEnabled) {
	if (!updatingFromSettings) {
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << isEnabled;
#ifdef D3D10_GRAB_SUPPORT
		Settings::setDx1011GrabberEnabled(isEnabled);
#endif
	}
	ui->checkBox_EnableDx9Capture->setEnabled(isEnabled);
}

void SettingsWindow::onDx9CaptureEnabledChanged(bool isEnabled) {
	if (!updatingFromSettings) {
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << isEnabled;
#ifdef D3D10_GRAB_SUPPORT
		Settings::setDx9GrabbingEnabled(isEnabled);
#endif
	}
}

void SettingsWindow::onGrabSlowdown_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setGrabSlowdown(value);
	refreshAmbilightEvaluated(0);// update max grab rate
}

void SettingsWindow::onGrabIsAvgColors_toggled(bool state)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;

	Settings::setGrabAvgColorsEnabled(state);
}

void SettingsWindow::onGrabOverBrighten_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setGrabOverBrighten(value);
}

void SettingsWindow::onGrabApplyGammaRamp_toggled(bool state)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;

	Settings::setGrabApplyGammaRampEnabled(state);
}


void SettingsWindow::onLuminosityThreshold_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setLuminosityThreshold(value);
}

void SettingsWindow::onMinimumLumosity_toggled(bool value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setMinimumLuminosityEnabled(ui->radioButton_MinimumLuminosity->isChecked());
}

void SettingsWindow::onDeviceRefreshDelay_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setDeviceRefreshDelay(value);
}

void SettingsWindow::onDisableUsbPowerLed_toggled(bool state) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;

	Settings::setDeviceUsbPowerLedDisabled(state);
}

void SettingsWindow::onDeviceSmooth_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setDeviceSmooth(value);
}

void SettingsWindow::onDeviceBrightness_valueChanged(int percent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << percent;

	Settings::setDeviceBrightness(percent);
}

void SettingsWindow::onDeviceColorDepth_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setDeviceColorDepth(value);
}

void SettingsWindow::onDeviceGammaCorrection_valueChanged(double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	Settings::setDeviceGamma(value);
	ui->horizontalSlider_GammaCorrection->setValue(floor((value * 100 + 0.5)));
	emit updateGamma(Settings::getDeviceGamma());
}

void SettingsWindow::onSliderDeviceGammaCorrection_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	Settings::setDeviceGamma(static_cast<double>(value + 0.4) / 100);
	ui->doubleSpinBox_DeviceGamma->setValue(Settings::getDeviceGamma());
	emit updateGamma(Settings::getDeviceGamma());
}

void SettingsWindow::onLightpackModes_currentIndexChanged(int index)
{
	if (updatingFromSettings) return;

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << index << sender();

	using namespace Lightpack;

	switch (index) {
		case MoodLampModeIndex:
			Settings::setLightpackMode(MoodLampMode);
			break;
#ifdef BASS_SOUND_SUPPORT
		case SoundVisualizeModeIndex:
			Settings::setLightpackMode(SoundVisualizeMode);
			break;
#endif
		default:
			Settings::setLightpackMode(AmbilightMode);

	}
}

void SettingsWindow::onLightpackModeChanged(Lightpack::Mode mode)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << mode << ui->comboBox_LightpackModes->currentIndex();

	switch (mode)
	{
	case Lightpack::AmbilightMode:
		ui->comboBox_LightpackModes->setCurrentIndex(GrabModeIndex);
		ui->stackedWidget_LightpackModes->setCurrentIndex(GrabModeIndex);
		emit showLedWidgets(!ui->radioButton_GrabWidgetsDontShow->isChecked() && this->isVisible());
		break;

	case Lightpack::MoodLampMode:
		ui->comboBox_LightpackModes->setCurrentIndex(MoodLampModeIndex);
		ui->stackedWidget_LightpackModes->setCurrentIndex(MoodLampModeIndex);
		emit showLedWidgets(false);
		break;

#ifdef BASS_SOUND_SUPPORT
	case SoundVisualizeModeIndex:
		ui->comboBox_LightpackModes->setCurrentIndex(SoundVisualizeModeIndex);
		ui->stackedWidget_LightpackModes->setCurrentIndex(SoundVisualizeModeIndex);
		emit showLedWidgets(false);
		break;
#endif

	default:
		DEBUG_LOW_LEVEL << "LightpacckMode unsuppotred value =" << mode;
		break;
	}
	backlightStatusChanged(m_backlightStatus);
}

void SettingsWindow::onMoodLampColor_changed(QColor color)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << color;
	Settings::setMoodLampColor(color);
}

void SettingsWindow::onMoodLampSpeed_valueChanged(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	Settings::setMoodLampSpeed(value);
}

void SettingsWindow::onMoodLampLiquidMode_Toggled(bool checked)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << checked;

	Settings::setMoodLampLiquidMode(checked);
	if (Settings::isMoodLampLiquidMode())
	{
		ui->pushButton_SelectColorMoodLamp->setEnabled(false);
		ui->horizontalSlider_MoodLampSpeed->setEnabled(true);
	} else {
		ui->pushButton_SelectColorMoodLamp->setEnabled(true);
		ui->horizontalSlider_MoodLampSpeed->setEnabled(false);
	}
}

#ifdef BASS_SOUND_SUPPORT
void SettingsWindow::onSoundVizDevice_currentIndexChanged(int index)
{
	if (!updatingFromSettings) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << index << ui->comboBox_SoundVizDevice->currentData().toInt();
		Settings::setSoundVisualizerDevice(ui->comboBox_SoundVizDevice->currentData().toInt());
	}
}

void SettingsWindow::onSoundVizMinColor_changed(QColor color)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << color;
	Settings::setSoundVisualizerMinColor(color);
}

void SettingsWindow::onSoundVizMaxColor_changed(QColor color)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << color;
	Settings::setSoundVisualizerMaxColor(color);
}

void SettingsWindow::onSoundVizLiquidMode_Toggled(bool isLiquidMode)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << isLiquidMode;

	Settings::setSoundVisualizerLiquidMode(isLiquidMode);
	if (Settings::isSoundVisualizerLiquidMode())
	{
		ui->pushButton_SelectColorSoundVizMin->setEnabled(false);
		ui->pushButton_SelectColorSoundVizMax->setEnabled(false);
		ui->horizontalSlider_SoundVizLiquidSpeed->setEnabled(true);
	} else {
		ui->pushButton_SelectColorSoundVizMin->setEnabled(true);
		ui->pushButton_SelectColorSoundVizMax->setEnabled(true);
		ui->horizontalSlider_SoundVizLiquidSpeed->setEnabled(false);
	}
}

void SettingsWindow::onSoundVizLiquidSpeed_valueChanged(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value;
	Settings::setSoundVisualizerLiquidSpeed(value);
}
#endif

void SettingsWindow::onDontShowLedWidgets_Toggled(bool checked)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << checked;
	emit showLedWidgets(!checked);
}

void SettingsWindow::onSetColoredLedWidgets(bool checked)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (checked)
		emit setColoredLedWidget(true);
}

void SettingsWindow::onSetWhiteLedWidgets(bool checked)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (checked)
		emit setColoredLedWidget(false);
}

void SettingsWindow::onDeviceSendDataOnlyIfColorsChanged_toggled(bool state)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;

	Settings::setSendDataOnlyIfColorsChanges(state);
}

// ----------------------------------------------------------------------------

void SettingsWindow::openFile(const QString &filePath)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString filePrefix = "file://";

#ifdef Q_OS_WIN
	filePrefix = "file:///";
#endif

	QDesktopServices::openUrl(QUrl(filePrefix + filePath, QUrl::TolerantMode));
}

// ----------------------------------------------------------------------------
// Profiles
// ----------------------------------------------------------------------------

void SettingsWindow::openCurrentProfile()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	openFile(Settings::getCurrentProfilePath());
}

void SettingsWindow::profileRename()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString configName = ui->comboBox_Profiles->currentText().trimmed();
	ui->comboBox_Profiles->lineEdit()->setText(configName);

	// Signal editingFinished() will be emited if focus wasn't lost (for example when return pressed),
	// and profileRename() function will be called again here
	this->setFocus(Qt::OtherFocusReason);

	if (Settings::getCurrentProfileName() == configName)
	{
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Nothing has changed";
		return;
	}

	if (configName == "")
	{
		configName = Settings::getCurrentProfileName();
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Profile name is empty, return back to" << configName;
	}
	else
	{
		Settings::renameCurrentProfile(configName);
	}

	ui->comboBox_Profiles->lineEdit()->setText(configName);
	ui->comboBox_Profiles->setItemText(ui->comboBox_Profiles->currentIndex(), configName);
}

void SettingsWindow::profileSwitch(const QString & configName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << configName;

	profilesLoadAll();

	int index = ui->comboBox_Profiles->findText(configName);

	if (index < 0)
	{
		qCritical() << Q_FUNC_INFO << "Fail find text:" << configName << "in profiles combobox";
		return;
	}

	ui->comboBox_Profiles->setCurrentIndex(index);

	Settings::loadOrCreateProfile(configName);

	if (m_trayIcon)
		m_trayIcon->updateProfiles();

}

void SettingsWindow::handleProfileLoaded(const QString &configName) {

	this->labelProfile->setText(tr("Profile: %1").arg(configName));
	updateUiFromSettings();
	checkOutdatedGrabber();
}

void SettingsWindow::profileTraySwitch(const QString &profileName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "switch to" << profileName;
	profileSwitchCombobox(profileName);
	return;
}

void SettingsWindow::profileSwitchCombobox(QString profile)
{
	int index = ui->comboBox_Profiles->findText(profile);
	ui->comboBox_Profiles->setCurrentIndex(index);
}

void SettingsWindow::profileNew()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString profileName = tr("New profile");

	if(ui->comboBox_Profiles->findText(profileName) != -1){
		int i = 1;
		while(ui->comboBox_Profiles->findText(profileName +" "+ QString::number(i)) != -1){
			i++;
		}
		profileName += + " " + QString::number(i);
	}

	ui->comboBox_Profiles->insertItem(0, profileName);
	ui->comboBox_Profiles->setCurrentIndex(0);

	ui->comboBox_Profiles->lineEdit()->selectAll();
	ui->comboBox_Profiles->lineEdit()->setFocus(Qt::MouseFocusReason);
}

void SettingsWindow::profileResetToDefaultCurrent()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	Settings::resetDefaults();

	// By default backlight is enabled, but make it same as current backlight status for usability purposes
	Settings::setIsBacklightEnabled(m_backlightStatus != Backlight::StatusOff);

	// Update settings
	updateUiFromSettings();
}

void SettingsWindow::profileDeleteCurrent()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if(ui->comboBox_Profiles->count() <= 1){
		qWarning() << "void MainWindow::profileDeleteCurrent(): profiles count ==" << ui->comboBox_Profiles->count();
		return;
	}

	// Delete settings file
	Settings::removeCurrentProfile();
	// Remove from combobox
	ui->comboBox_Profiles->removeItem(ui->comboBox_Profiles->currentIndex());
}

void SettingsWindow::profilesLoadAll()
{
	QStringList profiles = Settings::findAllProfiles();

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "found profiles:" << profiles;


	disconnect(ui->comboBox_Profiles, SIGNAL(currentIndexChanged(QString)), this, SLOT(profileSwitch(QString)));

	for (int i = 0; i < profiles.count(); i++)
	{
		if (ui->comboBox_Profiles->findText(profiles.at(i)) == -1)
			ui->comboBox_Profiles->addItem(profiles.at(i));
	}

	connect(ui->comboBox_Profiles, SIGNAL(currentIndexChanged(QString)), this, SLOT(profileSwitch(QString)));
}

void SettingsWindow::settingsProfileChanged_UpdateUI(const QString &profileName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	setWindowTitle(tr("Prismatik: %1").arg(profileName));

	if (m_backlightStatus == Backlight::StatusOn && m_trayIcon!=NULL)
		m_trayIcon->updateProfiles();

	if(ui->comboBox_Profiles->count() > 1){
		ui->pushButton_DeleteProfile->setEnabled(true);
	}else{
		ui->pushButton_DeleteProfile->setEnabled(false);
	}
}

// ----------------------------------------------------------------------------

void SettingsWindow::initPixmapCache()
{
	m_pixmapCache.insert("lock16", new QPixmap(QPixmap(":/icons/lock.png").scaledToWidth(16, Qt::SmoothTransformation)));
	m_pixmapCache.insert("on16", new QPixmap(QPixmap(":/icons/on.png").scaledToWidth(16, Qt::SmoothTransformation)) );
	m_pixmapCache.insert("off16", new QPixmap(QPixmap(":/icons/off.png").scaledToWidth(16, Qt::SmoothTransformation)) );
	m_pixmapCache.insert("error16", new QPixmap(QPixmap(":/icons/error.png").scaledToWidth(16, Qt::SmoothTransformation)) );
	m_pixmapCache.insert("persist16", new QPixmap(QPixmap(":/icons/persist.png").scaledToWidth(16, Qt::SmoothTransformation)));
}

//void SettingsWindow::handleConnectedDeviceChange(const SupportedDevices::DeviceType deviceType) {
//	this->labelDevice->setText(tr("Device: %1").arg(Settings::getConnectedDeviceName()));
//	updateUiFromSettings();
//}

// ----------------------------------------------------------------------------
// Translate GUI
// ----------------------------------------------------------------------------

void SettingsWindow::initLanguages()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	ui->comboBox_Language->clear();
	ui->comboBox_Language->addItem(tr("System default"));
	ui->comboBox_Language->addItem("English");
	ui->comboBox_Language->addItem("Russian");
	ui->comboBox_Language->addItem("Ukrainian");

	int langIndex = 0; // "System default"
	QString langSaved = Settings::getLanguage();
	if(langSaved != "<System>"){
		langIndex = ui->comboBox_Language->findText(langSaved);
	}
	ui->comboBox_Language->setCurrentIndex(langIndex);

	m_translator = NULL;
}

void SettingsWindow::loadTranslation(const QString & language)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << language;

	Settings::setLanguage(language);

	QString locale = QLocale::system().name();

	// add translation to Lightpack.pro TRANSLATIONS
	// lupdate Lightpack.pro
	// open linguist and translate application
	// lrelease Lightpack.pro
	// add new language to LightpackResources.qrc :/translations/
	// add new language to MainWindow::initLanguages() function
	// and only when all this done - append new line
	// locale - name of translation binary file form resources: %locale%.qm
	if(ui->comboBox_Language->currentIndex() == 0 /* System */){
		DEBUG_LOW_LEVEL << "System locale" << locale;
		Settings::setLanguage(SettingsScope::Main::LanguageDefault);
	}
	else if (language == "English") locale = "en_EN"; // :/translations/en_EN.qm
	else if (language == "Russian") locale = "ru_RU"; // :/translations/ru_RU.qm
	else if (language == "Ukrainian") locale = "uk_UA"; // :/translations/uk_UA.qm
	// append line for new language/locale here
	else {
		qWarning() << "Language" << language << "not found. Set to default" << SettingsScope::Main::LanguageDefault;
		DEBUG_LOW_LEVEL << "System locale" << locale;

		Settings::setLanguage(SettingsScope::Main::LanguageDefault);
	}

	QString pathToLocale = QString(":/translations/") + locale;

	if(m_translator != NULL){
		qApp->removeTranslator(m_translator);
		delete m_translator;
		m_translator = NULL;
	}

	if(locale == "en_EN" /* default no need to translate */){
		DEBUG_LOW_LEVEL << "Translation removed, using default locale" << locale;
		return;
	}

	updatingFromSettings = true;
	m_translator = new QTranslator();
	if(m_translator->load(pathToLocale)){
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Load translation for locale" << locale;
		qApp->installTranslator(m_translator);
	}else{
		qWarning() << "Fail load translation for locale" << locale << "pathToLocale" << pathToLocale;
	}
	updatingFromSettings = false;
}

// ----------------------------------------------------------------------------
// Create tray icon
// ----------------------------------------------------------------------------

void SettingsWindow::createTrayIcon()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	m_trayIcon = new SysTrayIcon();
	connect(m_trayIcon, SIGNAL(quit()), this, SLOT(quit()));
	connect(m_trayIcon, SIGNAL(showSettings()), this, SLOT(showSettings()));
	connect(m_trayIcon, SIGNAL(toggleSettings()), this, SLOT(toggleSettings()));
	connect(m_trayIcon, SIGNAL(backlightOn()), this, SLOT(backlightOn()));
	connect(m_trayIcon, SIGNAL(backlightOff()), this, SLOT(backlightOff()));
	connect(m_trayIcon, SIGNAL(profileSwitched(QString)), this, SLOT(profileTraySwitch(QString)));

	m_trayIcon->init();
	connect(this, SIGNAL(backlightStatusChanged(Backlight::Status)), this, SLOT(updateTrayAndActionStates()));
}

void SettingsWindow::updateUiFromSettings()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	updatingFromSettings = true;

	profilesLoadAll();

	ui->comboBox_Profiles->setCurrentIndex(ui->comboBox_Profiles->findText(Settings::getCurrentProfileName()));

	Lightpack::Mode mode = Settings::getLightpackMode();
	onLightpackModeChanged(mode);

	ui->checkBox_ExpertModeEnabled->setChecked						(Settings::isExpertModeEnabled());
	
	ui->checkBox_checkForUpdates->setChecked							(Settings::isCheckForUpdatesEnabled());
	ui->checkBox_installUpdates->setChecked							(Settings::isInstallUpdatesEnabled());

	ui->checkBox_SendDataOnlyIfColorsChanges->setChecked				(Settings::isSendDataOnlyIfColorsChanges());
	ui->checkBox_KeepLightsOnAfterExit->setChecked					(Settings::isKeepLightsOnAfterExit());
	ui->checkBox_KeepLightsOnAfterLockComputer->setChecked			(Settings::isKeepLightsOnAfterLock());
	ui->checkBox_KeepLightsOnAfterSuspend->setChecked				(Settings::isKeepLightsOnAfterSuspend());
	ui->checkBox_PingDeviceEverySecond->setChecked					(Settings::isPingDeviceEverySecond());

	ui->checkBox_GrabIsAvgColors->setChecked							(Settings::isGrabAvgColorsEnabled());
	ui->spinBox_GrabSlowdown->setValue								(Settings::getGrabSlowdown());
	ui->spinBox_GrabOverBrighten->setValue							(Settings::getGrabOverBrighten());
	ui->checkBox_GrabApplyGammaRamp->setChecked						(Settings::isGrabApplyGammaRampEnabled());
	ui->spinBox_LuminosityThreshold->setValue						(Settings::getLuminosityThreshold());

	// Check the selected moodlamp mode (setChecked(false) not working to select another)
	ui->radioButton_MinimumLuminosity->setChecked					(Settings::isMinimumLuminosityEnabled());
	ui->radioButton_LuminosityDeadZone->setChecked					(!Settings::isMinimumLuminosityEnabled());

	// Check the selected moodlamp mode (setChecked(false) not working to select another)
	ui->radioButton_ConstantColorMoodLampMode->setChecked			(!Settings::isMoodLampLiquidMode());
	ui->radioButton_LiquidColorMoodLampMode->setChecked				(Settings::isMoodLampLiquidMode());
	ui->pushButton_SelectColorMoodLamp->setColor						(Settings::getMoodLampColor());
	ui->horizontalSlider_MoodLampSpeed->setValue						(Settings::getMoodLampSpeed());

#ifdef BASS_SOUND_SUPPORT
	for (int i = 0; i < ui->comboBox_SoundVizDevice->count(); i++) {
		if (ui->comboBox_SoundVizDevice->itemData(i).toInt() == Settings::getSoundVisualizerDevice()) {
			ui->comboBox_SoundVizDevice->setCurrentIndex(i);
			break;
		}
	}
	ui->pushButton_SelectColorSoundVizMin->setColor					(Settings::getSoundVisualizerMinColor());
	ui->pushButton_SelectColorSoundVizMax->setColor					(Settings::getSoundVisualizerMaxColor());
	ui->radioButton_SoundVizConstantMode->setChecked					(!Settings::isSoundVisualizerLiquidMode());
	ui->radioButton_SoundVizLiquidMode->setChecked					(Settings::isSoundVisualizerLiquidMode());
	ui->horizontalSlider_SoundVizLiquidSpeed->setValue				(Settings::getSoundVisualizerLiquidSpeed());
#endif

	ui->checkBox_DisableUsbPowerLed->setChecked						(Settings::isDeviceUsbPowerLedDisabled());
	ui->horizontalSlider_DeviceRefreshDelay->setValue				(Settings::getDeviceRefreshDelay());
	ui->horizontalSlider_DeviceBrightness->setValue					(Settings::getDeviceBrightness());
	ui->horizontalSlider_DeviceSmooth->setValue						(Settings::getDeviceSmooth());
	ui->horizontalSlider_DeviceColorDepth->setValue					(Settings::getDeviceColorDepth());
	ui->doubleSpinBox_DeviceGamma->setValue							(Settings::getDeviceGamma());
	ui->horizontalSlider_GammaCorrection->setValue			(floor((Settings::getDeviceGamma() * 100 + 0.5)));

	ui->groupBox_Api->setChecked										(Settings::isApiEnabled());
	ui->checkBox_listenOnlyOnLoInterface->setChecked					(Settings::isListenOnlyOnLoInterface());
	ui->lineEdit_ApiPort->setText					(QString::number(Settings::getApiPort()));
	ui->lineEdit_ApiPort->setValidator(new QIntValidator(1, 49151));
	ui->lineEdit_ApiKey->setText										(Settings::getApiAuthKey());
	ui->spinBox_LoggingLevel->setValue								(g_debugLevel);

	switch (Settings::getGrabberType())
	{
#ifdef WINAPI_GRAB_SUPPORT
	case Grab::GrabberTypeWinAPI:
		ui->radioButton_GrabWinAPI->setChecked(true);
		break;
#endif
#ifdef DDUPL_GRAB_SUPPORT
	case Grab::GrabberTypeDDupl:
		ui->radioButton_GrabDDupl->setChecked(true);
		break;
#endif
#ifdef X11_GRAB_SUPPORT
	case Grab::GrabberTypeX11:
		ui->radioButton_GrabX11->setChecked(true);
		break;
#endif
#ifdef MAC_OS_CG_GRAB_SUPPORT
	case Grab::GrabberTypeMacCoreGraphics:
		ui->radioButton_GrabMacCoreGraphics->setChecked(true);
		break;
#endif
	}

#ifdef D3D10_GRAB_SUPPORT
	ui->checkBox_EnableDx1011Capture->setChecked(Settings::isDx1011GrabberEnabled());
	ui->checkBox_EnableDx9Capture->setChecked(Settings::isDx9GrabbingEnabled());
#endif

	onMoodLampLiquidMode_Toggled(ui->radioButton_LiquidColorMoodLampMode->isChecked());
	updateExpertModeWidgetsVisibility();
	onGrabberChanged();
	settingsProfileChanged_UpdateUI(Settings::getCurrentProfileName());
	updatingFromSettings = false;
}

Grab::GrabberType SettingsWindow::getSelectedGrabberType()
{
#ifdef X11_GRAB_SUPPORT
	if (ui->radioButton_GrabX11->isChecked()) {
		return Grab::GrabberTypeX11;
	}
#endif
#ifdef WINAPI_GRAB_SUPPORT
	if (ui->radioButton_GrabWinAPI->isChecked()) {
		return Grab::GrabberTypeWinAPI;
	}
#endif
#ifdef DDUPL_GRAB_SUPPORT
	if (ui->radioButton_GrabDDupl->isChecked()) {
		return Grab::GrabberTypeDDupl;
	}
#endif
#ifdef MAC_OS_CG_GRAB_SUPPORT
	if (ui->radioButton_GrabMacCoreGraphics->isChecked()) {
		return Grab::GrabberTypeMacCoreGraphics;
	}
#endif

	return Grab::GrabberTypeQt;
}

bool SettingsWindow::isDx1011CaptureEnabled() {
	return ui->checkBox_EnableDx1011Capture->isChecked();
}

// ----------------------------------------------------------------------------
// Quit application
// ----------------------------------------------------------------------------

void SettingsWindow::quit()
{
	if (!ui->checkBox_KeepLightsOnAfterExit->isChecked())
	{
		// Process all currently pending signals (which may include updating the color signals)
		QApplication::processEvents(QEventLoop::AllEvents, 1000);

		emit switchOffLeds();
		QApplication::processEvents(QEventLoop::AllEvents, 1000);
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "trayIcon->hide();";

	if (m_trayIcon!=NULL) {
		m_trayIcon->hide();
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "QApplication::quit();";

	QApplication::quit();
}

void SettingsWindow::setFirmwareVersion(const QString &firmwareVersion)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	this->fimwareVersion = firmwareVersion;
	versionsUpdate();
}

void SettingsWindow::versionsUpdate()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	// use template to construct version string
	QString versionsTemplate = tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"> <html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"> p, li { white-space: pre-wrap; } </style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\"> <p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">software <span style=\" font-size:8pt; font-weight:600;\">%1</span><span style=\" font-size:8pt;\"> (revision </span><a href=\"https://github.com/psieg/Lightpack/commit/%2\"><span style=\" font-size:8pt; text-decoration: underline; color:#0000ff;\">%2</span></a><span style=\" font-size:8pt;\">), firmware <b>%3</b></span></p></body></html>");

#ifdef GIT_REVISION
	versionsTemplate = versionsTemplate.arg(
				QApplication::applicationVersion(),
				GIT_REVISION,
				fimwareVersion );
#else
	versionsTemplate = versionsTemplate.arg(
				QApplication::applicationVersion(),
				"unknown",
				fimwareVersion );
	versionsTemplate.remove(QRegExp(" \\([^()]+unknown[^()]+\\)"));
#endif

	ui->labelVersions->setText( versionsTemplate );

	adjustSize();
	resize(minimumSize());
}

void SettingsWindow::showHelpOf(QObject *object)
{
	QCoreApplication::postEvent(object, new QHelpEvent(QEvent::WhatsThis, QPoint(0,0), QCursor::pos()));
}

void SettingsWindow::on_pushButton_LightpackSmoothnessHelp_clicked()
{
	showHelpOf(ui->horizontalSlider_DeviceSmooth);
}

void SettingsWindow::on_pushButton_LightpackColorDepthHelp_clicked()
{
	showHelpOf(ui->horizontalSlider_DeviceColorDepth);
}

void SettingsWindow::on_pushButton_LightpackRefreshDelayHelp_clicked()
{
	showHelpOf(ui->horizontalSlider_DeviceRefreshDelay);
}

void SettingsWindow::on_pushButton_GammaCorrectionHelp_clicked()
{
	showHelpOf(ui->horizontalSlider_GammaCorrection);
}

void SettingsWindow::on_pushButton_lumosityThresholdHelp_clicked()
{
	showHelpOf(ui->horizontalSlider_LuminosityThreshold);
}

void SettingsWindow::on_pushButton_grabOverBrightenHelp_clicked()
{
	showHelpOf(ui->horizontalSlider_GrabOverBrighten);
}

void SettingsWindow::on_pushButton_AllPluginsHelp_clicked()
{
	showHelpOf(ui->label_AllPlugins);
}

bool SettingsWindow::toPriority(Plugin* s1 ,Plugin* s2 )
{
	return s1->getPriority() > s2->getPriority();
}

void SettingsWindow::updatePlugin(QList<Plugin*> plugins)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;


	_plugins = plugins;
	// sort priority
	qSort(_plugins.begin() , _plugins.end(), SettingsWindow::toPriority );
	ui->list_Plugins->clear();
	foreach(Plugin* plugin, _plugins){
		int index = _plugins.indexOf(plugin);
		QListWidgetItem *item = new QListWidgetItem(getPluginName(plugin));
		item->setData(Qt::UserRole, index);
		item->setIcon(plugin->Icon());
		if (plugin->isEnabled())
		{
			item->setCheckState(Qt::Checked);
		}
		else
			item->setCheckState(Qt::Unchecked);

		ui->list_Plugins->addItem(item);
	}

	ui->pushButton_ReloadPlugins->setEnabled(true);

}

void SettingsWindow::on_list_Plugins_itemClicked(QListWidgetItem* current)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	bool isEnabled = true;

	if (current->checkState() == Qt::Checked)
		isEnabled = true;
	else
		isEnabled = false;

	int index =current->data(Qt::UserRole).toUInt();
	if (_plugins[index]->isEnabled() != isEnabled)
		_plugins[index]->setEnabled(isEnabled);

	pluginSwitch(index);
}

void SettingsWindow::pluginSwitch(int index)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << index;

	if (index == -1)
	{
		ui->label_PluginName->setText("");
		ui->label_PluginAuthor->setText("");
		ui->label_PluginVersion->setText("");
		ui->tb_PluginDescription->setText("");
		ui->label_PluginIcon->setPixmap(QIcon(":/plugin/Plugin.png").pixmap(50,50));
		return;
	}

	ui->label_PluginName->setText(_plugins[index]->Name());
	ui->label_PluginAuthor->setText(_plugins[index]->Author());
	ui->label_PluginVersion->setText(_plugins[index]->Version());
	ui->tb_PluginDescription->setText(_plugins[index]->Description());
	ui->label_PluginIcon->setPixmap(_plugins[index]->Icon().pixmap(50,50));

}

void SettingsWindow::on_pushButton_ReloadPlugins_clicked()
{
	foreach(Plugin* plugin, _plugins){
		plugin->Stop();
	}
	ui->list_Plugins->clear();
	_plugins.clear();
	ui->pushButton_ReloadPlugins->setEnabled(false);
	emit reloadPlugins();
}

void SettingsWindow::MoveUpPlugin() {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	int k= ui->list_Plugins->currentRow();
	if (k==0) return;
	int n = k-1;
	QListWidgetItem* pItem = ui->list_Plugins->takeItem(k);
	ui->list_Plugins->insertItem(n, pItem);
	ui->list_Plugins->setCurrentRow(n);
	savePriorityPlugin();

}
void SettingsWindow::MoveDownPlugin() {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	int k= ui->list_Plugins->currentRow();
	if (k==ui->list_Plugins->count()-1) return;
	int n = k+1;
	QListWidgetItem* pItem = ui->list_Plugins->takeItem(k);
	ui->list_Plugins->insertItem(n, pItem);
	ui->list_Plugins->setCurrentRow(n);
	savePriorityPlugin();
}

void SettingsWindow::savePriorityPlugin()
{
	int count = ui->list_Plugins->count();
	for(int index = 0; index < count; index++)
	{
		QListWidgetItem * item = ui->list_Plugins->item(index);
		int indexPlugin =item->data(Qt::UserRole).toUInt();
		_plugins[indexPlugin]->setPriority(count - index);
	}
}

QString SettingsWindow::getPluginName(const Plugin *plugin) const
{
	if (plugin->state() == QProcess::Running) {
		return plugin->Name().append(" (running)");
	} else {
		return plugin->Name().append(" (not running)");
	}
}

void SettingsWindow::on_pbRunConfigurationWizard_clicked()
{
#ifdef Q_OS_WIN
	QString cmdLine;
	cmdLine.append("\"");
	cmdLine.append(QApplication::applicationFilePath());
	cmdLine.append("\"");
	cmdLine.append(" --wizard");
	QProcess::startDetached(cmdLine);
#else
	QProcess::startDetached(QApplication::applicationFilePath().append(" --wizard"));
#endif

	quit();
}

void SettingsWindow::onKeepLightsAfterExit_Toggled(bool isEnabled)
{
	Settings::setKeepLightsOnAfterExit(isEnabled);
}

void SettingsWindow::onKeepLightsAfterLock_Toggled(bool isEnabled)
{
	Settings::setKeepLightsOnAfterLock(isEnabled);
}

void SettingsWindow::onKeepLightsAfterSuspend_Toggled(bool isEnabled)
{
	Settings::setKeepLightsOnAfterSuspend(isEnabled);
}

void SettingsWindow::onCheckBox_checkForUpdates_Toggled(bool isEnabled)
{
	Settings::setCheckForUpdatesEnabled(isEnabled);
	ui->checkBox_installUpdates->setEnabled(isEnabled);
}

void SettingsWindow::onCheckBox_installUpdates_Toggled(bool isEnabled)
{
	Settings::setInstallUpdatesEnabled(isEnabled);
}

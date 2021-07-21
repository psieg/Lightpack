/*
 * LedDeviceManager.cpp
 *
 *	Created on: 17.04.2011
 *		Author: Timur Sattarov && Mike Shatohin
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

#include <qglobal.h>

#include "LedDeviceManager.hpp"
#include "LedDeviceLightpack.hpp"

#ifdef Q_OS_WIN
#include "LedDeviceAlienFx.hpp"
#endif

#include "LedDeviceAdalight.hpp"
#include "LedDeviceArdulight.hpp"
#include "LedDeviceVirtual.hpp"
#include "LedDeviceDrgb.hpp"
#include "LedDeviceDnrgb.hpp"
#include "LedDeviceWarls.hpp"
#include "Settings.hpp"
#include "SystemSession.hpp"

using namespace SettingsScope;

LedDeviceManager::LedDeviceManager(QObject *parent)
	: QObject(parent)
{
	m_isLastCommandCompleted = true;

	m_ledDeviceThread = new QThread();

	m_backlightStatus = Backlight::StatusOn;

	m_isColorsSaved = false;

	m_cmdTimeoutTimer = NULL;

	m_recreateTimer = NULL;

	m_failedCreationAttempts = 0;

	m_savedBrightness = SettingsScope::Profile::Device::BrightnessDefault;

	m_savedBrightnessCap = SettingsScope::Profile::Device::BrightnessCapDefault;

	m_ledDevices.reserve(SupportedDevices::DeviceTypesCount);
	for (int i = 0; i < SupportedDevices::DeviceTypesCount; i++)
		m_ledDevices.append(NULL);
}

LedDeviceManager::~LedDeviceManager()
{
	m_ledDeviceThread->quit();
	m_ledDeviceThread->wait();

	for (int i = 0; i < m_ledDevices.size(); i++) {
		if(m_ledDevices[i])
			m_ledDevices[i]->close();
	}

	if (m_cmdTimeoutTimer)
		delete m_cmdTimeoutTimer;
}

void LedDeviceManager::init()
{
	if (!m_cmdTimeoutTimer) {
		m_cmdTimeoutTimer = new QTimer(this);
		// for some reason at 100ms this times out when ajusting white balance from settings (not the wizard) with UDP
		// also seems to depend on grab interval: the longer the more chances of timing out
		using namespace std::chrono_literals;
		m_cmdTimeoutTimer->setInterval(500ms);
		connect(m_cmdTimeoutTimer, &QTimer::timeout, this, &LedDeviceManager::ledDeviceCommandTimedOut);
	}

	if (!m_recreateTimer) {
		m_recreateTimer = new QTimer(this);
		m_recreateTimer->setSingleShot(true);
		connect(m_recreateTimer, &QTimer::timeout, this, &LedDeviceManager::recreateLedDevice);
	}

	initLedDevice();
}

void LedDeviceManager::recreateLedDevice()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	disconnectSignalSlotsLedDevice();

	initLedDevice();
}

void LedDeviceManager::switchOnLeds()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_backlightStatus = Backlight::StatusOn;
	if (m_isColorsSaved)
		emit ledDeviceSetColors(m_savedColors);
}

void LedDeviceManager::setColors(const QList<QRgb> & colors)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted
					<< " m_backlightStatus = " << m_backlightStatus;

	if (m_backlightStatus == Backlight::StatusOn)
	{
		m_savedColors = colors;
		m_isColorsSaved = true;
		if (m_isLastCommandCompleted)
		{
			m_cmdTimeoutTimer->start();
			m_isLastCommandCompleted = false;
			emit ledDeviceSetColors(colors);
		} else {
			cmdQueueAppend(LedDeviceCommands::SetColors);
		}
	}
}

void LedDeviceManager::switchOffLeds()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_cmdTimeoutTimer->start();
		m_isLastCommandCompleted = false;
		processOffLeds();
	} else {
		cmdQueueAppend(LedDeviceCommands::OffLeds);
	}
}

void LedDeviceManager::processOffLeds()
{
	m_backlightStatus = Backlight::StatusOff;

	emit ledDeviceOffLeds();
}

void LedDeviceManager::setUsbPowerLedDisabled(bool isDisabled) {
	DEBUG_MID_LEVEL << Q_FUNC_INFO << isDisabled
		<< "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted) {
		m_cmdTimeoutTimer->start();
		m_isLastCommandCompleted = false;
		emit ledDeviceSetUsbPowerLedDisabled(isDisabled);
	} else {
		m_savedUsbPowerLedDisabled = isDisabled;
		cmdQueueAppend(LedDeviceCommands::SetUsbPowerLedDisabled);
	}
}

void LedDeviceManager::setRefreshDelay(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value
					<< "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_cmdTimeoutTimer->start();
		m_isLastCommandCompleted = false;
		emit ledDeviceSetRefreshDelay(value);
	} else {
		m_savedRefreshDelay = value;
		cmdQueueAppend(LedDeviceCommands::SetRefreshDelay);
	}
}

void LedDeviceManager::setColorDepth(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_cmdTimeoutTimer->start();
		m_isLastCommandCompleted = false;
		emit ledDeviceSetColorDepth(value);
	} else {
		m_savedColorDepth = value;
		cmdQueueAppend(LedDeviceCommands::SetColorDepth);
	}
}

void LedDeviceManager::setSmoothSlowdown(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetSmoothSlowdown(value);
	} else {
		m_savedSmoothSlowdown = value;
		cmdQueueAppend(LedDeviceCommands::SetSmoothSlowdown);
	}
}

void LedDeviceManager::setGamma(double value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetGamma(value, m_backlightStatus != Backlight::StatusOff);
	} else {
		m_savedGamma = value;
		cmdQueueAppend(LedDeviceCommands::SetGamma);
	}
}

void LedDeviceManager::setBrightness(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetBrightness(value, m_backlightStatus != Backlight::StatusOff);
	} else {
		m_savedBrightness = value;
		cmdQueueAppend(LedDeviceCommands::SetBrightness);
	}
}

void LedDeviceManager::setBrightnessCap(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetBrightnessCap(value, m_backlightStatus != Backlight::StatusOff);
	}
	else {
		m_savedBrightnessCap = value;
		cmdQueueAppend(LedDeviceCommands::SetBrightnessCap);
	}
}

void LedDeviceManager::setLuminosityThreshold(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetLuminosityThreshold(value, m_backlightStatus != Backlight::StatusOff);
	} else {
		m_savedLuminosityThreshold = value;
		cmdQueueAppend(LedDeviceCommands::SetLuminosityThreshold);
	}
}

void LedDeviceManager::setMinimumLuminosityEnabled(bool value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetMinimumLuminosityEnabled(value, m_backlightStatus != Backlight::StatusOff);
	} else {
		m_savedIsMinimumLuminosityEnabled = value;
		cmdQueueAppend(LedDeviceCommands::SetMinimumLuminosityEnabled);
	}
}

void LedDeviceManager::setDitheringEnabled(bool isEnabled) {
	DEBUG_MID_LEVEL << Q_FUNC_INFO << isEnabled
		<< "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted) {
		m_cmdTimeoutTimer->start();
		m_isLastCommandCompleted = false;
		emit ledDeviceSetDitheringEnabled(isEnabled, m_backlightStatus != Backlight::StatusOff);
	}
	else {
		m_savedDitheringEnabled = isEnabled;
		cmdQueueAppend(LedDeviceCommands::SetDitheringEnabled);
	}
}

void LedDeviceManager::setColorSequence(const QString& value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceSetColorSequence(value);
	} else {
		m_savedColorSequence = value;
		cmdQueueAppend(LedDeviceCommands::SetColorSequence);
	}
}

void LedDeviceManager::requestFirmwareVersion()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceRequestFirmwareVersion();
	} else {
		cmdQueueAppend(LedDeviceCommands::RequestFirmwareVersion);
	}
}

void LedDeviceManager::updateDeviceSettings()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceUpdateDeviceSettings();
	} else {
		cmdQueueAppend(LedDeviceCommands::UpdateDeviceSettings);
	}
}

void LedDeviceManager::updateWBAdjustments()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	if (m_isLastCommandCompleted)
	{
		m_isLastCommandCompleted = false;
		m_cmdTimeoutTimer->start();
		emit ledDeviceUpdateWBAdjustments();
	} else {
		cmdQueueAppend(LedDeviceCommands::UpdateWBAdjustments);
	}
}

void LedDeviceManager::ledDeviceCommandCompleted(bool ok)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << ok;

	m_cmdTimeoutTimer->stop();

	if (ok)
	{
		if (m_cmdQueue.isEmpty() == false)
			cmdQueueProcessNext();
		else
			m_isLastCommandCompleted = true;
	}
	else
	{
		m_cmdQueue.clear();
		m_isLastCommandCompleted = true;
	}

	emit ioDeviceSuccess(ok);
}

void LedDeviceManager::ledDeviceOpenDeviceSuccess(bool isSuccess)
{
	if (isSuccess) {
		m_failedCreationAttempts = 0;
	} else {
		m_failedCreationAttempts++;
		triggerRecreateLedDevice();
	}
	emit openDeviceSuccess(isSuccess);
}

void LedDeviceManager::ledDeviceIoDeviceSuccess(bool isSuccess)
{
	if (!isSuccess) {
		triggerRecreateLedDevice();
	}
	emit ioDeviceSuccess(isSuccess);
}


void LedDeviceManager::triggerRecreateLedDevice()
{
	if (!m_recreateTimer->isActive()) {
		using namespace std::chrono_literals;
		if (m_failedCreationAttempts == 0) {
			m_recreateTimer->setInterval(200ms);
		} else if (m_failedCreationAttempts == 1) {
			m_recreateTimer->setInterval(500ms);
		} else if (m_failedCreationAttempts < 10) {
			m_recreateTimer->setInterval(1s);
		} else {
			m_recreateTimer->setInterval(5s);
		}
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Will try to recreate device in" << m_recreateTimer->interval() << "msec";
		m_recreateTimer->start();
	}
}

void LedDeviceManager::initLedDevice()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_isLastCommandCompleted = true;

	SupportedDevices::DeviceType connectedDevice = Settings::getConnectedDevice();

	if (m_ledDevices[connectedDevice] == NULL)
	{
		m_ledDevice = m_ledDevices[connectedDevice] = createLedDevice(connectedDevice);

		connectSignalSlotsLedDevice();

		m_ledDevice->moveToThread(m_ledDeviceThread);
		m_ledDeviceThread->start();
	} else {
		disconnectSignalSlotsLedDevice();

		m_ledDevice = m_ledDevices[connectedDevice];

		connectSignalSlotsLedDevice();
	}
	emit ledDeviceUpdateDeviceSettings();
	emit ledDeviceOpen();
}

AbstractLedDevice * LedDeviceManager::createLedDevice(SupportedDevices::DeviceType deviceType)
{

	if (deviceType == SupportedDevices::DeviceTypeAlienFx){
#		if !defined(Q_OS_WIN)
		qWarning() << Q_FUNC_INFO << "AlienFx not supported on current platform";

		Settings::setConnectedDevice(SupportedDevices::DefaultDeviceType);
		deviceType = Settings::getConnectedDevice();
#		endif /* Q_OS_WIN */
	}

	AbstractLedDevice* device = nullptr;

	switch (deviceType) {

	case SupportedDevices::DeviceTypeLightpack:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::LightpackDevice";
		device = (AbstractLedDevice *)new LedDeviceLightpack();
		break;

	case SupportedDevices::DeviceTypeAlienFx:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::AlienFxDevice";

#		ifdef Q_OS_WIN
		device = (AbstractLedDevice *)new LedDeviceAlienFx();
#		endif /* Q_OS_WIN */
		break;

	case SupportedDevices::DeviceTypeAdalight:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::AdalightDevice";
		device = (AbstractLedDevice *)new LedDeviceAdalight(Settings::getAdalightSerialPortName(), Settings::getAdalightSerialPortBaudRate());
		break;

	case SupportedDevices::DeviceTypeArdulight:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::ArdulightDevice";
		device = (AbstractLedDevice *)new LedDeviceArdulight(Settings::getArdulightSerialPortName(), Settings::getArdulightSerialPortBaudRate());
		break;

	case SupportedDevices::DeviceTypeDrgb:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::DrgbDevice";
		device = (AbstractLedDevice*)new LedDeviceDrgb(Settings::getDrgbAddress(), Settings::getDrgbPort(), Settings::getDrgbTimeout());
		break;

	case SupportedDevices::DeviceTypeDnrgb:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::DnrgbDevice";
		device = (AbstractLedDevice*)new LedDeviceDnrgb(Settings::getDnrgbAddress(), Settings::getDnrgbPort(), Settings::getDnrgbTimeout());
		break;

	case SupportedDevices::DeviceTypeWarls:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::WarlsDevice";
		device = (AbstractLedDevice*)new LedDeviceWarls(Settings::getWarlsAddress(), Settings::getWarlsPort(), Settings::getWarlsTimeout());
		break;

	case SupportedDevices::DeviceTypeVirtual:
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "SupportedDevices::VirtualDevice";
		device = (AbstractLedDevice *)new LedDeviceVirtual();
		break;

	default:
		break;
	}

	if (device) {
		device->setLedMilliAmps(Settings::getDeviceLedMilliAmps(deviceType));
		device->setPowerSupplyAmps(Settings::getDevicePowerSupplyAmps(deviceType));
		return device;
	}

	qFatal("%s %s%d%s", Q_FUNC_INFO,
			"Create LedDevice fail. deviceType = '", deviceType,
			"'. Application exit.");

	return nullptr; // Avoid compiler warning
}

void LedDeviceManager::connectSignalSlotsLedDevice()
{
	if (m_ledDevice == NULL)
	{
		qWarning() << Q_FUNC_INFO << "m_ledDevice == NULL";
		return;
	}

	connect(m_ledDevice, &AbstractLedDevice::commandCompleted,			this, &LedDeviceManager::ledDeviceCommandCompleted,				Qt::QueuedConnection);
	connect(m_ledDevice, &AbstractLedDevice::ioDeviceSuccess,				this, &LedDeviceManager::ledDeviceIoDeviceSuccess,					Qt::QueuedConnection);
	connect(m_ledDevice, &AbstractLedDevice::openDeviceSuccess,			this, &LedDeviceManager::ledDeviceOpenDeviceSuccess,				Qt::QueuedConnection);

	connect(m_ledDevice, &AbstractLedDevice::firmwareVersion,			this, &LedDeviceManager::firmwareVersion,						Qt::QueuedConnection);
	connect(m_ledDevice, &AbstractLedDevice::firmwareVersionUnofficial,	this, &LedDeviceManager::firmwareVersionUnofficial,				Qt::QueuedConnection);
	connect(m_ledDevice, &AbstractLedDevice::colorsUpdated,		this, &LedDeviceManager::setColors_VirtualDeviceCallback,	Qt::QueuedConnection);

	connect(this, &LedDeviceManager::ledDeviceOpen,								m_ledDevice, &AbstractLedDevice::open,										Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetColors,				m_ledDevice, &AbstractLedDevice::setColors,						Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceOffLeds,							m_ledDevice, &AbstractLedDevice::switchOffLeds,								Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetUsbPowerLedDisabled,		m_ledDevice, &AbstractLedDevice::setUsbPowerLedDisabled,				Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetRefreshDelay,				m_ledDevice, &AbstractLedDevice::setRefreshDelay,						Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetColorDepth,					m_ledDevice, &AbstractLedDevice::setColorDepth,							Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetSmoothSlowdown,				m_ledDevice, &AbstractLedDevice::setSmoothSlowdown,						Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetGamma,				m_ledDevice, &AbstractLedDevice::setGamma,						Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetBrightness,			m_ledDevice, &AbstractLedDevice::setBrightness,					Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetBrightnessCap,			m_ledDevice, &AbstractLedDevice::setBrightnessCap,					Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetColorSequence,			m_ledDevice, &AbstractLedDevice::setColorSequence,					Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetLuminosityThreshold,	m_ledDevice, &AbstractLedDevice::setLuminosityThreshold,			Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetMinimumLuminosityEnabled,	m_ledDevice, &AbstractLedDevice::setMinimumLuminosityThresholdEnabled,	Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceSetDitheringEnabled,			m_ledDevice, &AbstractLedDevice::setDitheringEnabled,					Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceRequestFirmwareVersion,			m_ledDevice, &AbstractLedDevice::requestFirmwareVersion,					Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceUpdateWBAdjustments,				m_ledDevice, qOverload<>(&AbstractLedDevice::updateWBAdjustments),						Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceUpdateDeviceSettings,				m_ledDevice, &AbstractLedDevice::updateDeviceSettings,						Qt::QueuedConnection);
}

void LedDeviceManager::disconnectSignalSlotsLedDevice()
{
	if (m_ledDevice == NULL)
	{
		qWarning() << Q_FUNC_INFO << "m_ledDevice == NULL";
		return;
	}
	m_ledDevice->disconnect(this, nullptr, nullptr, nullptr);
	disconnect(m_ledDevice, nullptr, nullptr, nullptr);
}

void LedDeviceManager::cmdQueueAppend(LedDeviceCommands::Cmd cmd)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << cmd;

	if (m_cmdQueue.contains(cmd) == false)
	{
		m_cmdQueue.append(cmd);
	}
}

void LedDeviceManager::cmdQueueProcessNext()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << m_cmdQueue;

	if (m_cmdQueue.isEmpty() == false)
	{
		LedDeviceCommands::Cmd cmd = m_cmdQueue.takeFirst();

		DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "processing cmd = " << cmd;

		switch(cmd)
		{
		case LedDeviceCommands::OffLeds:
			m_cmdTimeoutTimer->start();
			processOffLeds();
			break;

		case LedDeviceCommands::SetColors:
			if (m_isColorsSaved) {
				m_cmdTimeoutTimer->start();
				emit ledDeviceSetColors(m_savedColors);
			}
			break;

		case LedDeviceCommands::SetUsbPowerLedDisabled:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetUsbPowerLedDisabled(m_savedUsbPowerLedDisabled);
			break;

		case LedDeviceCommands::SetRefreshDelay:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetRefreshDelay(m_savedRefreshDelay);
			break;

		case LedDeviceCommands::SetColorDepth:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetColorDepth(m_savedColorDepth);
			break;

		case LedDeviceCommands::SetSmoothSlowdown:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetSmoothSlowdown(m_savedSmoothSlowdown);
			break;

		case LedDeviceCommands::SetGamma:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetGamma(m_savedGamma, m_backlightStatus != Backlight::StatusOff);
			break;

		case LedDeviceCommands::SetBrightness:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetBrightness(m_savedBrightness, m_backlightStatus != Backlight::StatusOff);
			break;

		case LedDeviceCommands::SetBrightnessCap:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetBrightnessCap(m_savedBrightnessCap, m_backlightStatus != Backlight::StatusOff);
			break;

		case LedDeviceCommands::SetColorSequence:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetColorSequence(m_savedColorSequence);
			break;

		case LedDeviceCommands::SetLuminosityThreshold:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetLuminosityThreshold(m_savedLuminosityThreshold, m_backlightStatus != Backlight::StatusOff);
			break;

		case LedDeviceCommands::SetMinimumLuminosityEnabled:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetMinimumLuminosityEnabled(m_savedIsMinimumLuminosityEnabled, m_backlightStatus != Backlight::StatusOff);
			break;

		case LedDeviceCommands::SetDitheringEnabled:
			m_cmdTimeoutTimer->start();
			emit ledDeviceSetDitheringEnabled(m_savedDitheringEnabled, m_backlightStatus != Backlight::StatusOff);
			break;

		case LedDeviceCommands::RequestFirmwareVersion:
			m_cmdTimeoutTimer->start();
			emit ledDeviceRequestFirmwareVersion();
			break;

		case LedDeviceCommands::UpdateDeviceSettings:
			m_cmdTimeoutTimer->start();
			emit ledDeviceUpdateDeviceSettings();
			break;

		case LedDeviceCommands::UpdateWBAdjustments:
			m_cmdTimeoutTimer->start();
			emit ledDeviceUpdateWBAdjustments();
			break;

		default:
			qCritical() << Q_FUNC_INFO << "fail process cmd =" << cmd;
			break;
		}
	}
}

void LedDeviceManager::ledDeviceCommandTimedOut()
{
	ledDeviceCommandCompleted(false);
	emit ioDeviceSuccess(false);
}

void LedDeviceManager::onSessionChange(SystemSession::Status status)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "status:" << status;

	if (status == SystemSession::Status::Sleeping)
		m_ledDevice->close();
	else if (status == SystemSession::Status::Resuming)
		recreateLedDevice();
}
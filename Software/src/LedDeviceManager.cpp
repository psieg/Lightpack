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
	m_ledDeviceThread = new QThread();

	m_ledDevices.reserve(SupportedDevices::DeviceTypesCount);
	for (int i = 0; i < SupportedDevices::DeviceTypesCount; i++)
		m_ledDevices.append(nullptr);
}

LedDeviceManager::~LedDeviceManager()
{
	for (int i = 0; i < m_ledDevices.size(); i++) {
		if (m_ledDevices[i]) {
			disconnect(m_ledDevices[i], nullptr, this, nullptr);
			disconnect(this, nullptr, m_ledDevices[i], nullptr);
		}
	}

	if (m_ledDeviceThread && m_ledDeviceThread->isRunning()) {
		if (QThread::currentThread() != m_ledDeviceThread) {
			for (int i = 0; i < m_ledDevices.size(); i++) {
				if (m_ledDevices[i])
					QMetaObject::invokeMethod(m_ledDevices[i], "close", Qt::BlockingQueuedConnection);
			}
		}
		m_ledDeviceThread->quit();
		m_ledDeviceThread->wait();
	}

	for (int i = 0; i < m_ledDevices.size(); i++) {
		delete m_ledDevices[i];
		m_ledDevices[i] = nullptr;
	}

	delete m_ledDeviceThread;
	m_ledDeviceThread = nullptr;
}

void LedDeviceManager::init()
{
	if (!m_cmdTimeoutTimer) {
		m_cmdTimeoutTimer = new QTimer(this);
		m_cmdTimeoutTimer->setSingleShot(true);
		// for some reason at 100ms this times out when ajusting white balance from settings (not the wizard) with UDP
		// also seems to depend on grab interval: the longer the more chances of timing out
		m_cmdTimeoutTimer->setInterval(m_cmdTimeoutMs);
		connect(m_cmdTimeoutTimer, &QTimer::timeout, this, &LedDeviceManager::ledDeviceCommandTimedOut);
	}

	if (!m_openTimeoutTimer) {
		m_openTimeoutTimer = new QTimer(this);
		m_openTimeoutTimer->setSingleShot(true);
		m_openTimeoutTimer->setInterval(m_openTimeoutMs);
		connect(m_openTimeoutTimer, &QTimer::timeout, this, &LedDeviceManager::ledDeviceOpenTimedOut);
	}

	if (!m_recreateTimer) {
		m_recreateTimer = new QTimer(this);
		m_recreateTimer->setSingleShot(true);
		connect(m_recreateTimer, &QTimer::timeout, this, &LedDeviceManager::onRecreateTimerTimeout);
	}

	initLedDevice();
}

void LedDeviceManager::onRecreateTimerTimeout()
{
	if (m_isSleeping) {
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Skipping timer recreate while sleeping";
		return;
	}

	recreateLedDevice();
}

void LedDeviceManager::recreateLedDevice()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_isSleeping = false;

	if (m_recreateTimer)
		m_recreateTimer->stop();

	initLedDevice();
}

void LedDeviceManager::switchOnLeds()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_backlightStatus = Backlight::StatusOn;
	m_cmdQueue.removeAll(LedDeviceCommands::OffLeds);
	if (m_isSleeping) {
		m_isSleeping = false;
		m_failedCreationAttempts = 0;
		recreateLedDevice();
		return;
	}
	if (m_isColorsSaved)
		enqueueOrSendCommand(LedDeviceCommands::SetColors);
}

void LedDeviceManager::setColors(const QList<QRgb> & colors)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted
					<< " m_backlightStatus = " << m_backlightStatus;

	if (m_backlightStatus == Backlight::StatusOn)
	{
		m_savedColors = colors;
		m_isColorsSaved = true;
		enqueueOrSendCommand(LedDeviceCommands::SetColors);
	}
}

void LedDeviceManager::switchOffLeds()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	m_backlightStatus = Backlight::StatusOff;
	m_cmdQueue.removeAll(LedDeviceCommands::SetColors);
	enqueueOrSendCommand(LedDeviceCommands::OffLeds);
}

void LedDeviceManager::setUsbPowerLedDisabled(bool isDisabled) {
	DEBUG_MID_LEVEL << Q_FUNC_INFO << isDisabled
		<< "Is last command completed:" << m_isLastCommandCompleted;

	m_savedUsbPowerLedDisabled = isDisabled;
	enqueueOrSendCommand(LedDeviceCommands::SetUsbPowerLedDisabled);
}

void LedDeviceManager::setRefreshDelay(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value
					<< "Is last command completed:" << m_isLastCommandCompleted;

	m_savedRefreshDelay = value;
	enqueueOrSendCommand(LedDeviceCommands::SetRefreshDelay);
}

void LedDeviceManager::setColorDepth(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedColorDepth = value;
	enqueueOrSendCommand(LedDeviceCommands::SetColorDepth);
}

void LedDeviceManager::setSmoothSlowdown(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedSmoothSlowdown = value;
	enqueueOrSendCommand(LedDeviceCommands::SetSmoothSlowdown);
}

void LedDeviceManager::setGamma(double value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedGamma = value;
	enqueueOrSendCommand(LedDeviceCommands::SetGamma);
}

void LedDeviceManager::setBrightness(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedBrightness = value;
	enqueueOrSendCommand(LedDeviceCommands::SetBrightness);
}

void LedDeviceManager::setBrightnessCap(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedBrightnessCap = value;
	enqueueOrSendCommand(LedDeviceCommands::SetBrightnessCap);
}

void LedDeviceManager::setLuminosityThreshold(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedLuminosityThreshold = value;
	enqueueOrSendCommand(LedDeviceCommands::SetLuminosityThreshold);
}

void LedDeviceManager::setMinimumLuminosityEnabled(bool value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedIsMinimumLuminosityEnabled = value;
	enqueueOrSendCommand(LedDeviceCommands::SetMinimumLuminosityEnabled);
}

void LedDeviceManager::setDitheringEnabled(bool isEnabled) {
	DEBUG_MID_LEVEL << Q_FUNC_INFO << isEnabled
		<< "Is last command completed:" << m_isLastCommandCompleted;

	m_savedDitheringEnabled = isEnabled;
	enqueueOrSendCommand(LedDeviceCommands::SetDitheringEnabled);
}

void LedDeviceManager::setColorSequence(const QString& value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << value << "Is last command completed:" << m_isLastCommandCompleted;

	m_savedColorSequence = value;
	enqueueOrSendCommand(LedDeviceCommands::SetColorSequence);
}

void LedDeviceManager::requestFirmwareVersion()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	m_firmwareVersionRequested = true;
	enqueueOrSendCommand(LedDeviceCommands::RequestFirmwareVersion);
}

void LedDeviceManager::updateDeviceSettings()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	enqueueOrSendCommand(LedDeviceCommands::UpdateDeviceSettings);
}

void LedDeviceManager::updateWBAdjustments()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Is last command completed:" << m_isLastCommandCompleted;

	enqueueOrSendCommand(LedDeviceCommands::UpdateWBAdjustments);
}

void LedDeviceManager::ledDeviceCommandCompleted(bool ok)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << ok;

	if (sender() != nullptr && sender() != m_ledDevice)
		return;

	if (m_isLastCommandCompleted || m_linkState != LinkState::Ready) {
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Ignoring late/unexpected commandCompleted(" << ok << ")";
		return;
	}

	if (m_cmdTimeoutTimer)
		m_cmdTimeoutTimer->stop();

	m_isLastCommandCompleted = true;

	if (!ok) {
		handleDeviceFailure();
		return;
	}

	onCommandSucceeded();
}

void LedDeviceManager::ledDeviceOpenDeviceSuccess(bool isSuccess)
{
	AbstractLedDevice *src = qobject_cast<AbstractLedDevice *>(sender());
	if (src == nullptr)
		src = m_ledDevice;

	int &pending = m_pendingOpens[src];
	if (pending > 0) {
		--pending;
	} else if (src == m_ledDevice) {
		return;
	}

	if (src != m_ledDevice || pending > 0)
		return;

	m_consecutiveOpenTimeouts = 0;

	if (m_isSleeping || m_linkState != LinkState::Opening)
		return;

	if (m_openTimeoutTimer)
		m_openTimeoutTimer->stop();

	if (!isSuccess) {
		handleDeviceFailure();
		return;
	}

	m_linkState = LinkState::Ready;
	emit openDeviceSuccess(true);
	replayDeviceState();
}

void LedDeviceManager::ledDeviceIoDeviceSuccess(bool isSuccess)
{
	if (sender() != nullptr && sender() != m_ledDevice)
		return;

	if (m_linkState != LinkState::Ready)
		return;

	if (!isSuccess)
		handleDeviceFailure();
}

void LedDeviceManager::handleDeviceFailure()
{
	resetCommandState();
	if (m_openTimeoutTimer)
		m_openTimeoutTimer->stop();
	m_linkState = LinkState::Closed;

	emit ioDeviceSuccess(false);

	if (m_isSleeping)
		return;

	triggerRecreateLedDevice();
}

void LedDeviceManager::onCommandSucceeded()
{
	if (m_inFlightCmd == LedDeviceCommands::SetColors || m_inFlightCmd == LedDeviceCommands::OffLeds)
		m_failedCreationAttempts = 0;
	if (m_inFlightCmd == LedDeviceCommands::RequestFirmwareVersion)
		m_firmwareVersionRequested = false;

	m_inFlightCmd.reset();

	emit ioDeviceSuccess(true);

	cmdQueueProcessNext();
}

void LedDeviceManager::replayDeviceState()
{
	enqueueOrSendCommand(LedDeviceCommands::UpdateDeviceSettings);
	if (m_backlightStatus == Backlight::StatusOff) {
		enqueueOrSendCommand(LedDeviceCommands::OffLeds);
	} else if (m_isColorsSaved) {
		enqueueOrSendCommand(LedDeviceCommands::SetColors);
	}
	if (m_firmwareVersionRequested) {
		enqueueOrSendCommand(LedDeviceCommands::RequestFirmwareVersion);
	}
}

void LedDeviceManager::triggerRecreateLedDevice()
{
	if (m_recreateTimer && !m_recreateTimer->isActive()) {
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
		if (m_failedCreationAttempts < 10)
			m_failedCreationAttempts++;
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Will try to recreate device in" << m_recreateTimer->interval() << "msec";
		m_recreateTimer->start();
	}
}

void LedDeviceManager::initLedDevice()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	resetCommandState();

	SupportedDevices::DeviceType connectedDevice = Settings::getConnectedDevice();
	AbstractLedDevice *targetDevice = m_ledDevices[connectedDevice];

	if (targetDevice == nullptr)
	{
		targetDevice = m_ledDevices[connectedDevice] = createLedDevice(connectedDevice);
		targetDevice->moveToThread(m_ledDeviceThread);
		m_ledDeviceThread->start();
		// Keep openDeviceSuccess connected for the lifetime of each cached device
		// so per-device m_pendingOpens stays accurate across A->B->A switches.
		connect(targetDevice, &AbstractLedDevice::openDeviceSuccess,
				this, &LedDeviceManager::ledDeviceOpenDeviceSuccess,
				Qt::QueuedConnection);
	}

	if (m_ledDevice != targetDevice) {
		if (m_ledDevice != nullptr) {
			emit ledDeviceClose();
			disconnectSignalSlotsLedDevice();
		}
		m_consecutiveOpenTimeouts = 0;
		m_ledDevice = targetDevice;
		connectSignalSlotsLedDevice();
	} else if (m_ledDevice != nullptr) {
		// Keep signal connections intact on the same device so m_pendingOpens stays accurate
		emit ledDeviceClose();
	}

	m_linkState = LinkState::Opening;
	++m_pendingOpens[m_ledDevice];
	if (m_openTimeoutTimer)
		m_openTimeoutTimer->start();

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
		device->setLedMilliAmps(Settings::getDeviceLedMilliAmps(deviceType), false);
		device->setPowerSupplyAmps(Settings::getDevicePowerSupplyAmps(deviceType), false);
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

	connect(m_ledDevice, &AbstractLedDevice::firmwareVersion,			this, &LedDeviceManager::firmwareVersion,						Qt::QueuedConnection);
	connect(m_ledDevice, &AbstractLedDevice::firmwareVersionUnofficial,	this, &LedDeviceManager::firmwareVersionUnofficial,				Qt::QueuedConnection);
	connect(m_ledDevice, &AbstractLedDevice::colorsUpdated,		this, &LedDeviceManager::setColors_VirtualDeviceCallback,	Qt::QueuedConnection);

	connect(this, &LedDeviceManager::ledDeviceOpen,								m_ledDevice, &AbstractLedDevice::open,										Qt::QueuedConnection);
	connect(this, &LedDeviceManager::ledDeviceClose,							m_ledDevice, &AbstractLedDevice::close,										Qt::QueuedConnection);
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
	disconnect(m_ledDevice, &AbstractLedDevice::commandCompleted, this, nullptr);
	disconnect(m_ledDevice, &AbstractLedDevice::ioDeviceSuccess, this, nullptr);
	disconnect(m_ledDevice, &AbstractLedDevice::firmwareVersion, this, nullptr);
	disconnect(m_ledDevice, &AbstractLedDevice::firmwareVersionUnofficial, this, nullptr);
	disconnect(m_ledDevice, &AbstractLedDevice::colorsUpdated, this, nullptr);
	disconnect(this, nullptr, m_ledDevice, nullptr);
}

void LedDeviceManager::resetCommandState()
{
	m_cmdQueue.clear();
	if (m_cmdTimeoutTimer)
		m_cmdTimeoutTimer->stop();
	m_isLastCommandCompleted = true;
	m_inFlightCmd.reset();
}

void LedDeviceManager::enqueueOrSendCommand(LedDeviceCommands::Cmd cmd)
{
	if (m_linkState != LinkState::Ready)
		return;

	if (m_isLastCommandCompleted)
		sendCommand(cmd);
	else
		cmdQueueAppend(cmd);
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

	while (!m_cmdQueue.isEmpty() && m_linkState == LinkState::Ready && m_isLastCommandCompleted)
	{
		LedDeviceCommands::Cmd cmd = m_cmdQueue.takeFirst();
		sendCommand(cmd);
	}
}

void LedDeviceManager::sendCommand(LedDeviceCommands::Cmd cmd)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "processing cmd = " << cmd;

	if (cmd == LedDeviceCommands::SetColors && !m_isColorsSaved)
		return;

	m_inFlightCmd = cmd;
	m_isLastCommandCompleted = false;
	if (m_cmdTimeoutTimer)
		m_cmdTimeoutTimer->start();

	switch(cmd)
	{
	case LedDeviceCommands::OffLeds:
		emit ledDeviceOffLeds();
		break;

	case LedDeviceCommands::SetColors:
		emit ledDeviceSetColors(m_savedColors);
		break;

	case LedDeviceCommands::SetUsbPowerLedDisabled:
		emit ledDeviceSetUsbPowerLedDisabled(m_savedUsbPowerLedDisabled);
		break;

	case LedDeviceCommands::SetRefreshDelay:
		emit ledDeviceSetRefreshDelay(m_savedRefreshDelay);
		break;

	case LedDeviceCommands::SetColorDepth:
		emit ledDeviceSetColorDepth(m_savedColorDepth);
		break;

	case LedDeviceCommands::SetSmoothSlowdown:
		emit ledDeviceSetSmoothSlowdown(m_savedSmoothSlowdown);
		break;

	case LedDeviceCommands::SetGamma:
		emit ledDeviceSetGamma(m_savedGamma, m_backlightStatus != Backlight::StatusOff);
		break;

	case LedDeviceCommands::SetBrightness:
		emit ledDeviceSetBrightness(m_savedBrightness, m_backlightStatus != Backlight::StatusOff);
		break;

	case LedDeviceCommands::SetBrightnessCap:
		emit ledDeviceSetBrightnessCap(m_savedBrightnessCap, m_backlightStatus != Backlight::StatusOff);
		break;

	case LedDeviceCommands::SetColorSequence:
		emit ledDeviceSetColorSequence(m_savedColorSequence);
		break;

	case LedDeviceCommands::SetLuminosityThreshold:
		emit ledDeviceSetLuminosityThreshold(m_savedLuminosityThreshold, m_backlightStatus != Backlight::StatusOff);
		break;

	case LedDeviceCommands::SetMinimumLuminosityEnabled:
		emit ledDeviceSetMinimumLuminosityEnabled(m_savedIsMinimumLuminosityEnabled, m_backlightStatus != Backlight::StatusOff);
		break;

	case LedDeviceCommands::SetDitheringEnabled:
		emit ledDeviceSetDitheringEnabled(m_savedDitheringEnabled, m_backlightStatus != Backlight::StatusOff);
		break;

	case LedDeviceCommands::RequestFirmwareVersion:
		emit ledDeviceRequestFirmwareVersion();
		break;

	case LedDeviceCommands::UpdateDeviceSettings:
		emit ledDeviceUpdateDeviceSettings();
		break;

	case LedDeviceCommands::UpdateWBAdjustments:
		emit ledDeviceUpdateWBAdjustments();
		break;

	default:
		qCritical() << Q_FUNC_INFO << "fail process cmd =" << cmd;
		m_isLastCommandCompleted = true;
		if (m_cmdTimeoutTimer)
			m_cmdTimeoutTimer->stop();
		break;
	}
}

void LedDeviceManager::ledDeviceCommandTimedOut()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Command timed out!";

	handleDeviceFailure();
}

void LedDeviceManager::ledDeviceOpenTimedOut()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Open device timed out!";

	if (++m_consecutiveOpenTimeouts >= kMaxConsecutiveOpenTimeouts) {
		qWarning() << Q_FUNC_INFO << "Resetting pending open counter after"
				   << m_consecutiveOpenTimeouts << "consecutive open timeouts";
		if (m_ledDevice)
			m_pendingOpens[m_ledDevice] = 0;
		m_consecutiveOpenTimeouts = 0;
	}

	handleDeviceFailure();
}

void LedDeviceManager::onSessionChange(SystemSession::Status status)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "status:" << status;

	if (status == SystemSession::Status::Sleeping) {
		if (m_isSleeping)
			return;
		m_isSleeping = true;
		m_consecutiveOpenTimeouts = 0;
		if (m_recreateTimer)
			m_recreateTimer->stop();
		if (m_openTimeoutTimer)
			m_openTimeoutTimer->stop();
		resetCommandState();
		m_linkState = LinkState::Closed;
		if (m_ledDevice != nullptr) {
			if (!Settings::isKeepLightsOnAfterSuspend())
				emit ledDeviceOffLeds();
			emit ledDeviceClose();
		}
	} else if (m_isSleeping && (status == SystemSession::Status::Resuming
							 || status == SystemSession::Status::ResumingAutomatic
							 || status == SystemSession::Status::Unlocking
							 || status == SystemSession::Status::DisplayOn)) {
		m_isSleeping = false;
		m_failedCreationAttempts = 0;
		recreateLedDevice();
	}
}

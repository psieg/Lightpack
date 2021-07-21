/*
 * LedDeviceManager.hpp
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

#pragma once

#include "enums.hpp"
#include "AbstractLedDevice.hpp"

class QTimer;
namespace SystemSession {
	enum Status;
};
/*!
	This class creates \a ILedDevice implementations and manages them after.
	It is always better way to interact with ILedDevice through \code LedDeviceManager \endcode.
 */
class LedDeviceManager : public QObject
{
	Q_OBJECT

public:
	explicit LedDeviceManager(QObject *parent = 0);
	virtual ~LedDeviceManager();

signals:
	void openDeviceSuccess(bool isSuccess);
	void ioDeviceSuccess(bool isSuccess);
	void firmwareVersion(const QString & fwVersion);
	void firmwareVersionUnofficial(const int version);
	void setColors_VirtualDeviceCallback(const QList<QRgb> & colors);

	// This signals are directly connected to ILedDevice. Don't use outside.
	void ledDeviceOpen();
	void ledDeviceSetColors(const QList<QRgb> & colors);
	void ledDeviceOffLeds();
	void ledDeviceSetUsbPowerLedDisabled(bool isDisabled);
	void ledDeviceSetRefreshDelay(int value);
	void ledDeviceSetColorDepth(int value);
	void ledDeviceSetSmoothSlowdown(int value);
	void ledDeviceSetGamma(double value, bool);
	void ledDeviceSetBrightness(int value, bool);
	void ledDeviceSetBrightnessCap(int value, bool);
	void ledDeviceSetLuminosityThreshold(int value, bool);
	void ledDeviceSetMinimumLuminosityEnabled(bool, bool);
	void ledDeviceSetDitheringEnabled(bool isEnabled, bool);
	void ledDeviceSetColorSequence(QString value);
	void ledDeviceRequestFirmwareVersion();
	void ledDeviceUpdateWBAdjustments();
	void ledDeviceUpdateDeviceSettings();

public slots:
	void init();

	void recreateLedDevice();

	// This slots are protected from the overflow of queries
	void setColors(const QList<QRgb> & colors);
	void switchOffLeds();
	void switchOnLeds();
	void setUsbPowerLedDisabled(bool isDisabled);
	void setRefreshDelay(int value);
	void setColorDepth(int value);
	void setSmoothSlowdown(int value);
	void setGamma(double value);
	void setBrightness(int value);
	void setBrightnessCap(int value);
	void setLuminosityThreshold(int value);
	void setMinimumLuminosityEnabled(bool value);
	void setDitheringEnabled(bool isEnabled);
	void setColorSequence(const QString& value);
	void requestFirmwareVersion();
	void updateWBAdjustments();
	void updateDeviceSettings();
	void onSessionChange(SystemSession::Status status);

private slots:
	void ledDeviceCommandCompleted(bool ok);
	void ledDeviceCommandTimedOut();
	void ledDeviceOpenDeviceSuccess(bool isSuccess);
	void ledDeviceIoDeviceSuccess(bool isSuccess);

private:
	void initLedDevice();
	AbstractLedDevice * createLedDevice(SupportedDevices::DeviceType deviceType);
	void connectSignalSlotsLedDevice();
	void disconnectSignalSlotsLedDevice();
	void cmdQueueAppend(LedDeviceCommands::Cmd);
	void cmdQueueProcessNext();
	void processOffLeds();
	void triggerRecreateLedDevice();

private:
	bool m_isLastCommandCompleted;
	bool m_isColorsSaved;
	Backlight::Status m_backlightStatus;

	QList<LedDeviceCommands::Cmd> m_cmdQueue;

	QList<QRgb> m_savedColors;
	bool m_savedUsbPowerLedDisabled;
	int m_savedRefreshDelay;
	int m_savedColorDepth;
	int m_savedSmoothSlowdown;
	double m_savedGamma;
	int m_savedBrightness;
	int m_savedBrightnessCap;
	int m_savedLuminosityThreshold;
	bool m_savedIsMinimumLuminosityEnabled;
	bool m_savedDitheringEnabled;
	QString m_savedColorSequence;

	QList<AbstractLedDevice *> m_ledDevices;
	AbstractLedDevice *m_ledDevice;
	QThread *m_ledDeviceThread;
	QTimer *m_cmdTimeoutTimer;
	QTimer *m_recreateTimer;
	int m_failedCreationAttempts;
};

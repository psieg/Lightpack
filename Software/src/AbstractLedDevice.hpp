/*
 * AbstractLedDevice.hpp
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

#include <QtGui>
#include "colorspace_types.h"
#include "types.h"
#include "SettingsDefaults.hpp"
/*!
	Abstract class representing any LED device.
	\a LedDeviceManager
*/
class AbstractLedDevice : public QObject
{
	Q_OBJECT
public:
	AbstractLedDevice(QObject * parent) : QObject(parent) {}
	virtual ~AbstractLedDevice(){}
	virtual QString name() const = 0;
	virtual int defaultLedsCount() = 0;
	virtual int maxLedsCount() = 0;

signals:
	void openDeviceSuccess(bool isSuccess);
	void ioDeviceSuccess(bool isSuccess);
	void firmwareVersion(const QString & fwVersion);
	void firmwareVersionUnofficial(const int version);

	/*!
		This signal must be sent at the completion of each command
		(setColors, setTimerOptions, setColorDepth, setSmoothSlowdown, etc.)
		\param ok is command completed successfully
	*/
	void commandCompleted(bool ok);
	void colorsUpdated(QList<QRgb> colors);

public slots:
	/*!
		Opens the underlying device.
		Contract: every call to open() MUST emit openDeviceSuccess(bool)
		exactly once, and MUST NOT emit commandCompleted(bool).
	*/
	virtual void open() = 0;
	virtual void close() = 0;
	virtual void setColors(const QList<QRgb> & colors) = 0;
	virtual void switchOffLeds() = 0;

	/*!
		\obsolete only form compatibility with Lightpack ver.<=5.5 hardware
		PWM timer period.
		\param value in millis
	*/
	virtual void setRefreshDelay(int value) = 0;
	virtual void setSmoothSlowdown(int value) = 0;
	virtual void setGamma(double value, bool updateColors = true);
	virtual void setBrightness(int value, bool updateColors = true);
	virtual void setBrightnessCap(int value, bool updateColors = true);
	virtual void setDitheringEnabled(bool value, bool updateColors = true);
	virtual void setLedMilliAmps(const int value, const bool updateColors = true);
	virtual void setPowerSupplyAmps(const double value, const bool updateColors = true);
	virtual void setColorSequence(const QString& value) = 0;
	virtual void setLuminosityThreshold(int value, bool updateColors = true);
	virtual void setMinimumLuminosityThresholdEnabled(bool value, bool updateColors = true);
	virtual void updateWBAdjustments(); // Reads from settings
	virtual void updateWBAdjustments(const QList<WBAdjustment> &coefs, bool updateColors = true);
	virtual void requestFirmwareVersion() = 0;
	virtual void updateDeviceSettings();


	/*!
		\obsolete only form compatibility with Lightpack ver.<=5.5 hardware
		\param value bits per channel
	*/
	virtual void setColorDepth(int value) = 0;


	virtual void setUsbPowerLedDisabled(bool isDisabled);

protected:
	class AckBatch
	{
	public:
		explicit AckBatch(AbstractLedDevice *device) : m_device(device)
		{
			if (m_device->m_batchDepth++ == 0)
				m_device->m_batchOk = true;
		}
		~AckBatch()
		{
			if (--m_device->m_batchDepth == 0)
				m_device->ack(m_device->m_batchOk);
		}
		AckBatch(const AckBatch &) = delete;
		AckBatch &operator=(const AckBatch &) = delete;

	private:
		AbstractLedDevice *m_device;
	};

	void ack(bool ok);

	virtual void applyColorModifications(const QList<QRgb> & inColors, QList<StructRgb> & outColors, const bool rawColors = false);
	virtual void applyDithering(QList<StructRgb>& colors, int colorDepth);

protected:
	QString m_colorSequence;
	double m_gamma{ SettingsScope::Profile::Device::GammaDefault };
	int m_brightness{ SettingsScope::Profile::Device::BrightnessDefault };
	int m_brightnessCap{ SettingsScope::Profile::Device::BrightnessCapDefault };
	int m_ledMilliAmps{ SettingsScope::Main::Device::LedMilliAmpsDefault };
	double m_powerSupplyAmps{ SettingsScope::Main::Device::PowerSupplyAmpsDefault };
	int m_luminosityThreshold{ SettingsScope::Profile::Grab::LuminosityThresholdDefault };
	bool m_isMinimumLuminosityEnabled{ SettingsScope::Profile::Grab::IsMinimumLuminosityEnabledDefault };
	bool m_isDitheringEnabled{ SettingsScope::Profile::Device::IsDitheringEnabledDefault };

	QList<WBAdjustment> m_wbAdjustments;

	QList<QRgb> m_colorsSaved;
	QList<StructRgb> m_colorsBuffer;

private:
	int m_batchDepth{ 0 };
	bool m_batchOk{ true };
};

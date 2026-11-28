/*
 * LedDeviceArdulight.hpp
 *
 *	Created on: 08.11.2011
 *		Author: Andrei Isupov
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

#include "AbstractLedDevice.hpp"
#include "colorspace_types.h"
#include <QtSerialPort/QSerialPort>

class LedDeviceArdulight : public AbstractLedDevice
{
	Q_OBJECT
public:
	LedDeviceArdulight(const QString &portName, const int baudRate, QObject * parent = 0);
	virtual ~LedDeviceArdulight();
	QString name() const { return QStringLiteral("ardulight"); }
	int maxLedsCount();
	virtual int defaultLedsCount() { return 25; }

public slots:
	void open();
	// Takes effect on the next open()
	void setSerialPortSettings(const QString &portName, int baudRate);
	void close();
	void setColors(const QList<QRgb> & /*colors*/);
	void switchOffLeds();
	void setRefreshDelay(int /*value*/);
	void setColorDepth(int /*value*/);
	void setSmoothSlowdown(int /*value*/);
	void setColorSequence(const QString& value);
	void requestFirmwareVersion();
	void updateDeviceSettings();
	void writeLastWill();
	void writeLastWill(const bool force);

private:
	bool writeColors(const QList<QRgb> & colors, bool force = false);
	bool writeBuffer(const QByteArray & buff, bool force = false);
	void resizeColorsBuffer(int buffSize);

private:
	QSerialPort *m_ArdulightDevice;

	QByteArray m_writeBufferHeader;
	QByteArray m_writeBuffer;

	QString m_portName;
	int m_baudRate;
	QTimer* m_lastWillTimer{ nullptr };
};

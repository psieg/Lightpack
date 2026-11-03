/*
 * LedDeviceAdalight.hpp
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

#include <QElapsedTimer>

#include "AbstractLedDevice.hpp"
#include "colorspace_types.h"
#include <QtSerialPort/QSerialPort>

class LedDeviceAdalight : public AbstractLedDevice
{
	Q_OBJECT
public:
	LedDeviceAdalight(const QString &portName, const int baudRate, QObject * parent = 0);
	virtual ~LedDeviceAdalight();
	QString name() const { return QStringLiteral("adalight"); }
	int maxLedsCount();
	virtual int defaultLedsCount() { return 25; }

public slots:
	void open();
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
	bool writeBuffer(const QByteArray & buff);
	void resizeColorsBuffer(int buffSize);
	void reinitBufferHeader(int ledsCount);

private:
	QSerialPort *m_AdalightDevice;

	QByteArray m_writeBufferHeader;
	QByteArray m_writeBuffer;
	QString m_portName;
	int m_baudRate;
	QTimer* m_lastWillTimer{nullptr};
	QElapsedTimer m_stuckTimer;
	QTimer* m_offRepeatTimer{nullptr};
	QByteArray m_offBuffer;
	int m_offRepeatsLeft{0};
};

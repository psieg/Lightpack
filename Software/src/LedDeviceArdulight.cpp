/*
 * LedDeviceAdalight.cpp
 *
 *	Created on: 08.11.2011
 *		Author: Isupov Andrei
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

#include "LedDeviceArdulight.hpp"
#include "PrismatikMath.hpp"
#include "Settings.hpp"
#include "debug.h"
#include "stdio.h"
#include <QtSerialPort/QSerialPortInfo>

using namespace SettingsScope;

LedDeviceArdulight::LedDeviceArdulight(const QString &portName, const int baudRate, QObject *parent) : AbstractLedDevice(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_portName = portName;
	m_baudRate = baudRate;

//	m_gamma = Settings::getDeviceGamma();
//	m_brightness = Settings::getDeviceBrightness();

	m_writeBufferHeader.append((char)255);

//	m_colorSequence = Settings::getColorSequence(SupportedDevices::DeviceTypeArdulight);
	m_ArdulightDevice = NULL;

	m_lastWillTimer = new QTimer(this);
	m_lastWillTimer->setTimerType(Qt::PreciseTimer);
	connect(m_lastWillTimer, SIGNAL(timeout()), this, SLOT(writeLastWill()));

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "initialized";
}

LedDeviceArdulight::~LedDeviceArdulight()
{
	close();
	delete m_lastWillTimer;
}

void LedDeviceArdulight::close()
{
	if (m_ArdulightDevice == NULL)
		return;

	if (m_lastWillTimer->isActive()) {
		m_lastWillTimer->stop();
		writeLastWill(true);
	}
	m_ArdulightDevice->close();

	delete m_ArdulightDevice;
	m_ArdulightDevice = NULL;
}

void LedDeviceArdulight::setColors(const QList<QRgb> & colors)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << colors;

	// Save colors for showing changes of the brightness
	m_colorsSaved = colors;

	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer);
	applyDithering(m_colorsBuffer, 8);

	for(int i=0; i < m_colorsBuffer.count(); i++) {
		PrismatikMath::maxCorrection(254, m_colorsBuffer[i]);
	}

	m_writeBuffer.clear();
	m_writeBuffer.append(m_writeBufferHeader);

	for (int i = 0; i < m_colorsBuffer.count(); i++)
	{
		StructRgb color = m_colorsBuffer[i];

		if (m_colorSequence == "RBG")
		{
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.g);
		}
		else if (m_colorSequence == "BRG")
		{
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.g);
		}
		else if (m_colorSequence == "BGR")
		{
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.r);
		}
		else if (m_colorSequence == "GRB")
		{
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.b);
		}
		else if (m_colorSequence == "GBR")
		{
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.r);
		}
		else
		{
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.b);
		}
	}

	bool ok = writeBuffer(m_writeBuffer);

	emit commandCompleted(ok);
}

void LedDeviceArdulight::switchOffLeds()
{
	int count = m_colorsSaved.count();
	m_colorsSaved.clear();

	for (int i = 0; i < count; i++)
		m_colorsSaved << 0;

	m_writeBuffer.clear();
	m_writeBuffer.append(m_writeBufferHeader);

	for (int i = 0; i < count; i++) {
		m_writeBuffer.append((char)0)
						.append((char)0)
						.append((char)0);
	}

	bool ok = writeBuffer(m_writeBuffer);

	emit commandCompleted(ok);
}

void LedDeviceArdulight::setRefreshDelay(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceArdulight::setColorDepth(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceArdulight::setSmoothSlowdown(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceArdulight::setColorSequence(QString value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	m_colorSequence = value;
	setColors(m_colorsSaved);
}

void LedDeviceArdulight::requestFirmwareVersion()
{
	emit firmwareVersion("unknown (ardulight device)");
	emit commandCompleted(true);
}

void LedDeviceArdulight::updateDeviceSettings()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	AbstractLedDevice::updateDeviceSettings();
	setColorSequence(Settings::getColorSequence(SupportedDevices::DeviceTypeArdulight));

}

void LedDeviceArdulight::open()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

//	m_gamma = Settings::getDeviceGamma();
//	m_brightness = Settings::getDeviceBrightness();

	if (m_ArdulightDevice != NULL)
		m_ArdulightDevice->close();
	else
		m_ArdulightDevice = new QSerialPort();

	m_ArdulightDevice->setPortName(m_portName);

	m_ArdulightDevice->open(QIODevice::WriteOnly);
	bool ok = m_ArdulightDevice->isOpen();

	// Ubuntu 10.04: on every second attempt to open the device leads to failure
	if (ok == false)
	{
		qWarning() << Q_FUNC_INFO << "Serial device" << m_ArdulightDevice->portName() << "open fail, will retry. Error" << (int)m_ArdulightDevice->error() << m_ArdulightDevice->errorString();
		// Try one more time
		m_ArdulightDevice->open(QIODevice::WriteOnly);
		ok = m_ArdulightDevice->isOpen();
	}

	if (ok)
	{
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Serial device" << m_ArdulightDevice->portName() << "open";

		ok = m_ArdulightDevice->setBaudRate(m_baudRate);
		if (ok)
		{
			ok = m_ArdulightDevice->setDataBits(QSerialPort::Data8);
			if (ok)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Baud rate	:" << m_ArdulightDevice->baudRate();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Data bits	:" << m_ArdulightDevice->dataBits();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Parity		:" << m_ArdulightDevice->parity();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Stop bits	:" << m_ArdulightDevice->stopBits();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Flow			:" << m_ArdulightDevice->flowControl();
			} else {
				qWarning() << Q_FUNC_INFO << "Set data bits 8 fail. Error" << (int)m_ArdulightDevice->error() << m_ArdulightDevice->errorString();
			}
		} else {
			qWarning() << Q_FUNC_INFO << "Set baud rate" << m_baudRate << "fail. Error" << (int)m_ArdulightDevice->error() << m_ArdulightDevice->errorString();
		}

	} else {
		qWarning() << Q_FUNC_INFO << "Serial device" << m_ArdulightDevice->portName() << "open fail. Error" << (int)m_ArdulightDevice->error() << m_ArdulightDevice->errorString();
		DEBUG_OUT << Q_FUNC_INFO << "Available ports:";
		QList<QSerialPortInfo> availPorts = QSerialPortInfo::availablePorts();
		for(int i=0; i < availPorts.size(); i++) {
			DEBUG_OUT << Q_FUNC_INFO << availPorts[i].portName();
		}
	}

	emit openDeviceSuccess(ok);
}

void LedDeviceArdulight::writeLastWill(const bool force)
{
	if (force || m_ArdulightDevice->bytesToWrite() == 0) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "Writing last will frame";
		setColors(m_colorsSaved);
	}
}

bool LedDeviceArdulight::writeBuffer(const QByteArray & buff)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Hex:" << buff.toHex();

	if (m_ArdulightDevice == NULL || m_ArdulightDevice->isOpen() == false)
		return false;

	if (m_ArdulightDevice->bytesToWrite() > 0) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "Serial bytesToWrite:" << m_ArdulightDevice->bytesToWrite() << ", skipping current frame";
		// If no more writes will be done ("Send data only of colors changed")
		// re-schedule last skipped frame in case it's important (for ex a black frame to turn off)
		m_lastWillTimer->start(100);
		return true;
	}
	m_lastWillTimer->stop();

	int bytesWritten = m_ArdulightDevice->write(buff);

	if (bytesWritten != buff.count())
	{
		qWarning() << Q_FUNC_INFO << "bytesWritten != buff.count():" << bytesWritten << buff.count() << " " << m_ArdulightDevice->errorString();
		return false;
	}

	return true;
}

void LedDeviceArdulight::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	m_colorsBuffer.clear();

	if (buffSize > MaximumNumberOfLeds::Ardulight)
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Ardulight" << buffSize << ">" << MaximumNumberOfLeds::Ardulight;

		buffSize = MaximumNumberOfLeds::Ardulight;
	}

	for (int i = 0; i < buffSize; i++)
	{
		m_colorsBuffer << StructRgb();
	}
}


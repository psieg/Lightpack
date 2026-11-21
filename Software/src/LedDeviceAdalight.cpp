/*
 * LedDeviceAdalight.cpp
 *
 *	Created on: 06.09.2011
 *		Author: Mike Shatohin (brunql)
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

#include "LedDeviceAdalight.hpp"
#include "PrismatikMath.hpp"
#include "Settings.hpp"
#include "debug.h"
#include "stdio.h"
#include <QtSerialPort/QSerialPortInfo>

using namespace SettingsScope;

LedDeviceAdalight::LedDeviceAdalight(const QString &portName, const int baudRate, QObject *parent) : AbstractLedDevice(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_portName = portName;
	m_baudRate = baudRate;

//	m_gamma = Settings::getDeviceGamma();
//	m_brightness = Settings::getDeviceBrightness();
//	m_colorSequence =Settings::getColorSequence(SupportedDevices::DeviceTypeAdalight);
	m_AdalightDevice = NULL;
	m_lastWillTimer = new QTimer(this);
	m_lastWillTimer->setTimerType(Qt::PreciseTimer);
	connect(m_lastWillTimer, &QTimer::timeout, this, qOverload<>(&LedDeviceAdalight::writeLastWill));
	// TODO: think about init m_savedColors in all ILedDevices

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "initialized";
}

LedDeviceAdalight::~LedDeviceAdalight()
{
	close();
	delete m_lastWillTimer;
}

int LedDeviceAdalight::maxLedsCount()
{
	return MaximumNumberOfLeds::Adalight;
}

void LedDeviceAdalight::close()
{
	if (m_AdalightDevice == NULL)
		return;

	if (m_lastWillTimer->isActive()) {
		m_lastWillTimer->stop();
		writeLastWill(true);
	}
	// the forced write above may re-arm the timer if the port is busy;
	// it must not fire once the port has been deleted
	m_lastWillTimer->stop();
	m_writeStallTimer.invalidate();
	m_AdalightDevice->close();

	delete m_AdalightDevice;
	m_AdalightDevice = NULL;
}

void LedDeviceAdalight::setColors(const QList<QRgb> & colors)
{
	// Save colors for showing changes of the brightness
	m_colorsSaved = colors;

	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer);
	applyDithering(m_colorsBuffer, 8);

	m_writeBuffer.clear();
	m_writeBuffer.append(m_writeBufferHeader);

	for (int i = 0; i < m_colorsBuffer.count(); i++)
	{
		StructRgb color = m_colorsBuffer[i];

		if (m_colorSequence == QStringLiteral("RBG"))
		{
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.g);
		}
		else if (m_colorSequence == QStringLiteral("BRG"))
		{
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.g);
		}
		else if (m_colorSequence == QStringLiteral("BGR"))
		{
			m_writeBuffer.append(color.b);
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.r);
		}
		else if (m_colorSequence == QStringLiteral("GRB"))
		{
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.b);
		}
		else if (m_colorSequence == QStringLiteral("GBR"))
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

void LedDeviceAdalight::switchOffLeds()
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

void LedDeviceAdalight::setRefreshDelay(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceAdalight::setColorDepth(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceAdalight::setSmoothSlowdown(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceAdalight::setColorSequence(const QString& value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	m_colorSequence = value;
	setColors(m_colorsSaved);
}

void LedDeviceAdalight::requestFirmwareVersion()
{
	emit firmwareVersion(QStringLiteral("unknown (adalight device)"));
	emit commandCompleted(true);
}

void LedDeviceAdalight::updateDeviceSettings()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	AbstractLedDevice::updateDeviceSettings();
	setColorSequence(Settings::getColorSequence(SupportedDevices::DeviceTypeAdalight));
}

void LedDeviceAdalight::open()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << sender();

//	m_gamma = Settings::getDeviceGamma();
//	m_brightness = Settings::getDeviceBrightness();

	if (m_AdalightDevice != NULL)
		m_AdalightDevice->close();
	else {
		m_AdalightDevice = new QSerialPort();
		connect(m_AdalightDevice, &QSerialPort::errorOccurred, this, &LedDeviceAdalight::onSerialError);
	}

	m_writeStallTimer.invalidate();

	m_AdalightDevice->setPortName(m_portName);// Settings::getAdalightSerialPortName());

	m_AdalightDevice->open(QIODevice::WriteOnly);
	bool ok = m_AdalightDevice->isOpen();

	// Ubuntu 10.04: on every second attempt to open the device leads to failure
	if (ok == false)
	{
		qWarning() << Q_FUNC_INFO << "Serial device" << m_AdalightDevice->portName() << "open fail, will retry. Error" << (int)m_AdalightDevice->error() << m_AdalightDevice->errorString();
		// Try one more time
		m_AdalightDevice->open(QIODevice::WriteOnly);
		ok = m_AdalightDevice->isOpen();
	}

	if (ok)
	{
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Serial device" << m_AdalightDevice->portName() << "open";
		ok = m_AdalightDevice->setBaudRate(m_baudRate);//Settings::getAdalightSerialPortBaudRate());
		if (ok)
		{
			ok = m_AdalightDevice->setDataBits(QSerialPort::Data8);
			if (ok)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Baud rate	:" << m_AdalightDevice->baudRate();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Data bits	:" << m_AdalightDevice->dataBits();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Parity		:" << m_AdalightDevice->parity();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Stop bits	:" << m_AdalightDevice->stopBits();
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Flow			:" << m_AdalightDevice->flowControl();
			} else {
				qWarning() << Q_FUNC_INFO << "Set data bits 8 fail. Error" << (int)m_AdalightDevice->error() << m_AdalightDevice->errorString();
			}
		} else {
			qWarning() << Q_FUNC_INFO << "Set baud rate" << m_baudRate << "fail. Error" << (int)m_AdalightDevice->error() << m_AdalightDevice->errorString();
		}

	} else {
		qWarning() << Q_FUNC_INFO << "Serial device" << m_AdalightDevice->portName() << "open fail. Error" << (int)m_AdalightDevice->error() << m_AdalightDevice->errorString();
		DEBUG_OUT << Q_FUNC_INFO << "Available ports:";
		QList<QSerialPortInfo> availPorts = QSerialPortInfo::availablePorts();
		for(int i=0; i < availPorts.size(); i++) {
			DEBUG_OUT << Q_FUNC_INFO << availPorts[i].portName();
		}
	}

	emit openDeviceSuccess(ok);
}

void LedDeviceAdalight::writeLastWill()
{
	writeLastWill(false);
}

void LedDeviceAdalight::writeLastWill(const bool force)
{
	if (m_AdalightDevice == NULL || m_AdalightDevice->isOpen() == false) {
		m_lastWillTimer->stop();
		return;
	}

	if (force || m_AdalightDevice->bytesToWrite() == 0) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "Writing last will frame";
		setColors(m_colorsSaved);
	} else {
		// nothing new is being sent (static image / "send only on change"),
		// so check here too whether the port is stuck
		isWriteStalled();
	}
}

bool LedDeviceAdalight::writeBuffer(const QByteArray & buff)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Hex:" << buff.toHex();

	if (m_AdalightDevice == NULL || m_AdalightDevice->isOpen() == false)
		return false;

	if (m_AdalightDevice->bytesToWrite() > 0) {
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "Serial bytesToWrite:" << m_AdalightDevice->bytesToWrite() << ", skipping current frame";
		if (isWriteStalled())
			return false;
		// If no more writes will be done ("Send data only of colors changed")
		// re-schedule last skipped frame in case it's important (for ex a black frame to turn off)
		using namespace std::chrono_literals;
		m_lastWillTimer->start(100ms);
		return true;
	}
	m_lastWillTimer->stop();
	m_writeStallTimer.invalidate();

	int bytesWritten = m_AdalightDevice->write(buff);

	if (bytesWritten != buff.count())
	{
		qWarning() << Q_FUNC_INFO << "bytesWritten != buff.count():" << bytesWritten << buff.count() << " " << m_AdalightDevice->errorString();
		emit ioDeviceSuccess(false);
		return false;
	}

	emit ioDeviceSuccess(true);
	return true;
}

bool LedDeviceAdalight::isWriteStalled()
{
	// If the OS never completes a pending write (USB-serial driver hiccup, device
	// briefly re-enumerated, etc.) bytesToWrite() stays > 0 forever and every new
	// frame gets skipped while the port still looks open: the LEDs freeze with no
	// error reported. Treat a buffer that doesn't drain in time as an I/O failure so
	// LedDeviceManager closes and reopens the port.
	static const qint64 WriteStallTimeoutMs = 1500;

	if (!m_writeStallTimer.isValid()) {
		m_writeStallTimer.start();
		return false;
	}
	if (!m_writeStallTimer.hasExpired(WriteStallTimeoutMs))
		return false;

	qWarning() << Q_FUNC_INFO << "Serial write stalled for" << m_writeStallTimer.elapsed() << "ms with"
			<< m_AdalightDevice->bytesToWrite() << "bytes pending, reopening" << m_AdalightDevice->portName();
	m_writeStallTimer.invalidate();
	m_lastWillTimer->stop();
	emit ioDeviceSuccess(false);
	return true;
}

void LedDeviceAdalight::onSerialError(QSerialPort::SerialPortError error)
{
	if (error != QSerialPort::WriteError && error != QSerialPort::ResourceError && error != QSerialPort::UnknownError)
		return;

	qWarning() << Q_FUNC_INFO << "Serial error" << (int)error << (m_AdalightDevice ? m_AdalightDevice->errorString() : QString())
			<< ", reopening port";
	m_lastWillTimer->stop();
	m_writeStallTimer.invalidate();
	emit ioDeviceSuccess(false);
}

void LedDeviceAdalight::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	m_colorsBuffer.clear();

	if (buffSize > MaximumNumberOfLeds::Adalight)
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Adalight" << buffSize << ">" << MaximumNumberOfLeds::Adalight;

		buffSize = MaximumNumberOfLeds::Adalight;
	}

	for (int i = 0; i < buffSize; i++)
	{
		m_colorsBuffer << StructRgb();
	}

	reinitBufferHeader(buffSize);
}

void LedDeviceAdalight::reinitBufferHeader(int ledsCount)
{
	m_writeBufferHeader.clear();

	// Initialize buffer header
	int ledsCountHi = ((ledsCount - 1) >> 8) & 0xff;
	int ledsCountLo = (ledsCount	- 1) & 0xff;

	m_writeBufferHeader.append((char)'A');
	m_writeBufferHeader.append((char)'d');
	m_writeBufferHeader.append((char)'a');
	m_writeBufferHeader.append((char)ledsCountHi);
	m_writeBufferHeader.append((char)ledsCountLo);
	m_writeBufferHeader.append((char)(ledsCountHi ^ ledsCountLo ^ 0x55));
}

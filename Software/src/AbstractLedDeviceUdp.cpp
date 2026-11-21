/*
 * AbstractLedDeviceUdp.cpp
 *
 *	Created on: 15.02.2020
 *		Author: Tom Archer
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

#include "AbstractLedDeviceUdp.hpp"
#include "enums.hpp"
#include "debug.h"

AbstractLedDeviceUdp::AbstractLedDeviceUdp(const QString& address, const QString& port, const uint8_t timeout, QObject * parent) : AbstractLedDevice(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_address = address;

	bool ok = false;
	// TODO: pass ushort
	m_port = port.toUShort(&ok);
	if (m_port == 0 || !ok) {
		m_port = 21324;
		qCritical() << "could not parse UDP port" << port << ". Using default" << m_port;
	}

	m_timeout = timeout;

	m_Socket = NULL;
}

AbstractLedDeviceUdp::~AbstractLedDeviceUdp()
{
	close();
}

void AbstractLedDeviceUdp::close()
{
	if (m_Socket != NULL) {
		m_Socket->close();

		delete m_Socket;
		m_Socket = NULL;
	}
}

void AbstractLedDeviceUdp::open()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if (m_Socket != NULL)
		m_Socket->close();
	else
		m_Socket = new QUdpSocket(this);

	m_Socket->connectToHost(m_address, m_port, QIODevice::WriteOnly);
	// TODO: connection slots
	// emit openDeviceSuccess(m_Socket->isValid());
	emit openDeviceSuccess(true);

	reinitBufferHeader();
}

void AbstractLedDeviceUdp::setRefreshDelay(int value)
{
	Q_UNUSED(value);
	ack(true);
}

void AbstractLedDeviceUdp::setSmoothSlowdown(int value)
{
	Q_UNUSED(value);
	ack(true);
}

void AbstractLedDeviceUdp::setColorSequence(const QString& value)
{
	Q_UNUSED(value);
	ack(true);
}

void AbstractLedDeviceUdp::setColorDepth(int value)
{
	Q_UNUSED(value);
	ack(true);
}

void AbstractLedDeviceUdp::switchOffLeds()
{
	const int count = m_colorsSaved.count();
	QList<QRgb> blackFrame;
	blackFrame.reserve(count);
	for (int i = 0; i < count; i++)
		blackFrame << 0;
	setColors(blackFrame, true);
}

void AbstractLedDeviceUdp::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	if (buffSize > maxLedsCount())
	{
		qCritical() << Q_FUNC_INFO << "buffSize > maxLedsCount()" << buffSize << ">" << maxLedsCount();

		buffSize = maxLedsCount();
	}
	m_colorsBuffer.clear();
	m_colorsBuffer.reserve(buffSize);
	for (int i = 0; i < buffSize; i++)
		m_colorsBuffer << StructRgb();
}

void AbstractLedDeviceUdp::requestFirmwareVersion()
{
	emit firmwareVersion(QStringLiteral("N/A (%1 device)").arg(name()));
	ack(true);
}

bool AbstractLedDeviceUdp::writeBuffer(const QByteArray& buff)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "Hex:" << buff.toHex();

	if (m_Socket == NULL)
		return false;

	const qint64 bytesWritten = m_Socket->write(buff.data(), buff.size());

	if (bytesWritten != buff.count())
	{
		qWarning() << Q_FUNC_INFO << "bytesWritten != buff.count():" << bytesWritten << buff.count() << " " << m_Socket->errorString();
		return false;
	}

	return true;
}

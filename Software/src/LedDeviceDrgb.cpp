/*
 * LedDeviceDrgb.cpp
 *
 *	Created on: 31.01.2020
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

#include "LedDeviceDrgb.hpp"
#include "enums.hpp"

LedDeviceDrgb::LedDeviceDrgb(const QString& address, const QString& port, const int timeout, QObject * parent) : AbstractLedDeviceUdp(address, port, timeout, parent)
{
}

const QString LedDeviceDrgb::name() const
{ 
	return "drgb"; 
}

int LedDeviceDrgb::maxLedsCount()
{
	return MaximumNumberOfLeds::Drgb;
}

void LedDeviceDrgb::setColors(const QList<QRgb> & colors)
{
	m_colorsSaved = colors;

	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer);
	applyDithering(m_colorsBuffer, 8);

	m_writeBuffer.clear();
	m_writeBuffer.append(m_writeBufferHeader);

	for (int i = 0; i < m_colorsBuffer.count(); i++)
	{
		StructRgb color = m_colorsBuffer[i];

		m_writeBuffer.append(color.r);
		m_writeBuffer.append(color.g);
		m_writeBuffer.append(color.b);
	}
	
	bool ok = writeBuffer(m_writeBuffer);
	emit commandCompleted(ok);
}

void LedDeviceDrgb::switchOffLeds()
{
	int count = m_colorsSaved.count();
	m_colorsSaved.clear();

	for (int i = 0; i < count; i++) {
		m_colorsSaved << 0;
	}

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

void LedDeviceDrgb::requestFirmwareVersion()
{
	emit firmwareVersion("1.0 (drgb device)");
	emit commandCompleted(true);
}

void LedDeviceDrgb::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	m_colorsBuffer.clear();

	if (buffSize > MaximumNumberOfLeds::Drgb)
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Drgb" << buffSize << ">" << MaximumNumberOfLeds::Drgb;

		buffSize = MaximumNumberOfLeds::Drgb;
	}

	for (int i = 0; i < buffSize; i++)
	{
		m_colorsBuffer << StructRgb();
	}

	reinitBufferHeader();
}

void LedDeviceDrgb::reinitBufferHeader()
{
	m_writeBufferHeader.clear();

	// Initialize buffer header
	m_writeBufferHeader.append((char)UdpDevice::Drgb);    // DRGB protocol
	m_writeBufferHeader.append((char)m_timeout);
}

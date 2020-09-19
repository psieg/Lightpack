/*
 * LedDeviceDnrgb.cpp
 *
 *	Created on: 16.02.2020
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

#include "LedDeviceDnrgb.hpp"
#include "enums.hpp"

LedDeviceDnrgb::LedDeviceDnrgb(const QString& address, const QString& port, const int timeout, QObject * parent) : AbstractLedDeviceUdp(address, port, timeout, parent)
{
}

const QString LedDeviceDnrgb::name() const
{ 
	return "dnrgb"; 
}

int LedDeviceDnrgb::maxLedsCount()
{
	return MaximumNumberOfLeds::Dnrgb;
}

void LedDeviceDnrgb::setColors(const QList<QRgb> & colors)
{
	bool ok = true;

	m_colorsSaved = colors;

	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer);
	applyDithering(m_colorsBuffer, 8);

	// Send multiple buffers
	uint16_t remainingColors = colors.count();
	uint16_t startIndex = 0;
	uint16_t colorsToSend = 0;
		
	while (remainingColors > 0)
	{
		m_writeBuffer.clear();
		m_writeBuffer.append(m_writeBufferHeader);
		m_writeBuffer.append((char)(startIndex >> 8));  //High byte
		m_writeBuffer.append((char)startIndex);         //Low byte

		if (remainingColors > LEDS_PER_PACKET)
		{
			colorsToSend = LEDS_PER_PACKET;
		}
		else
		{
			colorsToSend = remainingColors;
		}

		for (uint16_t i = startIndex; i < startIndex + colorsToSend; i++)
		{
			StructRgb color = m_colorsBuffer[i];

			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.b);
		}

		startIndex += colorsToSend;
		remainingColors -= colorsToSend;
		ok &= writeBuffer(m_writeBuffer);
	}

	emit commandCompleted(ok);
}

void LedDeviceDnrgb::switchOffLeds()
{
	int count = m_colorsSaved.count();
	m_colorsSaved.clear();

	for (int i = 0; i < count; i++) {
		m_colorsSaved << 0;
	}

	// Send multiple buffers
	int remainingColors = count;
	int startIndex = 0;
	int colorsToSend = 0;
	bool ok = true;

	while (remainingColors > 0)
	{
		m_writeBuffer.clear();
		m_writeBuffer.append(m_writeBufferHeader);
		m_writeBuffer.append((char)startIndex >> 8);  //High byte
		m_writeBuffer.append((char)startIndex);       //Low byte

		if (remainingColors > LEDS_PER_PACKET)
		{
			colorsToSend = LEDS_PER_PACKET;
		}
		else
		{
			colorsToSend = remainingColors;
		}

		for (int i = startIndex; i < startIndex + colorsToSend; i++)
		{
			m_writeBuffer.append((char)0)
				.append((char)0)
				.append((char)0);
		}

		startIndex += colorsToSend;
		remainingColors -= colorsToSend;
		ok &= writeBuffer(m_writeBuffer);
	}

	emit commandCompleted(ok);
}

void LedDeviceDnrgb::requestFirmwareVersion()
{
	emit firmwareVersion("1.0 (dnrgb device)");
	emit commandCompleted(true);
}

void LedDeviceDnrgb::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	m_colorsBuffer.clear();

	if (buffSize > MaximumNumberOfLeds::Dnrgb)
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Dnrgb" << buffSize << ">" << MaximumNumberOfLeds::Dnrgb;

		buffSize = MaximumNumberOfLeds::Dnrgb;
	}

	for (int i = 0; i < buffSize; i++)
	{
		m_colorsBuffer << StructRgb();
	}

	reinitBufferHeader();
}

void LedDeviceDnrgb::reinitBufferHeader()
{
	m_writeBufferHeader.clear();

	// Initialize buffer header
	m_writeBufferHeader.append((char)UdpDevice::Dnrgb);    // DNRGB protocol
	m_writeBufferHeader.append((char)m_timeout);
}

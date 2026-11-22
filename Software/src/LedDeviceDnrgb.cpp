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

LedDeviceDnrgb::LedDeviceDnrgb(const QString& address, const QString& port, const uint8_t timeout, QObject * parent) : AbstractLedDeviceUdp(address, port, timeout, parent)
{
}

QString LedDeviceDnrgb::name() const
{
	return QStringLiteral("dnrgb");
}

int LedDeviceDnrgb::maxLedsCount()
{
	return MaximumNumberOfLeds::Dnrgb;
}

void LedDeviceDnrgb::setColors(const QList<QRgb> & colors, const bool rawColors)
{
	bool ok = true;
	bool sentPackets = false;

	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer, rawColors);
	if (!rawColors)
		applyDithering(m_colorsBuffer, 8);

	// Send multiple buffers
	const int totalColorsSaved = m_processedColorsSaved.count();
	const int totalColors = colors.count();
	uint16_t startIndex = 0;
	QList<QRgb> newColors;
	newColors.reserve(colors.count());

	while (startIndex < totalColors)
	{
		// skip equals
		while (startIndex < totalColors
			&& startIndex < totalColorsSaved)
		{
			const StructRgb color = m_colorsBuffer[startIndex];
			const QRgb newColor = qRgb(color.r, color.g, color.b);
			if (m_processedColorsSaved[startIndex] != newColor)
				break;
			newColors << newColor;
			startIndex++;
		}

		// get diffs
		QByteArray colorPacket;
		uint16_t colorPacketLen = 0;
		while (colorPacketLen < LedsPerPacket
			&& startIndex + colorPacketLen < totalColors)
		{
			const StructRgb color = m_colorsBuffer[startIndex + colorPacketLen];
			const QRgb newColor = qRgb(color.r, color.g, color.b);
			if (startIndex + colorPacketLen < totalColorsSaved
				&& m_processedColorsSaved[startIndex + colorPacketLen] == newColor)
				break;

			newColors << newColor;
			colorPacket.append(color.r);
			colorPacket.append(color.g);
			colorPacket.append(color.b);

			colorPacketLen++;
		}

		if (colorPacketLen > 0) {
			m_writeBuffer.clear();
			m_writeBuffer.append(m_writeBufferHeader);
			m_writeBuffer.append((char)(startIndex >> 8));  //High byte
			m_writeBuffer.append((char)startIndex);         //Low byte
			m_writeBuffer.append(colorPacket);
			startIndex += colorPacketLen;
			ok &= writeBuffer(m_writeBuffer);
			sentPackets = true;
		}
	}

	// if no packets are sent, send empty packet to not timeout
	if (!sentPackets && m_timeout != InfiniteTimeout) {
		m_writeBuffer.clear();
		m_writeBuffer.append(m_writeBufferHeader);
		m_writeBuffer.append((char)0);
		m_writeBuffer.append((char)0);
		ok &= writeBuffer(m_writeBuffer);
	}

	m_colorsSaved = colors;
	m_processedColorsSaved = newColors;

	ack(ok);
}

void LedDeviceDnrgb::reinitBufferHeader()
{
	m_writeBufferHeader.clear();

	// Initialize buffer header
	m_writeBufferHeader.append((char)UdpDevice::Dnrgb);    // DNRGB protocol
	m_writeBufferHeader.append((char)m_timeout);
}

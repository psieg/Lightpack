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

LedDeviceDrgb::LedDeviceDrgb(const QString& address, const QString& port, const uint8_t timeout, QObject * parent) : AbstractLedDeviceUdp(address, port, timeout, parent)
{
}

QString LedDeviceDrgb::name() const
{
	return QStringLiteral("drgb");
}

int LedDeviceDrgb::maxLedsCount()
{
	return MaximumNumberOfLeds::Drgb;
}

void LedDeviceDrgb::setColors(const QList<QRgb> & colors, const bool rawColors)
{
	m_colorsSaved = colors;

	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer, rawColors);
	if (!rawColors)
		applyDithering(m_colorsBuffer, 8);

	m_writeBuffer.clear();
	m_writeBuffer.reserve(m_writeBuffer.count() + m_colorsBuffer.count() * sizeof(char));
	m_writeBuffer.append(m_writeBufferHeader);

	for (const StructRgb& color : m_colorsBuffer)
	{
		m_writeBuffer.append(color.r);
		m_writeBuffer.append(color.g);
		m_writeBuffer.append(color.b);
	}

	const bool ok = writeBuffer(m_writeBuffer);
	ack(ok);
}

void LedDeviceDrgb::reinitBufferHeader()
{
	m_writeBufferHeader.clear();

	// Initialize buffer header
	m_writeBufferHeader.append((char)UdpDevice::Drgb);    // DRGB protocol
	m_writeBufferHeader.append((char)m_timeout);
}

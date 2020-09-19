/*
 * LedDeviceWarls.cpp
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

#include "LedDeviceWarls.hpp"
#include "enums.hpp"

LedDeviceWarls::LedDeviceWarls(const QString& address, const QString& port, const int timeout, QObject * parent) : AbstractLedDeviceUdp(address, port, timeout, parent)
{
}

const QString LedDeviceWarls::name() const
{ 
	return "warls"; 
}

int LedDeviceWarls::maxLedsCount()
{
	return MaximumNumberOfLeds::Warls;
}

void LedDeviceWarls::setColors(const QList<QRgb> & colors)
{
	resizeColorsBuffer(colors.count());

	applyColorModifications(colors, m_colorsBuffer);
	applyDithering(m_colorsBuffer, 8);

	m_writeBuffer.clear();
	m_writeBuffer.append(m_writeBufferHeader);

	for (int i = 0; i < m_colorsBuffer.count(); i++)
	{
		if (colors[i] != m_colorsSaved[i])
		{
			StructRgb color = m_colorsBuffer[i];

			m_writeBuffer.append(i);
			m_writeBuffer.append(color.r);
			m_writeBuffer.append(color.g);
			m_writeBuffer.append(color.b);
		}
	}

	m_colorsSaved = colors;

	// This may send the header only
	bool ok = writeBuffer(m_writeBuffer);
	emit commandCompleted(ok);
}

void LedDeviceWarls::switchOffLeds()
{
	int count = m_colorsSaved.count();
	m_colorsSaved.clear();

	for (int i = 0; i < count; i++) {
		m_colorsSaved << 0;
	}

	m_writeBuffer.clear();
	m_writeBuffer.append(m_writeBufferHeader);

	for (int i = 0; i < count; i++) {
		m_writeBuffer.append((char)i)
			.append((char)0)
			.append((char)0)
			.append((char)0);
	}

	bool ok = writeBuffer(m_writeBuffer);
	emit commandCompleted(ok);
}

void LedDeviceWarls::requestFirmwareVersion()
{
	emit firmwareVersion("1.0 (warls device)");
	emit commandCompleted(true);
}

void LedDeviceWarls::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	m_colorsBuffer.clear();
	m_colorsSaved.clear();

	if (buffSize > MaximumNumberOfLeds::Warls)
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Warls" << buffSize << ">" << MaximumNumberOfLeds::Warls;

		buffSize = MaximumNumberOfLeds::Warls;
	}

	for (int i = 0; i < buffSize; i++)
	{
		m_colorsBuffer << StructRgb();
		m_colorsSaved << 0;
	}

	reinitBufferHeader();
}

void LedDeviceWarls::reinitBufferHeader()
{
	m_writeBufferHeader.clear();

	// Initialize buffer header
	m_writeBufferHeader.append((char)UdpDevice::Warls);    // WARLS protocol
	m_writeBufferHeader.append((char)m_timeout);
}

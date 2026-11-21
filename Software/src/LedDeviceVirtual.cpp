/*
 * LightpackVirtual.cpp
 *
 *	Created on: 06.09.2011
 *		Author: Mike Shatohin (brunql), Timur Sattarov
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

#include "LedDeviceVirtual.hpp"
#include "PrismatikMath.hpp"
#include "Settings.hpp"
#include "colorspace_types.h"
#include "enums.hpp"
#include "debug.h"

using namespace SettingsScope;

LedDeviceVirtual::LedDeviceVirtual(QObject * parent) : AbstractLedDevice(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
}

void LedDeviceVirtual::setColors(const QList<QRgb> & colors)
{
	if (!colors.isEmpty())
	{
		m_colorsSaved = colors;

		QList<QRgb> callbackColors;
		callbackColors.reserve(colors.size());

		resizeColorsBuffer(colors.count());

		applyColorModifications(colors, m_colorsBuffer);
		applyDithering(m_colorsBuffer, 8);

		for (const StructRgb& color : m_colorsBuffer)
			callbackColors.append(qRgb(color.r, color.g, color.b));

		emit colorsUpdated(callbackColors);
	}
	ack(true);
}

int LedDeviceVirtual::maxLedsCount()
{
	return MaximumNumberOfLeds::Virtual;
}

void LedDeviceVirtual::switchOffLeds()
{
	int count = m_colorsSaved.count();
	m_colorsSaved.clear();

	for (int i = 0; i < count; i++) {
		m_colorsSaved << 0;
	}
	emit colorsUpdated(m_colorsSaved);
	ack(true);
}

void LedDeviceVirtual::setRefreshDelay(int /*value*/)
{
	ack(true);
}

void LedDeviceVirtual::setColorDepth(int /*value*/)
{
	ack(true);
}

void LedDeviceVirtual::setSmoothSlowdown(int /*value*/)
{
	ack(true);
}

void LedDeviceVirtual::setColorSequence(const QString& /*value*/)
{
	ack(true);
}

void LedDeviceVirtual::requestFirmwareVersion()
{
	emit firmwareVersion(QStringLiteral("1.0 (virtual device)"));
	ack(true);
}


void LedDeviceVirtual::open()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	emit openDeviceSuccess(true);
}

void LedDeviceVirtual::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize)
		return;

	m_colorsBuffer.clear();

	if (buffSize > MaximumNumberOfLeds::Virtual)
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Virtual" << buffSize << ">" << MaximumNumberOfLeds::Virtual;

		buffSize = MaximumNumberOfLeds::Virtual;
	}

	for (int i = 0; i < buffSize; i++)
	{
		m_colorsBuffer << StructRgb();
	}
}

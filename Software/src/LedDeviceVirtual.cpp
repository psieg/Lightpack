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
#include "enums.hpp"
#include "debug.h"

using namespace SettingsScope;

LedDeviceVirtual::LedDeviceVirtual(QObject * parent) : AbstractLedDevice(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_gamma = Settings::getDeviceGamma();
	m_brightness = Settings::getDeviceBrightness();
}

void LedDeviceVirtual::setColors(const QList<QRgb> & colors)
{
	if(colors.size()> 0)
	{
		m_colorsSaved = colors;

		QList<QRgb> callbackColors;

		resizeColorsBuffer(colors.count());

		applyColorModifications(colors, m_colorsBuffer);
		applyDithering(m_colorsBuffer, 8);

		for (int i = 0; i < m_colorsBuffer.count(); i++)
		{
			callbackColors.append(qRgb(m_colorsBuffer[i].r, m_colorsBuffer[i].g, m_colorsBuffer[i].b));
		}

		emit colorsUpdated(callbackColors);
	}
	emit commandCompleted(true);
}

void LedDeviceVirtual::switchOffLeds()
{
	int count = m_colorsSaved.count();
	m_colorsSaved.clear();

	for (int i = 0; i < count; i++) {
		m_colorsSaved << 0;
	}
	emit colorsUpdated(m_colorsSaved);
	emit commandCompleted(true);
}

void LedDeviceVirtual::setRefreshDelay(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceVirtual::setColorDepth(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceVirtual::setSmoothSlowdown(int /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceVirtual::setColorSequence(QString /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceVirtual::setGamma(double value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	m_gamma = value;
	setColors(m_colorsSaved);
}

void LedDeviceVirtual::setBrightness(int percent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << percent;

	m_brightness = percent;
	setColors(m_colorsSaved);
}

void LedDeviceVirtual::requestFirmwareVersion()
{
	emit firmwareVersion("1.0 (virtual device)");
	emit commandCompleted(true);
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


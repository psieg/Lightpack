/*
 * MoodLampManager.cpp
 *
 *	Created on: 11.12.2011
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *	Lightpack a USB content-driving ambient lighting system
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


#include "LiquidColorGenerator.hpp"
#include "Settings.hpp"
#include <QTime>

using namespace SettingsScope;

const QColor LiquidColorGenerator::AvailableColors[LiquidColorGenerator::ColorsMoodLampCount] =
{
	Qt::white, Qt::red, Qt::yellow, Qt::green, Qt::blue, Qt::magenta, Qt::cyan,
	Qt::darkRed, Qt::darkGreen, Qt::darkBlue, Qt::darkYellow,
	qRgb(255,128,0), qRgb(128,255,255), qRgb(128,0,255)
};

LiquidColorGenerator::LiquidColorGenerator(QObject *parent) : QObject(parent)
{

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	m_rnd.seed(QTime(0,0,0).secsTo(QTime::currentTime()));
#else
	qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
#endif

	m_isEnabled = false;
	m_timer.setTimerType(Qt::PreciseTimer);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateColor()));
}

void LiquidColorGenerator::start(const QColor& color)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_isEnabled = true;

	reset();
	m_red = color.red();
	m_green = color.green();
	m_blue = color.blue();
	updateColor();
}

void LiquidColorGenerator::stop()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_isEnabled = false;

	m_timer.stop();
}

QColor LiquidColorGenerator::current()
{
	return QColor(m_red, m_green, m_blue);
}

void LiquidColorGenerator::setSpeed(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	m_speed = value;
	m_delay = generateDelay();
}

void LiquidColorGenerator::reset()
{
	m_red = 0;
	m_green = 0;
	m_blue = 0;
	m_unselectedColors.clear();
}

void LiquidColorGenerator::updateColor()
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << m_speed;

	if (m_red == m_redNew && m_green == m_greenNew && m_blue == m_blueNew)
	{
		m_delay = generateDelay();
		QColor colorNew = generateColor();

		m_redNew = colorNew.red();
		m_greenNew = colorNew.green();
		m_blueNew = colorNew.blue();

		DEBUG_HIGH_LEVEL << Q_FUNC_INFO << colorNew;
	}

	if (m_redNew != m_red)	{ if (m_red	> m_redNew)	--m_red;	else ++m_red; }
	if (m_greenNew != m_green){ if (m_green > m_greenNew) --m_green; else ++m_green; }
	if (m_blueNew != m_blue) { if (m_blue	> m_blueNew)	--m_blue;	else ++m_blue; }

	emit updateColor(QColor(m_red, m_green, m_blue));

	if (m_isEnabled)
	{
		m_timer.start(m_delay);
	}
}

int LiquidColorGenerator::generateDelay()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	return 1000 / (m_speed + m_rnd.bounded(25) + 1);
#else
	return 1000 / (m_speed + (qrand() % 25) + 1);
#endif
}

QColor LiquidColorGenerator::generateColor()
{
	if (m_unselectedColors.empty())
	{
		for (int i = 0; i < ColorsMoodLampCount; i++)
			m_unselectedColors << AvailableColors[i];
	}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	int randIndex = m_rnd.bounded(std::max(1, m_unselectedColors.size()));
#else
	int randIndex = qrand() % std::max(1, m_unselectedColors.size());
#endif

	return m_unselectedColors.takeAt(randIndex);
}

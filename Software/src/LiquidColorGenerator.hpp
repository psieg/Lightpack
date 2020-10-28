/*
 * MoodLampManager.hpp
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

#pragma once

#include <QObject>
#include <QColor>
#include <QTimer>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
#endif

class LiquidColorGenerator : public QObject
{
	Q_OBJECT
public:
	explicit LiquidColorGenerator(QObject *parent = 0);

signals:
	void updateColor(QColor color);

public:
	void start(const QColor& color = Qt::black);
	void stop();
	QColor current();
	void reset();

public slots:
	void setSpeed(int value);

private:
	int generateDelay();
	QColor generateColor();

private slots:
	void updateColor();

private:
	bool m_isEnabled;

	QTimer m_timer;
	int m_delay;
	int m_speed;

	int m_red = 0;
	int m_green = 0;
	int m_blue = 0;

	int m_redNew = 0;
	int m_greenNew = 0;
	int m_blueNew = 0;

	static const int ColorsMoodLampCount = 14;
	static const QColor AvailableColors[ColorsMoodLampCount];
	QList<QColor> m_unselectedColors;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	QRandomGenerator m_rnd;
#endif
};

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
#include <QRgba64>
#include <QTimer>
#include <QElapsedTimer>
#include "LiquidColorGenerator.hpp"
#include "MoodLamp.hpp"

class MoodLampManager : public QObject
{
	Q_OBJECT
public:
	explicit MoodLampManager(QObject *parent = 0);
	~MoodLampManager();

signals:
	void updateLedsColors(const QList<QRgba64> & colors);
	void lampList(const QList<MoodLampLampInfo> &, int);
	void moodlampFrametime(const double frameMs);

public:
	void start(bool isMoodLampEnabled);

	// Common options
	void reset();

public slots:
	void initFromSettings();
	void setLiquidMode(bool isEnabled);
	void setLiquidModeSpeed(int value);
	void settingsProfileChanged(const QString &profileName);
	void setNumberOfLeds(int value);
	void setCurrentColor(QColor color);
	void setCurrentLamp(const int id);
	void requestLampList();
	void setSendDataOnlyIfColorsChanged(bool state);

private slots:
	void updateColors(const bool forceUpdate = false);

private:
	void initColors(int numberOfLeds);

private:
	MoodLampBase* m_lamp{ nullptr };

	LiquidColorGenerator m_generator;
	QList<QRgba64> m_colors;

	bool	m_isMoodLampEnabled;
	QColor  m_currentColor;
	bool	m_isLiquidMode;
	bool	m_isSendDataOnlyIfColorsChanged;

	QTimer m_timer;
	QElapsedTimer m_elapsedTimer;
	size_t m_frames{ 1 };
};

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
#include <QElapsedTimer>
#include <QJSEngine>
#include <QDir>
#include "LiquidColorGenerator.hpp"

struct MoodLampLampInfo {
	MoodLampLampInfo() {}
	MoodLampLampInfo(const QString& name, const QString& moduleName, const QString& modulePath) :
		name(name), moduleName(moduleName), modulePath(modulePath)
	{}
	QString name;
	QString moduleName;
	QString modulePath;
};
Q_DECLARE_METATYPE(MoodLampLampInfo);

class MoodLampManager : public QObject
{
	Q_OBJECT
public:
	explicit MoodLampManager(const QString& appDir, QObject *parent = 0);
	~MoodLampManager();

signals:
	void updateLedsColors(const QList<QRgb> & colors);
	void lampList(const QList<MoodLampLampInfo> &);
	void moodlampFrametime(const double frameMs);

public:
	void start(bool isMoodLampEnabled);
	QString scriptDir() const;
	// Common options
	void reset();

public slots:
	void initFromSettings();
	void setLiquidMode(bool isEnabled);
	void setLiquidModeSpeed(int value);
	void settingsProfileChanged(const QString &profileName);
	void setNumberOfLeds(int value);
	void setCurrentColor(QColor color);
	void setCurrentLamp(const QString& moduleName);
	void requestLampList(const bool reloadScripts);
	void setSendDataOnlyIfColorsChanged(bool state);

private slots:
	void updateColors(const bool forceUpdate = false);

private:
	void initColors(int numberOfLeds);
	QDir installScripts(const QString& appDir);
	QJSEngine* loadScripts();

private:
	LiquidColorGenerator m_generator;
	QList<QRgb> m_colors;

	bool	m_isMoodLampEnabled;
	QColor  m_currentColor;
	bool	m_isLiquidMode;
	bool	m_isSendDataOnlyIfColorsChanged;

	QTimer m_timer;
	QElapsedTimer m_elapsedTimer;
	size_t m_frames{ 1 };

	QJSEngine* m_jsEngine;
	QJSValue m_jsLamp;
	QDir m_scriptDir;
	QString m_lampModuleName;

	QMap<QString, MoodLampLampInfo> m_lamps;
};

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


#include "MoodLampManager.hpp"
#include "PrismatikMath.hpp"
#include "Settings.hpp"
#include "QColorMetaWrapper.hpp"
#include <QTime>
#include <QRegularExpression>

using namespace SettingsScope;

MoodLampManager::MoodLampManager(const QString& appDir, QObject *parent) : QObject(parent)
{
	m_isMoodLampEnabled = false;

	m_timer.setTimerType(Qt::PreciseTimer);

	m_scriptDir = installScripts(appDir);
	m_jsEngine = loadScripts();

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateColors()));
	initFromSettings();
}

MoodLampManager::~MoodLampManager()
{
	m_jsEngine->collectGarbage();
	delete m_jsEngine;
}

void MoodLampManager::start(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << isEnabled;

	m_isMoodLampEnabled = isEnabled;

	if (m_isMoodLampEnabled)
	{
		// This is usable if start is called after API unlock, and we should force set current color
		updateColors(true);
	}

	if (m_isMoodLampEnabled && m_isLiquidMode)
		m_generator.start(m_currentColor);
	else
		m_generator.stop();

	if (m_isMoodLampEnabled && !m_jsLamp.isUndefined())
		m_timer.start(m_jsLamp.property("interval").toUInt());
	else
		m_timer.stop();
}

void MoodLampManager::setCurrentColor(QColor color)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << color;

	m_currentColor = color;

	if (m_isMoodLampEnabled)
		updateColors();
}

void MoodLampManager::setLiquidMode(bool state)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;
	m_isLiquidMode = state;
	emit moodlampFrametime(1000); // reset FPS to 1
	if (m_isLiquidMode && m_isMoodLampEnabled)
		m_generator.start(m_currentColor);
	else {
		m_generator.stop();
		if (m_isMoodLampEnabled)
			updateColors();
	}
}

void MoodLampManager::setLiquidModeSpeed(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;
	m_generator.setSpeed(value);
}

void MoodLampManager::setSendDataOnlyIfColorsChanged(bool state)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << state;
	m_isSendDataOnlyIfColorsChanged = state;
	emit moodlampFrametime(1000); // reset FPS to 1
}

void MoodLampManager::setNumberOfLeds(int numberOfLeds)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << numberOfLeds;

	initColors(numberOfLeds);
}

void MoodLampManager::reset()
{
	m_generator.reset();
}

void MoodLampManager::settingsProfileChanged(const QString &profileName)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	Q_UNUSED(profileName)
	initFromSettings();
}

void MoodLampManager::initFromSettings()
{
	m_generator.setSpeed(Settings::getMoodLampSpeed());
	m_currentColor = Settings::getMoodLampColor();
	setLiquidMode(Settings::isMoodLampLiquidMode());
	m_isSendDataOnlyIfColorsChanged = Settings::isSendDataOnlyIfColorsChanges();

	initColors(Settings::getNumberOfLeds(Settings::getConnectedDevice()));
	setCurrentLamp(Settings::getMoodLampLamp());
}

void MoodLampManager::setCurrentLamp(const QString& moduleName)
{
	m_timer.stop();

	if (!m_jsLamp.isUndefined()) {
		m_jsLamp = QJSValue();
		m_jsEngine->collectGarbage();
	}

	if (!m_lamps.contains(moduleName)) {
		qWarning() << Q_FUNC_INFO << moduleName << "unknown lamp";
		return;
	}

	const MoodLampLampInfo& lampInfo = m_lamps[moduleName];

	const QJSValue& newLamp = m_jsEngine->importModule(lampInfo.modulePath);
	if (newLamp.isError()) {
		qWarning() << Q_FUNC_INFO << QString("JS Error in %1:%2 %3")
			.arg(newLamp.property("fileName").toString())
			.arg(newLamp.property("lineNumber").toInt())
			.arg(newLamp.toString());
		return;
	}
	m_lampModuleName = moduleName;
	m_jsLamp = newLamp;
	emit moodlampFrametime(1000); // reset FPS to 1
	if (m_isMoodLampEnabled && !m_jsLamp.isUndefined()) {
		updateColors(true);
		m_timer.start(m_jsLamp.property("interval").toUInt());
	}
}

void MoodLampManager::updateColors(const bool forceUpdate)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << m_isLiquidMode;

	QColor baseColor;

	if (m_isLiquidMode)
		baseColor = m_generator.current();
	else
		baseColor = m_currentColor;

	DEBUG_MID_LEVEL << Q_FUNC_INFO << baseColor.rgb();

	bool changed = false;
	if (!m_jsLamp.isUndefined()) {
		QJSValueList args;
		args << baseColor.rgb();

		QVariantList outColors;
		outColors.reserve(m_colors.size());
		for (const QRgb color : m_colors)
			outColors << color;
		args << m_jsEngine->toScriptValue(outColors);

		// Qt 5.15+ can auto convert m_colors
		//args << m_jsEngine.toScriptValue(m_colors);

		const QJSValue& result = m_jsLamp.property("tick").call(args);
		if (result.isError()) {
			qWarning() << Q_FUNC_INFO << QString("JS Error in %1:%2 %3")
				.arg(result.property("fileName").toString())
				.arg(result.property("lineNumber").toInt())
				.arg(result.toString());
		}
		else if (result.isArray()) {
			const QVariantList& colors = result.toVariant().toList();
			for (int i = 0; i < colors.size(); ++i) {
				const QRgb newColor = Settings::isLedEnabled(i) ? colors[i].toInt() : 0;
				changed = changed || (m_colors[i] != newColor);
				m_colors[i] = newColor;
			}
		}
		else
			qWarning() << Q_FUNC_INFO << m_jsLamp.property("name").toString() << "tick() does not return [rgb1, rgb2, ...]";
	}
	else { // fallback to static
		for (QRgb& color : m_colors) {
			changed = changed || (color != baseColor.rgb());
			color = baseColor.rgb();
		}
	}

	if (changed || !m_isSendDataOnlyIfColorsChanged || forceUpdate) {
		emit updateLedsColors(m_colors);
		if (forceUpdate) {
			emit moodlampFrametime(1000);
			m_elapsedTimer.restart();
			m_frames = 0;
		} else if (m_elapsedTimer.hasExpired(1000)) { // 1s
			emit moodlampFrametime(m_elapsedTimer.restart() / m_frames);
			m_frames = 0;
		}
		m_frames++;
	}
}

void MoodLampManager::initColors(int numberOfLeds)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << numberOfLeds;

	m_colors.clear();

	for (int i = 0; i < numberOfLeds; i++)
		m_colors << 0;
}

void MoodLampManager::requestLampList(const bool reloadScripts = false)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << reloadScripts;

	if (reloadScripts) {
		m_lamps.clear();
		m_scriptDir.refresh();
		QJSEngine* oldEngine = m_jsEngine;
		m_jsEngine = loadScripts();
		setCurrentLamp(m_lampModuleName);
		delete oldEngine;
	}
	emit lampList(m_lamps.values());
}

QDir MoodLampManager::installScripts(const QString& appDir)
{
	const QString moodlampsDirName("moodlamps");
	QDir installDir(appDir);
	if (!installDir.exists(moodlampsDirName))
		installDir.mkdir(moodlampsDirName);
	installDir.cd(moodlampsDirName);

	const QDir resLampDir(":/" + moodlampsDirName, "*.mjs", QDir::IgnoreCase | QDir::Name, QDir::Files);
	const QStringList& reslampScriptList = resLampDir.entryList();
	for (const QString& lampScript : reslampScriptList) {
		if (QFile::exists(installDir.absoluteFilePath(lampScript)) && !QFile::remove(installDir.absoluteFilePath(lampScript))) {
			qWarning() << Q_FUNC_INFO << "Could not remove" << lampScript;
			continue;
		}
		if (!QFile::copy(":/" + moodlampsDirName + "/" + lampScript, installDir.absoluteFilePath(lampScript)))
			qWarning() << Q_FUNC_INFO << "Cound not install" << lampScript;
	}
	return installDir;
}

QJSEngine* MoodLampManager::loadScripts()
{
	QJSEngine* jsEngine = new QJSEngine(this);
	jsEngine->globalObject().setProperty("QColor", jsEngine->newQMetaObject<QColorMetaWrapper>());
	jsEngine->globalObject().setProperty("QT_VERSION", QT_VERSION);

	const QRegularExpression cleanNameFilter("^[a-z0-9_]+\\.mjs$");
	const QStringList& lampScriptList = m_scriptDir.entryList().filter(cleanNameFilter);
	bool enableConsole = false;
	for (const QString& lampScript : lampScriptList) {
		const QJSValue& jsModule = jsEngine->importModule(m_scriptDir.filePath(lampScript));
		if (jsModule.isError()) {
			qWarning() << Q_FUNC_INFO << lampScript << QString("JS Error in %1:%2 %3")
				.arg(jsModule.property("fileName").toString())
				.arg(jsModule.property("lineNumber").toInt())
				.arg(jsModule.toString());
		}
		else if (!jsModule.hasOwnProperty("name") || !jsModule.property("name").isString())
			qWarning() << Q_FUNC_INFO << lampScript << "does not have \"export const name = string\"";
		else if (!jsModule.hasOwnProperty("tick") || !jsModule.property("tick").isCallable())
			qWarning() << Q_FUNC_INFO << lampScript << "does not have \"export function tick(baseColor, colors){...}\"";
		else if (!jsModule.hasOwnProperty("interval") || !jsModule.property("interval").isNumber() || jsModule.property("interval").toUInt() < 1)
			qWarning() << Q_FUNC_INFO << lampScript << "does not have \"export const interval = number\" with a valid (> 0) value" ;
		else {
			m_lamps.insert(lampScript,
				MoodLampLampInfo(
					jsModule.property("name").toString(),
					lampScript,
					m_scriptDir.filePath(lampScript)
				)
			);
			enableConsole |= jsModule.hasOwnProperty("enableConsole") && jsModule.property("enableConsole").toBool();
		}
	}
	if (m_lamps.isEmpty())
		qWarning() << Q_FUNC_INFO << "No moodlamps loaded from " << m_scriptDir.absolutePath() << "; file names have to match" << cleanNameFilter.pattern();
	if (enableConsole)
		jsEngine->installExtensions(QJSEngine::ConsoleExtension);
	return jsEngine;
}

QString MoodLampManager::scriptDir() const
{
	return m_scriptDir.absolutePath();
}

/*
 * MoodLamp.cpp
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

#include <QTime>
#include <cmath>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
#endif
#include "MoodLamp.hpp"
#include "Settings.hpp"

// Needed to get around the fact that the lamps are macros
class QRandomGeneratorShim {

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	QRandomGenerator m_rnd;
public:
	void seed(quint32 seed) {
		m_rnd.seed(seed);
	}

	int bounded(int highest) {
		return m_rnd.bounded(highest);
	}

	int bounded(int lowest, int highest) {
		return m_rnd.bounded(lowest, highest);
	}
#else
public:
	void seed(quint32 seed) {
		qsrand(seed);
	}

	int bounded(int highest) {
		return qrand() % highest;
	}

	int bounded(int lowest, int highest) {
		return lowest + (qrand() % (highest - lowest + 1));
	}
#endif

};
 /*
	 _OBJ_NAME_	: class name prefix
	 _LABEL_	: name string to be displayed
	 _BODY_		: class declaration body
 */
#define DECLARE_LAMP(_OBJ_NAME_,_LABEL_,_BODY_) \
class _OBJ_NAME_ ## MoodLamp : public MoodLampBase \
{\
public:\
_OBJ_NAME_ ## MoodLamp() : MoodLampBase() {};\
~_OBJ_NAME_ ## MoodLamp() = default;\
static const char* const name() { return _LABEL_; };\
static MoodLampBase* create() { return new _OBJ_NAME_ ## MoodLamp(); };\
\
_BODY_\
};\
struct _OBJ_NAME_ ## Register {\
_OBJ_NAME_ ## Register(){\
	g_moodLampMap.insert(lampID, MoodLampLampInfo(_OBJ_NAME_ ## MoodLamp::name(), _OBJ_NAME_ ## MoodLamp::create, lampID));\
	++lampID;\
}\
};\
_OBJ_NAME_ ## Register _OBJ_NAME_ ## Reg;

using namespace SettingsScope;

namespace {
	static QMap<int, MoodLampLampInfo> g_moodLampMap;
	static int lampID = 0;
}

MoodLampBase* MoodLampBase::createWithID(const int id) {
	QMap<int, MoodLampLampInfo>::const_iterator i = g_moodLampMap.find(id);
	if (i != g_moodLampMap.end())
		return i.value().factory();
	qWarning() << Q_FUNC_INFO << "failed to find mood lamp ID: " << id;
	return nullptr;
}

void MoodLampBase::populateNameList(QList<MoodLampLampInfo>& list, int& recommended)
{
	list = g_moodLampMap.values();

	if (list.size() > 0)
		recommended = list[0].id;
}

DECLARE_LAMP(Static, "Static (default)",
public:
	int interval() const { return 50; };

	bool shine(const QColor& newColor, QList<QRgba64>& colors)
	{
		bool changed = false;
		for (int i = 0; i < colors.size(); i++)
		{
			QRgba64 rgb = Settings::isLedEnabled(i) ? newColor.rgba64() : qRgba64(0);
			changed = changed || (colors[i] != rgb);
			colors[i] = rgb;
		}
		return changed;
	};
);

DECLARE_LAMP(Fire, "Fire",
public:
	void init() {
		m_rnd.seed(QTime(0, 0, 0).secsTo(QTime::currentTime()));
	};

	bool shine(const QColor& newColor, QList<QRgba64>& colors) {
		if (colors.size() < 2)
			return false;

		if (colors.size() > m_lightness.size()) {
			const size_t oldSize = m_lightness.size();
			m_lightness.reserve(colors.size());
			for (size_t i = oldSize; i < colors.size(); ++i)
				m_lightness << 0;
		}

		// heavily inspired by FastLED Fire2012 demo
		// https://github.com/FastLED/FastLED/blob/master/examples/Fire2012/Fire2012.ino
		const int centerMax = colors.size() / 4;
		const int middleLed = std::floor(colors.size() / 2);
		const int sparkCount = colors.size() / 12;

		m_center += m_rnd.bounded(2) ? -1 : 1;
		m_center = std::max(-centerMax, std::min(centerMax, m_center));

		for (int i = 0; i < middleLed + m_center; ++i) {
			const int minLightnessReduction = Cooling * std::pow((double)i / (middleLed + m_center), 3);
			const int maxLightnessReduction = minLightnessReduction * 2;
			const int lightnessReduction = minLightnessReduction + m_rnd.bounded(std::max(1, maxLightnessReduction - minLightnessReduction)) + Cooling / 3;
			m_lightness[i] = std::max(0, m_lightness[i] - lightnessReduction);
		}

		for (int i = colors.size() - 1; i >= middleLed + m_center; --i) {
			const int minLightnessReduction = Cooling * std::pow((double)(colors.size() - 1 - i) / (middleLed - m_center), 3);
			const int maxLightnessReduction = minLightnessReduction * 2;
			const int lightnessReduction = minLightnessReduction + m_rnd.bounded(std::max(1, maxLightnessReduction - minLightnessReduction)) + Cooling / 3;
			m_lightness[i] = std::max(0, m_lightness[i] - lightnessReduction);
		}


		for (int k = middleLed + m_center; k > 1; --k)
			m_lightness[k] = (m_lightness[k - 1] + m_lightness[k - 2] * 2) / 3;

		for (int k = middleLed + m_center; k < colors.size() - 2; ++k)
			m_lightness[k] = (m_lightness[k + 1] + m_lightness[k + 2] * 2) / 3;


		if (m_rnd.bounded(2) == 0) {
			int y = m_rnd.bounded(std::max(1, sparkCount));
			m_lightness[y] = std::max(SparkMax, (int)m_lightness[y] + (SparkMin + m_rnd.bounded(SparkMax - SparkMin)));
		}
		if (m_rnd.bounded(2) == 0) {
			int z = colors.size() - 1 - m_rnd.bounded(std::max(1, sparkCount));
			m_lightness[z] = std::max(SparkMax, (int)m_lightness[z] + (SparkMin + m_rnd.bounded(SparkMax - SparkMin)));
		}

		for (int i = 0; i < colors.size(); i++)
		{
			QColor color(newColor);
			color.setHsl(color.hue(), color.saturation(), m_lightness[i]);
			colors[i] = Settings::isLedEnabled(i) ? color.rgba64() : qRgba64(0);
		}
		return true;
	};
private:
	QRandomGeneratorShim m_rnd;
	QList<quint8> m_lightness;
	int m_center{ 0 };

	const size_t Cooling = 8;
	const int SparkMax = 160;
	const int SparkMin = 100;
);

DECLARE_LAMP(RGBLife, "RGB is Life",
public:
	bool shine(const QColor& newColor, QList<QRgba64>& colors)
	{
		bool changed = false;
		const int degrees = 360 / colors.size();
		const int step = Speed * m_frames++;
		for (int i = 0; i < colors.size(); i++)
		{
			QColor color(newColor);
			color.setHsl(color.hue() + degrees * i + step, color.saturation(), color.lightness());
			QRgba64 rgb = Settings::isLedEnabled(i) ? color.rgba64() : qRgba64(0);
			changed = changed || (colors[i] != rgb);
			colors[i] = rgb;
		}
		if (step > 360)
			m_frames = 0;
		return changed;
	};
private:
	const double Speed = 1.5;
);

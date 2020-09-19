/*
 * MoodLamp.hpp
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
#include <QRgba64>

class MoodLampBase;

typedef MoodLampBase* (*LampFactory)();

struct MoodLampLampInfo {
	MoodLampLampInfo() { this->name = ""; this->id = -1; this->factory = nullptr; }
	MoodLampLampInfo(QString name, LampFactory factory, int id) { this->name = name; this->id = id; this->factory = factory; }
	QString name;
	LampFactory factory;
	int id;
};
Q_DECLARE_METATYPE(MoodLampLampInfo);

class MoodLampBase
{
public:
	MoodLampBase() { init(); };
	virtual ~MoodLampBase() = default;

	static const char* const name() { return "NO_NAME"; };
	static MoodLampBase* create() { Q_ASSERT_X(false, "MoodLampBase::create()", "not implemented"); return nullptr; };
	static MoodLampBase* createWithID(const int id);
	static void populateNameList(QList<MoodLampLampInfo>& list, int& recommended);

	virtual void init() {};
	virtual int interval() const { return DefaultInterval; };
	virtual bool shine(const QColor& newColor, QList<QRgba64>& colors) = 0;
protected:
	size_t m_frames{ 0 };
private:
	const int DefaultInterval = 33;
};

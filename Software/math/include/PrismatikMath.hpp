/*
 * LightpackMath.hpp
 *
 *	Created on: 29.09.2011
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

#include <QList>
#include <QRgb>
#include <cmath>
#include "colorspace_types.h"
#include "../../common/defs.h"

namespace PrismatikMath
{
	void gammaCorrection(double gamma, StructRgb &);
	void brightnessCorrection(unsigned int brightness, StructRgb &);
	void maxCorrection(unsigned int max, StructRgb &);
	int getValueHSV(const QRgb rgb);
	int getChromaHSV(const QRgb rgb);
	int max(const QRgb);
	int min(const QRgb);
	QRgb withValueHSV(const QRgb, int);
	QRgb withChromaHSV(const QRgb, int);
	void applyColorTemperature(QList<QRgb>&, const quint16, double gamma);
	StructRgb whitePoint(const quint16 colorTemperature);
	StructRgb avgColor(const QList<StructRgb> &);
	StructXyz toXyz(const StructRgb &);
	StructXyz toXyz(const StructLab &);
	StructLab toLab(const StructRgb &);
	StructLab toLab(const StructXyz &);
	StructRgb toRgb(const StructXyz &);
	StructRgb toRgb(const StructLab &);

	// Convert ASCII char '5' to 5
	inline char getDigit(const char d)
	{
		if (isdigit(d))
			return (d - '0');
		else
			return -1;
	}

	inline int rand(int val)
	{
		return qrand() % val;
	}

	inline double round(double number)
	{
#if defined HAVE_PLATFORM_ROUND_FUNC
		return ::round(number);
#else
		return number < 0.0 ? std::ceil(number - 0.5) : std::floor(number + 0.5);
#endif
	}
}


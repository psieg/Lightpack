/*
 * tst_GrabCalculationTestTest.cpp
 *
 *	Created on: 17.07.2012
 *		Project: Lightpack
 *
 *	Copyright (c) 2012 Timur Sattarov
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


#include <QString>
#include <QtTest>
#include <QRgb>
#include <QRect>
#include "enums.hpp"
#include "calculations.hpp"

class GrabCalculationTest : public QObject
{
	Q_OBJECT
	
public:
	GrabCalculationTest(){}
	
private Q_SLOTS:
	void testCase1();
	void testEmptyRectDoesNotCrash();
	void testOutOfBoundsAndNegativeCoordsRejected();
};


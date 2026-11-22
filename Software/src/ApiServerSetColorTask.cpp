/*
 * ApiServerSetColorTask.cpp
 *
 *	Created on: 07.09.2011
 *		Author: Mike Shatohin
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
#include <QThread>
#include "debug.h"
#include "ApiServerSetColorTask.hpp"
#include "Settings.hpp"
#include <cmath>
#include "PrismatikMath.hpp"

ApiServerSetColorTask::ApiServerSetColorTask(QObject *parent) :
	QObject(parent)
{
	m_numberOfLeds = MaximumNumberOfLeds::Default;

	reinitColorBuffers();
}

void ApiServerSetColorTask::startParseSetColorTask(QByteArray buffer)
{
	API_DEBUG_OUT << QString(buffer) << "task thread:" << thread()->currentThreadId();
	bool isReadFail = false;

	// buffer can contains only something like this:
	// 1-34,9,125
	// 2-0,255,0;3-0,255,0;6-0,255,0;

	while (buffer.isEmpty() == false)
	{
		if (isReadFail == true)
		{
			qWarning() << "got it!";
		}

		// Check buffer length, minimum 7 - '2-0,0,0'
		if (buffer.length() < 7)
		{
			API_DEBUG_OUT << "error: buffer.length() < 7";
			isReadFail = true;
			goto end;
		}

		// Read led number
		int ledNumber = PrismatikMath::getDigit(buffer[0]); // first char of ledNumber
		int ledNumber2 = PrismatikMath::getDigit(buffer[1]); // second char of ledNumber
		int ledNumber3 = PrismatikMath::getDigit(buffer[2]); // third char of ledNumber
		if (ledNumber > 0)
		{
			if (buffer[1] == '-') // single digit number
			{
				buffer.remove(0, 2); // remove "2-"
			}
			else if (ledNumber2 >= 0 && buffer[2] == '-') // 2 digit number
			{
				ledNumber = ledNumber * 10 + ledNumber2; // 10,11,12..99
				buffer.remove(0, 3); // remove "10-"
			}
			else if (ledNumber3 >= 0 && buffer[3] == '-') // 3 digit number
			{
				ledNumber = (ledNumber * 100) + (ledNumber2 * 10) + ledNumber3;
				buffer.remove(0, 4); // remove "100-" from the buffer
			} else {
				API_DEBUG_OUT << "lednumber fail:" << QString(buffer);
				isReadFail = true;
				goto end;
			}
		} else {
			API_DEBUG_OUT << "isdigit fail:" << QString(buffer);
			isReadFail = true;
			goto end;
		}

		API_DEBUG_OUT << "lednumber-" << ledNumber << "buff-" << QString(buffer);

		if (ledNumber <= 0 || ledNumber > m_numberOfLeds || ledNumber > m_colors.size())
		{
			API_DEBUG_OUT << "ledNumber is out of bounds:" << ledNumber;
			isReadFail = true;
			goto end;
		}

		// Convert for using in zero-based arrays
		ledNumber = ledNumber - 1;

		// Read led red, green and blue colors to buffer buffRgb[]
		int indexBuffer = 0;
		int indexRgb = 0;
		memset(buffRgb, 0, sizeof(buffRgb));

		bool isDigitExpected = true;
		for (indexBuffer = 0; indexBuffer < buffer.length(); indexBuffer++)
		{
			int d = PrismatikMath::getDigit(buffer[indexBuffer]);
			if (d < 0)
			{
				if (buffer[indexBuffer] == ';')
				{
					break;
				}
				else if (buffer[indexBuffer] != ',')
				{
					API_DEBUG_OUT << "expected comma, buffer:" << QString(buffer) << indexBuffer << buffer[indexBuffer];
					isReadFail = true;
					goto end;
				}
				else if (isDigitExpected || indexRgb + 1 >= bSize)
				{
					API_DEBUG_OUT << "unexpected or excess comma, buffer:" << QString(buffer) << indexBuffer << buffer[indexBuffer];
					isReadFail = true;
					goto end;
				}
				isDigitExpected = true;
				indexRgb++;
			} else {
				buffRgb[indexRgb] *= 10;
				buffRgb[indexRgb] += d;

				isDigitExpected = false;

				if (buffRgb[indexRgb] > 255)
				{
					API_DEBUG_OUT << "rgb value > 255";
					isReadFail = true;
					goto end;
				}
			}
		}

		if (indexRgb != bBlue || isDigitExpected)
		{
			API_DEBUG_OUT << "incomplete RGB triplet, indexRgb:" << indexRgb;
			isReadFail = true;
			goto end;
		}

		// Remove read colors
		buffer.remove(0, indexBuffer);

		// Save colors
		m_colors[ledNumber] = qRgb(buffRgb[bRed], buffRgb[bGreen], buffRgb[bBlue]);

		API_DEBUG_OUT << "result color:" << buffRgb[bRed] << buffRgb[bGreen] << buffRgb[bBlue]
						<< "buffer:" << QString(buffer);

		if (!buffer.isEmpty() && buffer[0] == ';')
		{
			// Remove semicolon
			buffer.remove(0, 1);

			API_DEBUG_OUT << "buffer.isEmpty() == " << buffer.isEmpty();
		}
	}

end:
	if (isReadFail)
	{
		API_DEBUG_OUT << "errors while reading buffer";
		emit taskParseSetColorIsSuccess(false);
	} else {
		API_DEBUG_OUT << "read setcolor buffer - ok";
		emit taskParseSetColorDone(m_colors);
		emit taskParseSetColorIsSuccess(true);
	}
}

void ApiServerSetColorTask::reinitColorBuffers()
{
	m_colors.clear();

	for (int i = 0; i < m_numberOfLeds; i++)
		m_colors << 0;

	buffRgb[bRed] = 0;
	buffRgb[bGreen] = 0;
	buffRgb[bBlue] = 0;
}

void ApiServerSetColorTask::setApiDeviceNumberOfLeds(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	if (value < 0)
		value = 0;
	else if (value > MaximumNumberOfLeds::AbsoluteMaximum)
		value = MaximumNumberOfLeds::AbsoluteMaximum;

	m_numberOfLeds = value;

	reinitColorBuffers();
}

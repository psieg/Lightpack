/*
 * LedDeviceDnrgb.hpp
 *
 *	Created on: 16.02.2020
 *		Author: Tom Archer
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

#pragma once

#include "AbstractLedDeviceUdp.hpp"

class LedDeviceDnrgb : public AbstractLedDeviceUdp
{
	Q_OBJECT
public:
    LedDeviceDnrgb(const QString& address, const QString& port, const int timeout, QObject * parent = 0);

public slots:
    const QString name() const;
	void setColors(const QList<QRgba64> & colors);
	void switchOffLeds();
    void requestFirmwareVersion();
    int maxLedsCount();

protected:
    void resizeColorsBuffer(int buffSize);
    void reinitBufferHeader();

private:
    const int LEDS_PER_PACKET = 489;
};

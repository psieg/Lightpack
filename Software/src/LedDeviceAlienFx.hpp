/*
 * LedDeviceAlienFx.hpp
 *
 *  Created on: 17.04.2011
 *      Author: Timur Sattarov && Mike Shatohin
 *     Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <qglobal.h>

#ifdef Q_OS_WIN

#include "AbstractLedDevice.hpp"

class LedDeviceAlienFx : public AbstractLedDevice
{
    Q_OBJECT
public:
    LedDeviceAlienFx(QObject *parent = 0);
    virtual ~LedDeviceAlienFx();

public slots:
    const QString name() const { return "lightfx"; }
    void open();
    void close(){};
    void switchOffLeds();
    void setColorSequence(QString /*value*/);
    void requestFirmwareVersion();
	int maxLedsCount() { return 1; }
	int defaultLedsCount() { return 1; }

protected:
	virtual void setColorsUnsmoothed(const QList<QRgb> & colors);

private:
    HINSTANCE m_hLfxLibrary;
    bool m_isInitialized;
};

#endif /* Q_OS_WIN */

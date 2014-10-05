/*
 * ApiServerSetColorTask.hpp
 *
 *  Created on: 07.09.2011
 *      Author: Mike Shatohin
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

#include <QObject>
#include <QRgb>

class ApiServerSetColorTask : public QObject
{
    Q_OBJECT
public:
    explicit ApiServerSetColorTask(QObject *parent = 0);

signals:
    void taskParseSetColorDone(const QList<QRgb> & colors);
    void taskParseSetColorIsSuccess(bool isSuccess);

public slots:
    void startParseSetColorTask(QByteArray buffer);
    void reinitColorBuffers();
    void setApiDeviceNumberOfLeds(int value);

private:
    QList<QRgb> m_colors;
    int m_numberOfLeds;

    enum BuffRgbIndexes{
        bRed, bGreen, bBlue, bSize
    };
    int buffRgb[bSize]; // buffer for store temp red, green and blue values

};

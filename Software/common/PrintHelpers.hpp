/*
 *  Printhelpers.hpp
 *
 *  Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2014 Dreamer Dead, dreamer.dead@gmail.com
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

#define DEBUG_OUT_RGB( RGB_VALUE ) \
    qDebug() << #RGB_VALUE << "=" << qRed(RGB_VALUE) << qGreen(RGB_VALUE) << qBlue(RGB_VALUE)

namespace Debug
{
inline const QString toString(const QRgb color) {
    return QString("r=%1, g=%2, b=%3, a=%4").arg(
        QString::number(qRed(color)),
        QString::number(qGreen(color)),
        QString::number(qBlue(color)),
        QString::number(qAlpha(color)));
}

inline const QString toString(const QRect& rect) {
    return QString("x=%1, y=%2, width=%3, height=%4").arg(
        QString::number(rect.x()),
        QString::number(rect.y()),
        QString::number(rect.width()),
        QString::number(rect.height()));
}
}

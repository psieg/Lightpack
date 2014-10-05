/*
 * GrabbedArea.hpp
 *
 *     Project: Prismatik
 *
 *  Copyright (c) 2014 dreamer.dead@gmail.com
 *
 *  Lightpack is an open-source, USB content-driving ambient lighting
 *  hardware.
 *
 *  Prismatik is a free, open-source software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Prismatik and Lightpack files is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GRABBEDAREA_HPP
#define GRABBEDAREA_HPP

#include <QRect>

class GrabbedArea {
public:
    virtual ~GrabbedArea() {}
    virtual bool isEnabled() const { return false; }
    virtual QRect geometry() const { return QRect(); }
};

#endif // GRABBEDAREA_HPP

/*
 * MacOSGrabber.hpp
 *
 *  Created on: 01.10.11
 *     Project: Lightpack
 *
 *  Copyright (c) 2011 Timur Sattarov, Nikolay Kudashov, Mike Shatohin
 *
 *  Lightpack a USB content-driving ambient lighting system
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

#include "common/defs.h"

#ifdef MAC_OS_CG_GRAB_SUPPORT

#include <CoreGraphics/CGColorSpace.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGImage.h>

#include "GrabberBase.hpp"

class MacOSGrabber : public GrabberBase
{
public:
    MacOSGrabber(QObject *parent, GrabberContext *context);
    virtual ~MacOSGrabber();

    DECLARE_GRABBER_NAME("MacOSGrabber")

protected slots:
    virtual GrabResult grabScreens();
    virtual bool reallocate(const QList< ScreenInfo > &grabScreens);

    virtual QList< ScreenInfo > * screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabbedArea *> &grabWidgets);
private:
    void freeScreens();
    void toGrabbedScreen(CGImageRef, GrabbedScreen *);
};

#endif // MAC_OS_CG_GRAB_SUPPORT

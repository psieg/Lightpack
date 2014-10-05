/*
 * MacOSGrabber.cpp
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

#include"MacOSGrabber.hpp"

#ifdef MAC_OS_CG_GRAB_SUPPORT

#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#include <QGuiApplication>

#include "calculations.hpp"
#include "common/DebugOut.hpp"

const uint32_t kMaxDisplaysCount = 10;

MacOSGrabber::MacOSGrabber(QObject *parent, GrabberContext *context):
    GrabberBase(parent, context)
{
}

MacOSGrabber::~MacOSGrabber()
{
    freeScreens();
}

void MacOSGrabber::freeScreens()
{
    for (int i = 0; i < _screensWithWidgets.size(); ++i) {
        GrabbedScreen screen = _screensWithWidgets[i];
        if (screen.imgData != NULL)
            free(screen.imgData);

        if (screen.associatedData != NULL)
            free(screen.associatedData);
    }
    _screensWithWidgets.clear();
}

QList< ScreenInfo > * MacOSGrabber::screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets)
{
    CGDirectDisplayID displays[kMaxDisplaysCount];
    uint32_t displayCount;

    CGError err = CGGetActiveDisplayList(kMaxDisplaysCount, displays, &displayCount);

    result->clear();

    if (err == kCGErrorSuccess) {
        for (unsigned int i = 0; i < displayCount; ++i) {
            CGRect cgScreenRect = CGDisplayBounds(displays[i]);
            for (int k = 0; k < grabWidgets.size(); ++k) {
                QRect rect = grabWidgets[k]->frameGeometry();
                CGPoint widgetCenter = CGPointMake(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
                if (CGRectContainsPoint(cgScreenRect, widgetCenter)) {
                    ScreenInfo screenInfo;
                    int x1 = cgScreenRect.origin.x;
                    int y1 = cgScreenRect.origin.y;
                    int x2 = cgScreenRect.size.width  + x1 - 1;
                    int y2 = cgScreenRect.size.height + y1 - 1;
                    screenInfo.rect.setCoords( x1, y1, x2, y2);
                    screenInfo.handle = reinterpret_cast<void *>(displays[i]);

                    result->append(screenInfo);

                    break;
                }
            }
        }

    } else {
        qCritical() << "couldn't get active displays, error code " << QString::number(err, 16);
    }
    return result;

}

void MacOSGrabber::toGrabbedScreen(CGImageRef imageRef, GrabbedScreen *screen)
{
    size_t width = CGImageGetWidth(imageRef);
    size_t height = CGImageGetHeight(imageRef);
    /*
    size_t new_buf_size = height * width * bytesPerPixel;
    if (new_buf_size > _imageBufSize) {
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << "new width = " << width << " new height = " << height;
        if (_imageBuf)
            free(_imageBuf);
        _imageBuf = (unsigned char*) calloc(height * width * bytesPerPixel, sizeof(unsigned char));
        _imageBufSize = new_buf_size;
    }
*/

    CGDataProviderRef provider = CGImageGetDataProvider(imageRef);
    CFDataRef dataref = CGDataProviderCopyData(provider);
    memcpy(screen->imgData, CFDataGetBytePtr(dataref), width * height * 4);
    CFRelease(dataref);
}

bool MacOSGrabber::reallocate(const QList<ScreenInfo> &screens)
{
    const size_t kBytesPerPixel = 4;
    freeScreens();
    for (int i = 0; i < screens.size(); ++i) {


        //MacOSScreenData *d = new MacOSScreenData();

        int screenid = reinterpret_cast<intptr_t>(screens[i].handle);

        qreal pixelRatio = ((QGuiApplication*)QCoreApplication::instance())->devicePixelRatio();
        long width = CGDisplayPixelsWide(screenid) * pixelRatio;
        long height = CGDisplayPixelsHigh(screenid) * pixelRatio;

        DEBUG_HIGH_LEVEL << "dimensions " << width << "x" << height << pixelRatio << screens[i].handle;

        size_t imgSize = height * width * kBytesPerPixel;
        unsigned char *buf = reinterpret_cast<unsigned char*>(calloc(imgSize, sizeof(unsigned char)));
        if (buf == NULL) {
            qCritical() << "couldn't allocate image buffer";
            freeScreens();
            return false;
        }
        GrabbedScreen grabScreen;
        grabScreen.imgData = buf;
        grabScreen.imgFormat = BufferFormatArgb;
        grabScreen.screenInfo = screens[i];
        //grabScreen.associatedData = d;
        _screensWithWidgets.append(grabScreen);

    }
    return true;
}

GrabResult MacOSGrabber::grabScreens()
{
    for (int i = 0; i < _screensWithWidgets.size(); ++i) {

        CGDirectDisplayID display = reinterpret_cast<intptr_t>(_screensWithWidgets[i].screenInfo.handle);

        CGImageRef imageRef = CGDisplayCreateImage(display);

        if (imageRef != NULL)
        {

            toGrabbedScreen(imageRef, &_screensWithWidgets[i] );

            CGImageRelease(imageRef);

        } else {

            qCritical() << Q_FUNC_INFO << "CGDisplayCreateImage(..) returned NULL";
            return GrabResultError;
        }
    }
    return GrabResultOk;
}

#endif // MAC_OS_CG_GRAB_SUPPORT

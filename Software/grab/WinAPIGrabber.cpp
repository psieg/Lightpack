/*
 * WinAPIGrabber.cpp
 *
 *  Created on: 25.07.11
 *     Project: Lightpack
 *
 *  Copyright (c) 2011 Timur Sattarov, Mike Shatohin
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

#include "WinAPIGrabber.hpp"

#ifdef WINAPI_GRAB_SUPPORT

#include <cmath>

#include "common/DebugOut.hpp"
#include "calculations.hpp"
#include "../prismatic/enums.hpp"

struct WinAPIScreenData
{
    WinAPIScreenData()
        : hScreenDC(NULL)
        , hMemDC(NULL)
        , hBitmap(NULL)
    {}
    HDC hScreenDC;
    HDC hMemDC;
    HBITMAP hBitmap;

};

WinAPIGrabber::WinAPIGrabber(QObject * parent, GrabberContext *context)
    : GrabberBase(parent, context)
{
}

WinAPIGrabber::~WinAPIGrabber()
{
    freeScreens();
}

void WinAPIGrabber::freeScreens()
{
    for (int i = 0; i < _screensWithWidgets.size(); ++i) {
        WinAPIScreenData *d = reinterpret_cast<WinAPIScreenData *>(_screensWithWidgets[i].associatedData);
        if (d->hScreenDC)
            DeleteObject(d->hScreenDC);

        if (d->hBitmap)
            DeleteObject(d->hBitmap);

        if (d->hMemDC)
            DeleteObject(d->hMemDC);

        delete d;

        if (_screensWithWidgets[i].imgData != NULL) {
            free(_screensWithWidgets[i].imgData);
            _screensWithWidgets[i].imgData = NULL;
            _screensWithWidgets[i].imgDataSize = 0;
        }
    }

    _screensWithWidgets.clear();
}

QList< ScreenInfo > * WinAPIGrabber::screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabWidget *> &grabWidgets)
{
    result->clear();
    for (int i = 0; i < grabWidgets.size(); ++i) {
        HMONITOR hMonitorNew = MonitorFromWindow(reinterpret_cast<HWND>(grabWidgets[i]->winId()), MONITOR_DEFAULTTONULL);

        if (hMonitorNew != NULL) {
            MONITORINFO monitorInfo;

            ZeroMemory( &monitorInfo, sizeof(MONITORINFO) );
            monitorInfo.cbSize = sizeof(MONITORINFO);

            GetMonitorInfo( hMonitorNew, &monitorInfo );

            LONG left = monitorInfo.rcMonitor.left;
            LONG top = monitorInfo.rcMonitor.top;
            LONG right = monitorInfo.rcMonitor.right;
            LONG bottom = monitorInfo.rcMonitor.bottom;
            ScreenInfo screenInfo;
            screenInfo.rect = QRect(left, top, right - left, bottom - top);
            screenInfo.handle = hMonitorNew;

            if (!result->contains(screenInfo))
                result->append(screenInfo);
        }

    }
    return result;
}

bool WinAPIGrabber::reallocate(const QList< ScreenInfo > &screens)
{
    freeScreens();

    for (int i = 0; i < screens.size(); ++i) {

        const ScreenInfo screen = screens[i];

        long width = screen.rect.width();
        long height = screen.rect.height();

        DEBUG_LOW_LEVEL << "dimensions " << width << "x" << height << screen.handle;

        WinAPIScreenData *d = new WinAPIScreenData();

        d->hScreenDC = CreateDC( TEXT("DISPLAY"), NULL, NULL, NULL );

        // Create a bitmap compatible with the screen DC
        d->hBitmap = CreateCompatibleBitmap( d->hScreenDC, width, height );

        // Create a memory DC compatible to screen DC
        d->hMemDC = CreateCompatibleDC( d->hScreenDC );

        // Select new bitmap into memory DC
        SelectObject( d->hMemDC, d->hBitmap );

        BITMAP bmp;
        memset(&bmp, 0, sizeof(BITMAP));

        // Now get the actual Bitmap
        GetObject( d->hBitmap, sizeof(BITMAP), &bmp );

        // Calculate the size the buffer needs to be
        unsigned pixelsBuffSizeNew = bmp.bmWidthBytes * bmp.bmHeight;

        DEBUG_LOW_LEVEL << Q_FUNC_INFO << "pixelsBuffSize =" << pixelsBuffSizeNew;

        // The amount of bytes per pixel is the amount of bits divided by 8
        bytesPerPixel = bmp.bmBitsPixel / 8;

        if( bytesPerPixel != 4 ){
            qCritical() << "Not 32-bit mode is not supported!" << bytesPerPixel;
        }

        GrabbedScreen grabScreen;
        grabScreen.imgDataSize = pixelsBuffSizeNew;
        grabScreen.imgData = (BYTE *)malloc(grabScreen.imgDataSize);
        grabScreen.imgFormat = BufferFormatArgb;
        grabScreen.screenInfo = screen;
        grabScreen.associatedData = d;
        _screensWithWidgets.append(grabScreen);
    }


    return true;
}

GrabResult WinAPIGrabber::grabScreens()
{
    foreach(GrabbedScreen screen, _screensWithWidgets) {
        QRect screenRect = screen.screenInfo.rect;
        WinAPIScreenData *d = reinterpret_cast<WinAPIScreenData *>(screen.associatedData);

        BitBlt( d->hMemDC, 0, 0, screenRect.width(), screenRect.height(), d->hScreenDC,
                screenRect.left(), screenRect.top(), SRCCOPY );

        // Get the actual RGB data and put it into pbPixelsBuff
        GetBitmapBits( d->hBitmap, (LONG)screen.imgDataSize, screen.imgData );
    }
    return GrabResultOk;
}

/*
void WinAPIGrabber::updateMonitorInfo()
{
    ZeroMemory( &monitorInfo, sizeof(MONITORINFO) );
    monitorInfo.cbSize = sizeof(MONITORINFO);

    // Get position and resolution of the monitor
    GetMonitorInfo( hMonitor, &monitorInfo );

    screenWidth  = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    screenHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "screenWidth x screenHeight" << screenWidth << "x" << screenHeight;

    freeDCs();

    // CreateDC for multiple monitors
    hScreenDC = CreateDC( TEXT("DISPLAY"), NULL, NULL, NULL );

    // Create a bitmap compatible with the screen DC
    hBitmap = CreateCompatibleBitmap( hScreenDC, screenWidth, screenHeight );

    // Create a memory DC compatible to screen DC
    hMemDC = CreateCompatibleDC( hScreenDC );

    // Select new bitmap into memory DC
    SelectObject( hMemDC, hBitmap );
}
void WinAPIGrabber::resizePixelsBuffer()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Allocate memory for pbPixelsBuff and update pixelsBuffSize, bytesPerPixel";
    BITMAP bmp;
    memset(&bmp, 0, sizeof(BITMAP));

    // Now get the actual Bitmap
    GetObject( hBitmap, sizeof(BITMAP), &bmp );

    // Calculate the size the buffer needs to be
    unsigned pixelsBuffSizeNew = bmp.bmWidthBytes * bmp.bmHeight;

    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "pixelsBuffSize =" << pixelsBuffSizeNew;

    // ReAllocate memory for new buffer size
    pbPixelsBuff.resize(pixelsBuffSizeNew);

    // The amount of bytes per pixel is the amount of bits divided by 8
    bytesPerPixel = bmp.bmBitsPixel / 8;

    if( bytesPerPixel != 4 ){
        qDebug() << "Not 32-bit mode is not supported!" << bytesPerPixel;
    }
}

void WinAPIGrabber::updateGrabMonitor(QWidget *widget)
{
    HMONITOR hMonitorNew = MonitorFromWindow(reinterpret_cast<HWND>(widget->winId()), MONITOR_DEFAULTTONEAREST);
    if (hMonitor != hMonitorNew) {
        hMonitor = hMonitorNew;

        updateMonitorInfo();
        resizePixelsBuffer();
    }
}

GrabResult WinAPIGrabber::_grab(QList<QRgb> &grabResult, const QList<GrabWidget *> &grabWidgets)
{
    captureScreen();
    grabResult.clear();
    foreach(GrabWidget * widget, grabWidgets) {
        grabResult.append( widget->isAreaEnabled() ? getColor(widget) : qRgb(0,0,0) );
    }
    return GrabResultOk;
}


void WinAPIGrabber::captureScreen()
{
    DEBUG_HIGH_LEVEL << Q_FUNC_INFO;

    for (int i = 0; i < _screensWithWidgets.size(); ++i) {

        GrabbedScreen screen = _screensWithWidgets[i];
        QRect screenRect = screen.screenInfo.rect;
        WinAPIScreenData *d = reinterpret_cast<WinAPIScreenData *>(_screensWithWidgets[i].associatedData);

        BitBlt( d->hMemDC, 0, 0, screenRect.width(), screenRect.height(), d->hScreenDC,
                screenRect.left(), screenRect.top(), SRCCOPY );

        // Get the actual RGB data and put it into pbPixelsBuff
        GetBitmapBits( hBitmap, pbPixelsBuff.size(), &pbPixelsBuff[0] );
    }

    // Copy screen
}

QRgb WinAPIGrabber::getColor(const QWidget * grabme)
{
    DEBUG_HIGH_LEVEL << Q_FUNC_INFO;

    QRect widgetRect = grabme->frameGeometry();
    return getColor(widgetRect);
}

QRgb WinAPIGrabber::getColor(const QRect &widgetRect)
{
    DEBUG_HIGH_LEVEL << Q_FUNC_INFO << Debug::toString(widgetRect);

    if (pbPixelsBuff.empty())
    {
        qCritical() << Q_FUNC_INFO << "pbPixelsBuff is empty!";
        return 0;
    }

    RECT rcMonitor = monitorInfo.rcMonitor;
    QRect monitorRect = QRect( QPoint(rcMonitor.left, rcMonitor.top), QPoint(rcMonitor.right-1, rcMonitor.bottom-1));

    QRect clippedRect = monitorRect.intersected(widgetRect);

    // Checking for the 'grabme' widget position inside the monitor that is used to capture color
    if( !clippedRect.isValid() ){

        DEBUG_MID_LEVEL << "Widget 'grabme' is out of screen:" << Debug::toString(clippedRect);

        // Widget 'grabme' is out of screen
        return 0x000000;
    }

    // Convert coordinates from "Main" desktop coord-system to capture-monitor coord-system
    QRect preparedRect = clippedRect.translated(-monitorRect.x(), -monitorRect.y());

    // Align width by 4 for accelerated calculations
    preparedRect.setWidth(preparedRect.width() - (preparedRect.width() % 4));

    if( !preparedRect.isValid() ){
        qWarning() << Q_FUNC_INFO << " preparedRect is not valid:" << Debug::toString(preparedRect);

        // width and height can't be negative
        return 0x000000;
    }

    using namespace Grab;
    QRgb avgColor;
    if (Calculations::calculateAvgColor(&avgColor, &pbPixelsBuff[0], BufferFormatArgb, screenWidth * bytesPerPixel, preparedRect ) == 0) {
        return avgColor;
    } else {
        return qRgb(0,0,0);
    }

#if 0
    if (screenWidth < 1920 && (r > 120 || g > 120 || b > 120)) {
        int monitorWidth = screenWidth;
        int monitorHeight = screenHeight;
        const int BytesPerPixel = 4;
        // Save image of screen:
        QImage * im = new QImage( monitorWidth, monitorHeight, QImage::Format_RGB32 );
        for(int i=0; i<monitorWidth; i++){
            for(int j=0; j<monitorHeight; j++){
                index = (BytesPerPixel * j * monitorWidth) + (BytesPerPixel * i);
                QRgb rgb = pbPixelsBuff[index+2] << 16 | pbPixelsBuff[index+1] << 8 | pbPixelsBuff[index];
                im->setPixel(i, j, rgb);
            }
        }
        im->save("screen.jpg");
        delete im;
    }
#endif

}

*/
#endif // WINAPI_GRAB_SUPPORT

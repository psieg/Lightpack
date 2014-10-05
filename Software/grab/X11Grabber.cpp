/*
 * X11Grabber.cpp
 *
 *  Created on: 25.06.11
 *     Project: Lightpack
 *
 *  Copyright (c) 2011 Andrey Isupov, Timur Sattarov, Mike Shatohin
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

#include "X11Grabber.hpp"

#ifdef X11_GRAB_SUPPORT

#include <X11/Xutil.h>
// x shared-mem extension
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <cmath>
#include <sys/ipc.h>
#include <errno.h>
#include <inttypes.h>

#include "common/DebugOut.hpp"

struct X11GrabberData
{
    X11GrabberData()
        : image(NULL)
    {
        memset(&shminfo, 0, sizeof(shminfo));
    }

    XImage *image;
    XShmSegmentInfo shminfo;
};

X11Grabber::X11Grabber(QObject *parent, GrabberContext * context)
    : GrabberBase(parent, context)
{
    _display = XOpenDisplay(NULL);
}

X11Grabber::~X11Grabber()
{
    freeScreens();
    XCloseDisplay(_display);
}

QList<ScreenInfo> * X11Grabber::screensWithWidgets(QList<ScreenInfo> *result, const QList<GrabWidget *> &grabWidgets)
{
    result->clear();

    for (int i = 0; i < ScreenCount(_display); ++i) {
        XWindowAttributes xwa;
        XGetWindowAttributes(_display, RootWindow(_display, i), &xwa);
        ScreenInfo screen;
        intptr_t handle = i;
        screen.handle = reinterpret_cast<void *>(handle);
        screen.rect = QRect(xwa.x, xwa.y, xwa.width, xwa.height);
        for (int k = 0; k < grabWidgets.size(); ++k) {
            if (screen.rect.intersects(grabWidgets[k]->rect())) {
                result->append(screen);
                break;
            }
        }
    }

    return result;
}

void X11Grabber::freeScreens()
{
    for (int i = 0; i < _screensWithWidgets.size(); ++i) {
        X11GrabberData *d = reinterpret_cast<X11GrabberData *>(_screensWithWidgets[i].associatedData);
        XShmDetach(_display, &d->shminfo);
        XDestroyImage(d->image);
        shmdt (d->shminfo.shmaddr);
        shmctl(d->shminfo.shmid, IPC_RMID, 0);
        delete d;
        d = NULL;
    }

    _screensWithWidgets.clear();
}

bool X11Grabber::reallocate(const QList<ScreenInfo> &screens)
{
    freeScreens();

    for (int i = 0; i < screens.size(); ++i) {

        long width = screens[i].rect.width();
        long height = screens[i].rect.height();

        DEBUG_HIGH_LEVEL << "dimensions " << width << "x" << height << screens[i].handle;

        X11GrabberData *d = new X11GrabberData();

        int screenid = reinterpret_cast<intptr_t>(screens[i].handle);

        Screen * xscreen = ScreenOfDisplay(_display, screenid);

        d->image = XShmCreateImage(_display, DefaultVisualOfScreen(xscreen),
                                   DefaultDepthOfScreen(xscreen),
                                   ZPixmap, NULL, &d->shminfo,
                                   width, height );
        uint imagesize;
        imagesize = d->image->bytes_per_line * d->image->height;
        d->shminfo.shmid = shmget(    IPC_PRIVATE,
                                      imagesize,
                                      IPC_CREAT|0777
                                      );
        if (d->shminfo.shmid == -1) {
            qCritical() << Q_FUNC_INFO << " error occured while trying to get shared memory: " << strerror(errno);
        }

        char* mem = (char*)shmat(d->shminfo.shmid, 0, 0);
        d->shminfo.shmaddr = mem;
        d->image->data = mem;
        d->shminfo.readOnly = False;

        XShmAttach(_display, &d->shminfo);

        GrabbedScreen grabScreen;
        grabScreen.imgData = (unsigned char *)mem;
        grabScreen.imgFormat = BufferFormatArgb;
        grabScreen.screenInfo = screens[i];
        grabScreen.associatedData = d;
        _screensWithWidgets.append(grabScreen);
    }


    return true;
}

GrabResult X11Grabber::grabScreens()
{
    for (int i = 0; i < _screensWithWidgets.size(); ++i) {
        XShmGetImage(_display,
                     RootWindow(_display, reinterpret_cast<intptr_t>(_screensWithWidgets[i].screenInfo.handle)),
                     reinterpret_cast<X11GrabberData *>(_screensWithWidgets[i].associatedData)->image,
                     0,
                     0,
                     0x00FFFFFF
                     );
    }
#if 0
    DEBUG_LOW_LEVEL << "QImage";
    QImage *pic = new QImage(1024,768,QImage::Format_RGB32);
    DEBUG_LOW_LEVEL << "format";
    unsigned long pixel;
    for (int y = 0; y < 768; y++)
    {
        for (int x = 0; x < 1024; x++)
        {
            pixel = XGetPixel(image, x, y);
            int r = (pixel >> 16) & 0xff;
            int g = (pixel >>  8) & 0xff;
            int b = (pixel >>  0) & 0xff;
            //QRgb rgb = QRgb() (r,g,b);
            pic->setPixel(x,y,pixel);
        }
    }
    DEBUG_LOW_LEVEL << "save";
    pic->save("/home/atarity/.Lightpack/test.bmp");
#endif
    /*
    _context->grabResult->clear();
    foreach(GrabWidget * widget, *_context->grabWidgets) {
        _context->grabResult->append( widget->isAreaEnabled() ? getColor(widget) : qRgb(0,0,0) );
    }
    return GrabResultOk;
    */
    return GrabResultOk;
}

#endif // X11_GRAB_SUPPORT

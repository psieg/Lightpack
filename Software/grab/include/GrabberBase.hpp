/*
 * GrabberBase.hpp
 *
 *  Created on: 18.07.2012
 *     Project: Lightpack
 *
 *  Copyright (c) 2012 Timur Sattarov
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

#include <QColor>
#include <QList>
#include <QRect>
#include <QScopedPointer>
#include <QTimer>

#include "common/BufferFormat.h"

class GrabberContext;
class GrabbedArea;

enum GrabResult {
    GrabResultOk,
    GrabResultFrameNotReady,
    GrabResultError
};

struct ScreenInfo {
    ScreenInfo()
        : handle(NULL)
    {}
    QRect rect;
    void * handle;
    bool operator== (const ScreenInfo &other) const {
        return other.rect == this->rect;
    }
};

struct GrabbedScreen {
    GrabbedScreen()
        : imgFormat(BufferFormatUnknown)
        , associatedData(NULL)
    {}
    unsigned char * imgData;
    size_t imgDataSize;
    BufferFormat imgFormat;
    ScreenInfo screenInfo;
    void * associatedData;
};

#define DECLARE_GRABBER_NAME(grabber_name) \
    virtual const char * name() const { \
        static const char * static_grabber_name = (grabber_name); \
        return static_grabber_name; \
    }

/*!
  Base class which represents each particular grabber. If you want to add a new grabber just add implementation of \code GrabberBase \endcode
  and modify \a GrabManager
*/
class GrabberBase : public QObject
{
    Q_OBJECT
public:

    /*!
     \param parent standart Qt-specific owner
     \param grabResult \code QList \endcode to write results of grabbing to
     \param grabWidgets List of GrabbedArea
    */
    GrabberBase(QObject * parent, GrabberContext * grabberContext);
    virtual ~GrabberBase() {}

    virtual const char * name() const = 0;

public slots:
    virtual void startGrabbing();
    virtual void stopGrabbing();
    virtual bool isGrabbingStarted() const;
    virtual void setGrabInterval(int msec);

    virtual void grab();

protected slots:
    /*!
      Grabs screens and saves them to \a GrabberBase#_screensWithWidgets field. Called by
      \a GrabberBase#grab() slot. Needs to be implemented in derived classes.
      \return GrabResult
    */
    virtual GrabResult grabScreens() = 0;

    /*!
     * Frees unnecessary resources and allocates needed ones based on \a ScreenInfo
     * \param grabScreens
     * \return
     */
    virtual bool reallocate(const QList< ScreenInfo > &grabScreens) = 0;

    /*!
     * Get all screens grab widgets lies on.
     * \param result
     * \param grabWidgets
     * \return
     */
    virtual QList< ScreenInfo > * screensWithWidgets(QList< ScreenInfo > * result, const QList<GrabbedArea *> &grabWidgets) = 0;

    virtual bool isReallocationNeeded(const QList< ScreenInfo > &grabScreens) const;

protected:
    const GrabbedScreen * screenOfRect(const QRect &rect) const;

signals:
    void frameGrabAttempted(GrabResult grabResult);

    /*!
      Signals \a GrabManager that the grabber wants to be started or stopped
    */
    void grabberStateChangeRequested(bool isStartRequested);

protected:
    GrabberContext *_context;
    GrabResult _lastGrabResult;
    int grabScreensCount;
    QList<GrabbedScreen> _screensWithWidgets;
    QScopedPointer<QTimer> m_timer;
};

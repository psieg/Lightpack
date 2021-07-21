//
//  SystemSession.hpp
//  Prismatik
//
//  Created by zomfg on 27/12/2018.
//
#pragma once
#ifndef SYSTEM_SESSION_HPP
#define SYSTEM_SESSION_HPP

#include "QObject"
#include "QAbstractNativeEventFilter"

namespace SystemSession {
    
    enum Status : int {
        Ending,
        Locking,
        Unlocking,
        Sleeping,
        Resuming,
        DisplayOn,
        DisplayOff,
        DisplayDimmed
    };
    
    class EventFilter :
    public QObject,
    public QAbstractNativeEventFilter
    {
        Q_OBJECT
    public:
        EventFilter() = default;
        virtual ~EventFilter() = default;
        
        static EventFilter* create();

    signals:
        void sessionChangeDetected(Status status);
    };
}

#endif /* SYSTEM_SESSION_HPP */

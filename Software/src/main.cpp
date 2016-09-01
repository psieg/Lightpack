/*
 * main.cpp
 *
 *  Created on: 21.07.2010
 *      Author: Mike Shatohin (brunql)
 *     Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#include <QtGui>
#include <QMetaType>
#include <iostream>

#include "common/DebugOut.hpp"
#include "LightpackApplication.hpp"
#include "Settings.hpp"
#include "LogWriter.hpp"
#include "SettingsWizard.hpp"

#ifdef Q_OS_WIN
#if !defined NOMINMAX
#define NOMINMAX
#endif

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef Q_OS_MACX
#import <Foundation/NSProcessInfo.h>
#import <CoreServices/CoreServices.h>
#endif

using namespace std;

unsigned g_debugLevel = SettingsScope::Main::DebugLevelDefault;

QString getApplicationDirectoryPath(const char * firstCmdArgument)
{    
    QFileInfo fileInfo(firstCmdArgument);
    QString appDirPath = fileInfo.absoluteDir().absolutePath();

    QString lightpackMainConfPath = appDirPath + "/main.conf";

    cout << lightpackMainConfPath.toStdString() << endl;

    if (QFile::exists(lightpackMainConfPath))
    {
        /// Portable version
        /// Store logs and settings in application directory

        cout << "Portable version" << endl;
    }
    else
    {
        /// Unportable version
        /// Store logs and settings in home directory of the current user

        cout << "Unportable version" << endl;

        QString home = QDir::homePath();
        QString normalizedHome = home.endsWith("/") ? home.left(home.size() - 1) : home;

#       ifdef Q_OS_WIN
        appDirPath = normalizedHome  + "/Prismatik";
#       else
        appDirPath = normalizedHome + "/.Prismatik";
#       endif

        QDir dir(appDirPath);
        if (dir.exists() == false)
        {
            cout << "mkdir " << appDirPath.toStdString() << endl;
            if (dir.mkdir(appDirPath) == false)
            {
                cerr << "Failed mkdir '" << appDirPath.toStdString() << "' for application generated stuff. Exit." << endl;
                exit(LightpackApplication::AppDirectoryCreationFail_ErrorCode);
            }
        }
    }

    cout << "Application directory: " << appDirPath.toStdString() << endl;

    return appDirPath;
}

int main(int argc, char **argv)
{

#if defined Q_OS_MACX && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_9
    id activity;
    SInt32 version = 0;
    Gestalt( gestaltSystemVersion, &version );
    bool endActivityRequired = false;
    if ( version >= 0x1090 ) {
      activity = [[NSProcessInfo processInfo] beginActivityWithOptions: NSActivityLatencyCritical reason:@"Prismatik is latency-critical app"];
      endActivityRequired = true;
      DEBUG_LOW_LEVEL << "Latency critical activity is started";
    }
#elif defined Q_OS_WIN
    // Mutex used by InnoSetup to detect that app is runnning.
    // http://www.jrsoftware.org/ishelp/index.php?topic=setup_appmutex
    HANDLE appMutex = CreateMutexW(NULL, FALSE, L"LightpackAppMutex");
#endif

    const QString appDirPath = getApplicationDirectoryPath(argv[0]);

    LogWriter logWriter;
    const int logInitResult = logWriter.initWith(appDirPath + "/Logs");
    if (logInitResult != LightpackApplication::OK_ErrorCode) {
        exit(logInitResult);
    }

    LogWriter::ScopedMessageHandler messageHandlerGuard(&logWriter);
    Q_UNUSED(messageHandlerGuard);

    LightpackApplication lightpackApp(argc, argv);
    lightpackApp.setLibraryPaths(QStringList(appDirPath + "/plugins"));
    lightpackApp.initializeAll(appDirPath);

    if (lightpackApp.isRunning())
    {
        lightpackApp.sendMessage("Application already running");

        qWarning() << "Application already running";
        exit(0);
    }

    Q_INIT_RESOURCE(LightpackResources);

    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "thread id: " << lightpackApp.thread()->currentThreadId();

    qDebug() << "Start main event loop: lightpackApp.exec();";

    int returnCode = lightpackApp.exec();

#if defined Q_OS_MACX && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_9
    if (endActivityRequired)
      [[NSProcessInfo processInfo] endActivity: activity];
#elif defined Q_OS_WIN
    CloseHandle(appMutex);
#endif

    return returnCode;
}

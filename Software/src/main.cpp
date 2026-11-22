/*
 * main.cpp
 *
 *	Created on: 21.07.2010
 *		Author: Mike Shatohin (brunql)
 *		Project: Lightpack
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
 *
 *	Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *	Lightpack is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Lightpack is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QtGui>
#include <QMetaType>
#include <iostream>

#include "LightpackApplication.hpp"
#include "Settings.hpp"
#include "debug.h"
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

#ifdef Q_OS_MACOS
#import <Foundation/NSProcessInfo.h>
#import <CoreServices/CoreServices.h>
#endif

using namespace std;

unsigned g_debugLevel = SettingsScope::Main::DebugLevelDefault;

QString getApplicationDirectoryPath(const char * firstCmdArgument)
{
	QFileInfo fileInfo(firstCmdArgument);
	QString appDirPath = fileInfo.absoluteDir().absolutePath();

	QString lightpackMainConfPath = appDirPath + QStringLiteral("/main.conf");

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
		QString normalizedHome = home.endsWith(QStringLiteral("/")) ? home.left(home.size() - 1) : home;

#		ifdef Q_OS_WIN
		appDirPath = normalizedHome	+ QStringLiteral("/Prismatik");
#		else
		appDirPath = normalizedHome + QStringLiteral("/.Prismatik");
#		endif

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

#if defined Q_OS_MACOS && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_9
    bool canUseLatencyCriticalOption = false;
    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(isOperatingSystemAtLeastVersion:)]) // available since 10.10, so canUseLatencyCriticalOption will always be true
        canUseLatencyCriticalOption = [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:NSOperatingSystemVersion{10,9,0}];
    else {// fallback to old method, deprecation can be suppressed
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        SInt32 version = 0;
        Gestalt(gestaltSystemVersion, &version);
        canUseLatencyCriticalOption = (version >= 0x1090);
#pragma clang diagnostic pop
    }
    id activity;
    bool endActivityRequired = false;
    if (canUseLatencyCriticalOption) {
		activity = [[NSProcessInfo processInfo] beginActivityWithOptions: NSActivityLatencyCritical reason:@"Prismatik is latency-critical app"];
		endActivityRequired = true;
		DEBUG_LOW_LEVEL << "Latency critical activity is started";
	}
#elif defined Q_OS_WIN
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif
	// Remove current working directory and PATH from default DLL search order to prevent DLL sideloading.
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
	SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);

	// Mutex used by InnoSetup to detect that app is runnning.
	// http://www.jrsoftware.org/ishelp/index.php?topic=setup_appmutex
	HANDLE appMutex = CreateMutexW(NULL, FALSE, L"LightpackAppMutex");
#endif

	LogWriter logWriter;

	LogWriter::ScopedMessageHandler messageHandlerGuard(&logWriter);
	Q_UNUSED(messageHandlerGuard);

	LightpackApplication lightpackApp(argc, argv);

	// init the logger after initializeAll to know the configured debugLevel

	const int logInitResult = g_debugLevel > 0 ? logWriter.initEnabled(lightpackApp.configDir() + "/Logs") : logWriter.initDisabled(lightpackApp.configDir() + "/Logs");
	if (logInitResult != LightpackApplication::OK_ErrorCode)
		exit(logInitResult);

	Q_INIT_RESOURCE(LightpackResources);

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "thread id: " << lightpackApp.thread()->currentThreadId();

	qDebug() << "Start main event loop: lightpackApp.exec();";

	int returnCode = lightpackApp.exec();

#if defined Q_OS_MACOS && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_9
	if (endActivityRequired)
		[[NSProcessInfo processInfo] endActivity: activity];
#elif defined Q_OS_WIN
	CloseHandle(appMutex);
#endif

	return returnCode;
}

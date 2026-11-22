/*
 * ApiServer.hpp
 *
 *	Created on: 07.09.2011
 *		Authors: Andrey Isupov && Mike Shatohin
 *		Project: Lightpack
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#pragma once

#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
#include <QRgb>
#include <QTime>
#include "SettingsWindow.hpp"
#include "LightpackPluginInterface.hpp"
#include "ApiServerSetColorTask.hpp"
#include "debug.h"
#include "enums.hpp"

struct ClientInfo
{
	bool isAuthorized = false;
	QString sessionKey;
	int unknownCmdCount = 0;
	int failedAuthCount = 0;
	// Think about it. May be we need to save gamma,
	// smooth and brightness and after success lock send
	// this values to device?
};

class ApiServer : public QTcpServer
{
	Q_OBJECT

public:
	ApiServer(QObject *parent = 0);
	ApiServer(quint16 port, QObject *parent = 0);
	~ApiServer();

	void setInterface(LightpackPluginInterface *lightpackInterface);
	void firstStart();

public:
	static const char * const ApiVersion;
	static const char * const CmdUnknown;
	static const char * const CmdDeprecated;
	static const char * const CmdExit;
	static const char * const CmdHelp;
	static const char * const CmdHelpShort;

	static const char * const CmdApiKey;
	static const char * const CmdApiKeyResult_Ok;
	static const char * const CmdApiKeyResult_Fail;
	static const char * const CmdApiCheck_AuthRequired;

	static const char * const CmdGetStatus;
	static const char * const CmdResultStatus_On;
	static const char * const CmdResultStatus_Off;
	static const char * const CmdResultStatus_DeviceError;
	static const char * const CmdResultStatus_Unknown;

	static const char * const CmdGetStatusAPI;
	static const char * const CmdResultStatusAPI_Busy;
	static const char * const CmdResultStatusAPI_Idle;

	static const char * const CmdGetProfiles;
	static const char * const CmdResultProfiles;

	static const char * const CmdGetProfile;
	static const char * const CmdResultProfile;

	static const char * const CmdGetDevices;
	static const char * const CmdResultDevices;

	static const char * const CmdGetDevice;
	static const char * const CmdResultDevice;

	static const char * const CmdGetMaxLeds;
	static const char * const CmdResultMaxLeds;

	static const char * const CmdGetCountLeds;
	static const char * const CmdResultCountLeds;

	static const char * const CmdGetLeds;
	static const char * const CmdResultLeds;

	static const char * const CmdGetColors;
	static const char * const CmdResultGetColors;

	static const char * const CmdGetFPS;
	static const char * const CmdResultFPS;

	static const char * const CmdGetScreenSize;
	static const char * const CmdResultScreenSize;

	static const char * const CmdGetCountMonitor;
	static const char * const CmdResultCountMonitor;
	static const char * const CmdGetSizeMonitor;
	static const char * const CmdResultSizeMonitor;

	static const char * const CmdGetBacklight;
	static const char * const CmdResultBacklight_Ambilight;
	static const char * const CmdResultBacklight_Moodlamp;
#ifdef SOUNDVIZ_SUPPORT
	static const char * const CmdResultBacklight_SoundViz;
#endif

	static const char * const CmdGetGamma;
	static const char * const CmdResultGamma;

	static const char * const CmdGetBrightness;
	static const char * const CmdResultBrightness;

	static const char * const CmdGetSmooth;
	static const char * const CmdResultSmooth;

#ifdef SOUNDVIZ_SUPPORT
	static const char * const CmdGetSoundVizColors;
	static const char * const CmdResultSoundVizColors;

	static const char * const CmdGetSoundVizLiquid;
	static const char * const CmdResultSoundVizLiquid;
#endif

	static const char * const CmdGetPersistOnUnlock;
	static const char * const CmdGetPersistOnUnlock_On;
	static const char * const CmdGetPersistOnUnlock_Off;

	static const char * const CmdGuid;

	static const char * const CmdLockStatus;

	static const char * const CmdLock;
	static const char * const CmdResultLock_Success;
	static const char * const CmdResultLock_Busy;

	static const char * const CmdUnlock;
	static const char * const CmdResultUnlock_Success;
	static const char * const CmdResultUnlock_NotLocked;

	// Set-commands works only after success lock
	static const char * const CmdSetResult_Ok;
	static const char * const CmdSetResult_Error;
	static const char * const CmdSetResult_Busy;
	static const char * const CmdSetResult_NotLocked;

	static const char * const CmdSetColor;
	static const char * const CmdSetGamma;
	static const char * const CmdSetBrightness;
	static const char * const CmdSetSmooth;
	static const char * const CmdSetProfile;

#ifdef SOUNDVIZ_SUPPORT
	static const char * const CmdSetSoundVizColors;
	static const char * const CmdSetSoundVizLiquid;
#endif

	static const char * const CmdSetDevice;

	static const char * const CmdSetCountLeds;
	static const char * const CmdSetLeds;

	static const char * const CmdNewProfile;
	static const char * const CmdDeleteProfile;

	static const char * const CmdSetStatus;
	static const char * const CmdSetStatus_On;
	static const char * const CmdSetStatus_Off;

	static const char * const CmdSetBacklight;
	static const char * const CmdSetBacklight_Ambilight;
	static const char * const CmdSetBacklight_Moodlamp;
#ifdef SOUNDVIZ_SUPPORT
	static const char * const CmdSetBacklight_SoundViz;
#endif

	static const char * const CmdSetPersistOnUnlock;
	static const char * const CmdSetPersistOnUnlock_On;
	static const char * const CmdSetPersistOnUnlock_Off;

	static const int SignalWaitTimeoutMs;

signals:
	void startParseSetColorTask(QByteArray buffer);
	void errorOnStartListening(QString errorMessage);
	void clearColorBuffers();
	void updateApiDeviceNumberOfLeds(int value);

public slots:
	void apiServerSettingsChanged();
	void updateApiKey(const QString &key);

protected:
	void incomingConnection(qintptr socketDescriptor);

private slots:
	void clientDisconnected();
	void clientProcessCommands();
	void taskSetColorIsSuccess(bool isSuccess);

private:
	static constexpr int MaxClients = 64;
	static constexpr qint64 MaxReadBufferSize = 65536;
	static constexpr qint64 MaxCommandLength = 32768;
	static constexpr int MaxConsecutiveUnknownCommands = 5;
	static constexpr int MaxFailedAuthAttempts = 5;

	LightpackPluginInterface *lightpack = nullptr;
	quint64 m_nextClientId = 0;
	void initPrivateVariables();
	void initApiSetColorTask();
	void startListening();
	void stopListening();
	void abortClient(QTcpSocket *client);
	static bool isCrossProtocolLine(const QByteArray &line);
	static bool constantTimeKeyEquals(const QByteArray &candidate, const QString &expected);
	void writeData(QTcpSocket* client, const QString & data);
	QString formatHelp(const QString & cmd);
	QString formatHelp(const QString & cmd, const QString & description);
	QString formatHelp(const QString & cmd, const QString & description, const QString & results);
	QString formatHelp(const QString & cmd, const QString & description, const QString & examples, const QString & results);
	void initHelpMessage();
	void initShortHelpMessage();

private:
	int m_apiPort;
	bool m_listenOnlyOnLoInterface;
	QString m_apiAuthKey;
	bool m_isAuthEnabled;
	QElapsedTimer m_timer;

	QMap <QTcpSocket*, ClientInfo> m_clients;

	QThread *m_apiSetColorTaskThread;
	ApiServerSetColorTask *m_apiSetColorTask;

	bool m_isTaskSetColorDone;
	bool m_isTaskSetColorParseSuccess;

	QString m_helpMessage;
	QString m_shortHelpMessage;
};

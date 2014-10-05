/*
 * ApiServer.hpp
 *
 *  Created on: 07.09.2011
 *     Authors: Andrey Isupov && Mike Shatohin
 *     Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#include <QDesktopWidget>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
#include <QRgb>
#include <QTime>

#include "common/DebugOut.hpp"
#include "SettingsWindow.hpp"
#include "LightpackPluginInterface.hpp"
#include "ApiServerSetColorTask.hpp"
#include "enums.hpp"

struct ClientInfo
{
    bool isAuthorized;
    QString sessionKey;
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
    static const char * ApiVersion;   
    static const char * CmdUnknown;
    static const char * CmdExit;
    static const char * CmdHelp;
    static const char * CmdHelpShort;

    static const char * CmdApiKey;
    static const char * CmdApiKeyResult_Ok;
    static const char * CmdApiKeyResult_Fail;
    static const char * CmdApiCheck_AuthRequired;

    static const char * CmdGetStatus;
    static const char * CmdResultStatus_On;
    static const char * CmdResultStatus_Off;
    static const char * CmdResultStatus_DeviceError;
    static const char * CmdResultStatus_Unknown;

    static const char * CmdGetStatusAPI;
    static const char * CmdResultStatusAPI_Busy;
    static const char * CmdResultStatusAPI_Idle;

    static const char * CmdGetProfiles;
    static const char * CmdResultProfiles;

    static const char * CmdGetProfile;
    static const char * CmdResultProfile;

    static const char * CmdGetDevices;
    static const char * CmdResultDevices;

    static const char * CmdGetDevice;
    static const char * CmdResultDevice;

    static const char * CmdGetMaxLeds;
    static const char * CmdResultMaxLeds;

    static const char * CmdGetCountLeds;
    static const char * CmdResultCountLeds;

    static const char * CmdGetLeds;
    static const char * CmdResultLeds;

    static const char * CmdGetColors;
    static const char * CmdResultGetColors;

    static const char * CmdGetFPS;
    static const char * CmdResultFPS;

    static const char * CmdGetScreenSize;
    static const char * CmdResultScreenSize;

    static const char * CmdGetCountMonitor;
    static const char * CmdResultCountMonitor;
    static const char * CmdGetSizeMonitor;
    static const char * CmdResultSizeMonitor;

    static const char * CmdGetBacklight;
    static const char * CmdResultBacklight_Ambilight;
    static const char * CmdResultBacklight_Moodlamp;

    static const char * CmdGetGamma;
    static const char * CmdResultGamma;

    static const char * CmdGetBrightness;
    static const char * CmdResultBrightness;

    static const char * CmdGetSmooth;
    static const char * CmdResultSmooth;

    static const char * CmdGuid;

    static const char * CmdLockStatus;

    static const char * CmdLock;
    static const char * CmdResultLock_Success;
    static const char * CmdResultLock_Busy;

    static const char * CmdUnlock;
    static const char * CmdResultUnlock_Success;
    static const char * CmdResultUnlock_NotLocked;

    // Set-commands works only after success lock
    static const char * CmdSetResult_Ok;
    static const char * CmdSetResult_Error;
    static const char * CmdSetResult_Busy;
    static const char * CmdSetResult_NotLocked;

    static const char * CmdSetColor;
    static const char * CmdSetGamma;
    static const char * CmdSetBrightness;
    static const char * CmdSetSmooth;
    static const char * CmdSetProfile;

    static const char * CmdSetDevice;

    static const char * CmdSetCountLeds;
    static const char * CmdSetLeds;

    static const char * CmdNewProfile;
    static const char * CmdDeleteProfile;

    static const char * CmdSetStatus;
    static const char * CmdSetStatus_On;
    static const char * CmdSetStatus_Off;

    static const char * CmdSetBacklight;
    static const char * CmdSetBacklight_Ambilight;
    static const char * CmdSetBacklight_Moodlamp;

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
    LightpackPluginInterface *lightpack;
    void initPrivateVariables();
    void initApiSetColorTask();
    void startListening();
    void stopListening();
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
    QTime m_time;

    QMap <QTcpSocket*, ClientInfo> m_clients;

    QThread *m_apiSetColorTaskThread;
    ApiServerSetColorTask *m_apiSetColorTask;

    bool m_isTaskSetColorDone;
    bool m_isTaskSetColorParseSuccess;

    QString m_helpMessage;
    QString m_shortHelpMessage;
};

/*
 * ApiServer.cpp
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

#include <QtNetwork>
#include <QCryptographicHash>
#include <stdlib.h>
#ifdef Q_OS_WIN
#include <winsock2.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <unistd.h>
#endif

#include "ApiServer.hpp"
#include "LightpackPluginInterface.hpp"
#include "ApiServerSetColorTask.hpp"
#include "Settings.hpp"
#include "TimeEvaluations.hpp"
#include "version.h"
#include <QApplication>

using namespace SettingsScope;

// Immediatly after successful connection server sends to client -- ApiVersion
const char * const ApiServer::ApiVersion = "Lightpack API v1.4 - Prismatik API v" API_VERSION " (type \"help\" for more info)\r\n";
const char * const ApiServer::CmdUnknown = "unknown command\r\n";
const char * const ApiServer::CmdDeprecated = "deprecated command, not supported\r\n";
const char * const ApiServer::CmdExit = "exit";
const char * const ApiServer::CmdHelp = "help";
const char * const ApiServer::CmdHelpShort = "?";

const char * const ApiServer::CmdApiKey = "apikey:";
const char * const ApiServer::CmdApiKeyResult_Ok = "ok\r\n";
const char * const ApiServer::CmdApiKeyResult_Fail = "fail\r\n";
const char * const ApiServer::CmdApiCheck_AuthRequired = "authorization required\r\n";

const char * const ApiServer::CmdGetStatus = "getstatus";
const char * const ApiServer::CmdResultStatus_On = "status:on\r\n";
const char * const ApiServer::CmdResultStatus_Off = "status:off\r\n";
const char * const ApiServer::CmdResultStatus_DeviceError = "status:device error\r\n";
const char * const ApiServer::CmdResultStatus_Unknown = "status:unknown\r\n";

const char * const ApiServer::CmdGetStatusAPI = "getstatusapi";
const char * const ApiServer::CmdResultStatusAPI_Busy = "statusapi:busy\r\n";
const char * const ApiServer::CmdResultStatusAPI_Idle = "statusapi:idle\r\n";

const char * const ApiServer::CmdGetProfiles = "getprofiles";
// Necessary to add a new line after filling results!
const char * const ApiServer::CmdResultProfiles = "profiles:";

const char * const ApiServer::CmdGetProfile = "getprofile";
// Necessary to add a new line after filling results!
const char * const ApiServer::CmdResultProfile = "profile:";

const char * const ApiServer::CmdGetDevices = "getdevices";
const char * const ApiServer::CmdResultDevices = "devices:";
const char * const ApiServer::CmdGetDevice = "getdevice";
const char * const ApiServer::CmdResultDevice = "device:";

const char * const ApiServer::CmdGetMaxLeds = "getmaxleds";
const char * const ApiServer::CmdResultMaxLeds = "maxleds:";

const char * const ApiServer::CmdGetCountLeds = "getcountleds";
// Necessary to add a new line after filling results!
const char * const ApiServer::CmdResultCountLeds = "countleds:";

const char * const ApiServer::CmdGetLeds = "getleds";
const char * const ApiServer::CmdResultLeds = "leds:";

const char * const ApiServer::CmdGetColors = "getcolors";
const char * const ApiServer::CmdResultGetColors = "colors:";

const char * const ApiServer::CmdGetFPS = "getfps";
const char * const ApiServer::CmdResultFPS = "fps:";

const char * const ApiServer::CmdGetScreenSize = "getscreensize";
const char * const ApiServer::CmdResultScreenSize = "screensize:";

const char * const ApiServer::CmdGetCountMonitor = "getcountmonitors";
const char * const ApiServer::CmdResultCountMonitor = "countmonitors:";

const char * const ApiServer::CmdGetSizeMonitor = "getsizemonitor:";
const char * const ApiServer::CmdResultSizeMonitor = "sizemonitor:";

const char * const ApiServer::CmdGetBacklight = "getmode";
const char * const ApiServer::CmdResultBacklight_Ambilight = "mode:ambilight\r\n";
const char * const ApiServer::CmdResultBacklight_Moodlamp = "mode:moodlamp\r\n";
#ifdef SOUNDVIZ_SUPPORT
const char * const ApiServer::CmdResultBacklight_SoundViz = "mode:soundviz\r\n";
#endif

const char * const ApiServer::CmdGetGamma = "getgamma";
const char * const ApiServer::CmdResultGamma = "gamma:";

const char * const ApiServer::CmdGetBrightness = "getbrightness";
const char * const ApiServer::CmdResultBrightness = "brightness:";

const char * const ApiServer::CmdGetSmooth = "getsmooth";
const char * const ApiServer::CmdResultSmooth = "smooth:";

#ifdef SOUNDVIZ_SUPPORT
const char * const ApiServer::CmdGetSoundVizColors = "getsoundvizcolors";
const char * const ApiServer::CmdResultSoundVizColors = "soundvizcolors:";

const char * const ApiServer::CmdGetSoundVizLiquid = "getsoundvizliquid";
const char * const ApiServer::CmdResultSoundVizLiquid = "soundvizliquid:";
#endif
const char * const ApiServer::CmdGetPersistOnUnlock = "getpersistonunlock";
const char * const ApiServer::CmdGetPersistOnUnlock_On = "persistonunlock:on\r\n";
const char * const ApiServer::CmdGetPersistOnUnlock_Off = "persistonunlock:off\r\n";

const char * const ApiServer::CmdGuid = "guid:";

const char * const ApiServer::CmdLockStatus = "getlockstatus";

const char * const ApiServer::CmdLock = "lock";
const char * const ApiServer::CmdResultLock_Success = "lock:success\r\n";
const char * const ApiServer::CmdResultLock_Busy = "lock:busy\r\n";

const char * const ApiServer::CmdUnlock = "unlock";
const char * const ApiServer::CmdResultUnlock_Success = "unlock:success\r\n";
const char * const ApiServer::CmdResultUnlock_NotLocked = "unlock:not locked\r\n";

// Set-commands works only after success lock
// Set-commands can return, after self-name, only this results:
const char * const ApiServer::CmdSetResult_Ok = "ok\r\n";
const char * const ApiServer::CmdSetResult_Error = "error\r\n";
const char * const ApiServer::CmdSetResult_Busy = "busy\r\n";
const char * const ApiServer::CmdSetResult_NotLocked = "not locked\r\n";

// Set-commands contains at end semicolon!!!
const char * const ApiServer::CmdSetColor = "setcolor:";
const char * const ApiServer::CmdSetGamma = "setgamma:";
const char * const ApiServer::CmdSetBrightness = "setbrightness:";
const char * const ApiServer::CmdSetSmooth = "setsmooth:";
const char * const ApiServer::CmdSetProfile = "setprofile:";

#ifdef SOUNDVIZ_SUPPORT
const char * const ApiServer::CmdSetSoundVizColors = "setsoundvizcolors:";
const char * const ApiServer::CmdSetSoundVizLiquid = "setsoundvizliquid:";
#endif

const char * const ApiServer::CmdSetDevice = "setdevice:";

const char * const ApiServer::CmdSetCountLeds = "setcountleds:";
const char * const ApiServer::CmdSetLeds = "setleds:";

const char * const ApiServer::CmdNewProfile = "newprofile:";
const char * const ApiServer::CmdDeleteProfile = "deleteprofile:";

const char * const ApiServer::CmdSetStatus = "setstatus:";
const char * const ApiServer::CmdSetStatus_On = "on";
const char * const ApiServer::CmdSetStatus_Off = "off";

const char * const ApiServer::CmdSetBacklight = "setmode:";
const char * const ApiServer::CmdSetBacklight_Ambilight = "ambilight";
const char * const ApiServer::CmdSetBacklight_Moodlamp = "moodlamp";
#ifdef SOUNDVIZ_SUPPORT
const char * const ApiServer::CmdSetBacklight_SoundViz = "soundviz";
#endif

const char * const ApiServer::CmdSetPersistOnUnlock = "setpersistonunlock:";
const char * const ApiServer::CmdSetPersistOnUnlock_On = "on";
const char * const ApiServer::CmdSetPersistOnUnlock_Off = "off";

const int ApiServer::SignalWaitTimeoutMs = 1000; // 1 second

ApiServer::ApiServer(QObject *parent)
	: QTcpServer(parent)
{
	initPrivateVariables();
	initApiSetColorTask();
	initHelpMessage();
	initShortHelpMessage();
}

ApiServer::ApiServer(quint16 port, QObject *parent)
	: QTcpServer(parent)
{
	// This constructor is for using in ApiTests

	initPrivateVariables();
	initApiSetColorTask();
	initHelpMessage();
	initShortHelpMessage();

	m_apiPort = port;

	bool ok = listen(QHostAddress::Any, m_apiPort);
	if (!ok)
	{
		qFatal("%s listen(Any, %d) fail", Q_FUNC_INFO, m_apiPort);
	}
}

ApiServer::~ApiServer() {
	m_apiSetColorTaskThread->quit();
	m_apiSetColorTaskThread->wait();
}

void ApiServer::setInterface(LightpackPluginInterface *lightpackInterface)
{
	lightpack = lightpackInterface;
	if (!lightpack)
		return;
	QString test = lightpack->Version();
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << test;
	connect(m_apiSetColorTask, &ApiServerSetColorTask::taskParseSetColorDone, lightpack, &LightpackPluginInterface::updateLedsColors, Qt::QueuedConnection);
	connect(m_apiSetColorTask, &ApiServerSetColorTask::taskParseSetColorDone, lightpack, &LightpackPluginInterface::updateColorsCache, Qt::QueuedConnection);

}


void ApiServer::firstStart()
{
	// Call this function after connect all nesessary signals/slots

	if (Settings::isApiEnabled())
	{
		updateApiKey(Settings::getApiAuthKey());
		startListening();
	}
}

void ApiServer::apiServerSettingsChanged()
{
	initPrivateVariables();
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if (Settings::isApiEnabled()) {
		stopListening();
		startListening();
	}
	else
		stopListening();
}

void ApiServer::updateApiKey(const QString &key)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << (key.isEmpty() ? "empty" : "<redacted>");

	m_apiAuthKey = key;
	m_isAuthEnabled = !key.isEmpty();

	for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if (m_isAuthEnabled)
		{
			it.value().isAuthorized = false;
			if (lightpack && lightpack->CheckLock(it.value().sessionKey) == 1)
				lightpack->UnLock(it.value().sessionKey);
		}
		else
		{
			it.value().isAuthorized = true;
			it.value().failedAuthCount = 0;
		}
	}
}

void ApiServer::incomingConnection(qintptr socketDescriptor)
{
	if (m_clients.size() >= MaxClients)
	{
		qWarning() << Q_FUNC_INFO << "Max API clients reached, rejecting connection";
		QTcpSocket *rejected = new QTcpSocket(this);
		if (rejected->setSocketDescriptor(socketDescriptor))
		{
			rejected->abort();
		}
		else
		{
#ifdef Q_OS_WIN
			closesocket(static_cast<SOCKET>(socketDescriptor));
#else
			close(static_cast<int>(socketDescriptor));
#endif
		}
		rejected->deleteLater();
		return;
	}

	QTcpSocket *client = new QTcpSocket(this);
	if (!client->setSocketDescriptor(socketDescriptor))
	{
		delete client;
		return;
	}
	client->setReadBufferSize(MaxReadBufferSize);

	ClientInfo cs;
	cs.isAuthorized = !m_isAuthEnabled;
	cs.unknownCmdCount = 0;
	cs.failedAuthCount = 0;
	// Monotonically increasing session key prevents session-key reuse across connect/disconnect cycles
	cs.sessionKey = QStringLiteral("API%1%2").arg(
		lightpack ? lightpack->GetSessionKey(QStringLiteral("API")) : QStringLiteral("Lock"),
		QString::number(++m_nextClientId));

	m_clients.insert(client, cs);

	client->write(ApiVersion);

	DEBUG_LOW_LEVEL << "Incoming connection from:" << client->peerAddress().toString();

	connect(client, &QTcpSocket::readyRead, this, &ApiServer::clientProcessCommands);
	connect(client, &QTcpSocket::disconnected, this, &ApiServer::clientDisconnected);
}

void ApiServer::abortClient(QTcpSocket *client)
{
	if (!client || !m_clients.contains(client))
		return;

	QString sessionKey = m_clients[client].sessionKey;
	if (lightpack && lightpack->CheckLock(sessionKey) == 1)
		lightpack->UnLock(sessionKey);

	m_clients.remove(client);

	disconnect(client, &QTcpSocket::readyRead, this, &ApiServer::clientProcessCommands);
	disconnect(client, &QTcpSocket::disconnected, this, &ApiServer::clientDisconnected);

	client->abort();
	client->deleteLater();
}

bool ApiServer::isCrossProtocolLine(const QByteArray &line)
{
	static const char * const prefixes[] = {
		"GET ", "POST ", "PUT ", "DELETE ", "HEAD ", "OPTIONS ", "PATCH ", "CONNECT ", "TRACE ", "PRI * ",
		"HTTP/", "Host:", "Origin:", "Referer:", "User-Agent:", "Accept:", "Accept-",
		"Content-Type:", "Content-Length:", "Connection:", "Sec-", "Cookie:", "Upgrade:"
	};

	for (const char *prefix : prefixes)
	{
		const int len = static_cast<int>(qstrlen(prefix));
		if (line.size() >= len && qstrnicmp(line.constData(), prefix, static_cast<uint>(len)) == 0)
			return true;
	}
	if (line.contains(" HTTP/1.") || line.contains(" HTTP/2"))
		return true;
	return false;
}

bool ApiServer::constantTimeKeyEquals(const QByteArray &candidate, const QString &expected)
{
	const QByteArray expectedBytes = expected.toUtf8();
	const QByteArray candidateHash = QCryptographicHash::hash(candidate, QCryptographicHash::Sha256);
	const QByteArray expectedHash = QCryptographicHash::hash(expectedBytes, QCryptographicHash::Sha256);
	if (candidateHash.size() != expectedHash.size() || candidateHash.isEmpty())
		return false;

	volatile unsigned char diff = (candidate.size() == expectedBytes.size()) ? 0 : 1;
	for (int i = 0; i < candidateHash.size(); ++i)
	{
		diff |= static_cast<unsigned char>(candidateHash.at(i)) ^ static_cast<unsigned char>(expectedHash.at(i));
	}
	return diff == 0;
}

void ApiServer::clientDisconnected()
{
	QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
	if (!client || !m_clients.contains(client))
		return;

	DEBUG_LOW_LEVEL << "Client disconnected:" << client->peerAddress().toString();

	QString sessionKey = m_clients[client].sessionKey;
	if (lightpack && lightpack->CheckLock(sessionKey)==1)
		lightpack->UnLock(sessionKey);

	m_clients.remove(client);

	disconnect(client, &QTcpSocket::readyRead, this, &ApiServer::clientProcessCommands);
	disconnect(client, &QTcpSocket::disconnected, this, &ApiServer::clientDisconnected);

	client->deleteLater();
}

void ApiServer::clientProcessCommands()
{
	API_DEBUG_OUT << Q_FUNC_INFO << "ApiServer thread id:" << this->thread()->currentThreadId();

	QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
	if (!client || !m_clients.contains(client) || !lightpack)
		return;

	while (m_clients.contains(client) && client->canReadLine())
	{
		QByteArray rawLine = client->readLine(MaxCommandLength + 2);
		if (rawLine.size() > MaxCommandLength)
		{
			qWarning() << Q_FUNC_INFO << "Oversized API command line, disconnecting client";
			abortClient(client);
			return;
		}

		QString sessionKey = m_clients[client].sessionKey;
		int m_lockedClient = lightpack->CheckLock(sessionKey);

		QByteArray cmdBuffer = rawLine.trimmed();
		API_DEBUG_OUT << cmdBuffer;

		QString result = CmdUnknown;

		if (cmdBuffer.isEmpty())
		{
			// Ignore empty lines and continue draining buffered commands
			continue;
		}

		if (isCrossProtocolLine(cmdBuffer))
		{
			qWarning() << Q_FUNC_INFO << "Rejected cross-protocol HTTP/browser request on API socket";
			abortClient(client);
			return;
		}

		if (cmdBuffer == CmdExit)
		{
			writeData(client, QStringLiteral("Goodbye!\r\n"));
			if (m_clients.contains(client))
				client->close();
			return;
		}
		else if (cmdBuffer == CmdHelp)
		{
			m_clients[client].unknownCmdCount = 0;
			writeData(client, m_helpMessage);
			continue;
		}
		else if (cmdBuffer == CmdHelpShort)
		{
			m_clients[client].unknownCmdCount = 0;
			writeData(client, m_shortHelpMessage);
			continue;
		}
		else if (cmdBuffer.startsWith(CmdApiKey))
		{
			API_DEBUG_OUT << CmdApiKey;

			if (m_isAuthEnabled)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);

				if (constantTimeKeyEquals(cmdBuffer, m_apiAuthKey))
				{
					API_DEBUG_OUT << CmdApiKey << "OK";

					m_clients[client].isAuthorized = true;
					m_clients[client].failedAuthCount = 0;
					m_clients[client].unknownCmdCount = 0;

					result = CmdApiKeyResult_Ok;
				} else {
					API_DEBUG_OUT << CmdApiKey << "Api key is not valid";

					m_clients[client].isAuthorized = false;
					if (lightpack && lightpack->CheckLock(sessionKey) == 1)
						lightpack->UnLock(sessionKey);

					result = CmdApiKeyResult_Fail;
					if (++m_clients[client].failedAuthCount >= MaxFailedAuthAttempts)
					{
						writeData(client, result);
						client->flush();
						qWarning() << Q_FUNC_INFO << "Too many failed API key attempts, disconnecting client";
						abortClient(client);
						return;
					}
				}
			}
			else
			{
				API_DEBUG_OUT << CmdApiKey << "Authorization is disabled, return ok;";
				m_clients[client].unknownCmdCount = 0;
				result = CmdApiKeyResult_Ok;
			}

			writeData(client, result);
			continue;
		}

		if (m_isAuthEnabled && m_clients[client].isAuthorized == false)
		{
			writeData(client, CmdApiCheck_AuthRequired);
			continue;
		}

		// We are working only with authorized clients!

		if (cmdBuffer == CmdGetStatus)
		{
			API_DEBUG_OUT << CmdGetStatus;

			int status = lightpack->GetStatus();
			if (status== 1) result = CmdResultStatus_On;
			if (status== 0) result = CmdResultStatus_Off;
			if (status==-1) result = CmdResultStatus_DeviceError;
			if (status==-2) result = CmdResultStatus_Unknown;
		}
		else if (cmdBuffer == CmdGetStatusAPI)
		{
			API_DEBUG_OUT << CmdGetStatusAPI;

			if (lightpack->GetStatusAPI())
				result = CmdResultStatusAPI_Busy;
			else
				result = CmdResultStatusAPI_Idle;
		}
		else if (cmdBuffer == CmdGetProfiles)
		{
			API_DEBUG_OUT << CmdGetProfiles;

			QStringList profiles = lightpack->GetProfiles();

			result = ApiServer::CmdResultProfiles;

			for (int i = 0; i < profiles.count(); i++)
				result += QStringLiteral("%1;").arg(profiles[i]);
			result += QStringLiteral("\r\n");
		}
		else if (cmdBuffer == CmdGetProfile)
		{
			API_DEBUG_OUT << CmdGetProfile;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultProfile, lightpack->GetProfile());
		}
		else if (cmdBuffer == CmdGetDevices)
		{
			API_DEBUG_OUT << CmdGetDevices;

			QStringList devices = Settings::getSupportedDevices();

			result = ApiServer::CmdResultDevices;
			for (int i = 0; i < devices.count(); i++)
				result += QStringLiteral("%1;").arg(devices[i]);
			result += QStringLiteral("\r\n");
		}
		else if (cmdBuffer == CmdGetDevice)
		{
			API_DEBUG_OUT << CmdGetDevice;
			result = QStringLiteral("%1%2\r\n").arg(ApiServer::CmdResultDevice, Settings::getConnectedDeviceName());
		}
		else if (cmdBuffer == CmdGetMaxLeds)
		{
			API_DEBUG_OUT << CmdGetMaxLeds;

			int max = MaximumNumberOfLeds::AbsoluteMaximum;
			switch (Settings::getConnectedDevice())
			{
			case SupportedDevices::DeviceTypeAdalight:
				max = MaximumNumberOfLeds::Adalight;
				break;
			case SupportedDevices::DeviceTypeArdulight:
				max = MaximumNumberOfLeds::Ardulight;
				break;
			case SupportedDevices::DeviceTypeLightpack:
				max = MaximumNumberOfLeds::Lightpack5;
				break;
			case SupportedDevices::DeviceTypeVirtual:
				max = MaximumNumberOfLeds::Virtual;
				break;
			case SupportedDevices::DeviceTypeAlienFx:
				max = MaximumNumberOfLeds::AlienFx;
				break;
			case SupportedDevices::DeviceTypeDrgb:
				max = MaximumNumberOfLeds::Drgb;
				break;
			case SupportedDevices::DeviceTypeDnrgb:
				max = MaximumNumberOfLeds::Dnrgb;
				break;
			case SupportedDevices::DeviceTypeWarls:
				max = MaximumNumberOfLeds::Warls;
				break;
			default:
				max = MaximumNumberOfLeds::Default;
			}
			result = QStringLiteral("%1%2\r\n").arg(CmdResultMaxLeds, QString::number(max));
		}
		else if (cmdBuffer == CmdGetCountLeds)
		{
			API_DEBUG_OUT << CmdGetCountLeds;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultCountLeds, QString::number(lightpack->GetCountLeds()));
		}
		else if (cmdBuffer == CmdGetLeds)
		{
			API_DEBUG_OUT << CmdGetLeds;

			result = ApiServer::CmdResultLeds;

			for (int i = 0; i < Settings::getNumberOfLeds(Settings::getConnectedDevice()); i++)
			{
				QSize size = Settings::getLedSize(i);
				QPoint pos = Settings::getLedPosition(i);
				result += QStringLiteral("%1-%2,%3,%4,%5;").arg(i).arg(pos.x()).arg(pos.y()).arg(size.width()).arg(size.height());
			}
			result += QStringLiteral("\r\n");

		}
		else if (cmdBuffer == CmdGetColors)
		{
			API_DEBUG_OUT << CmdGetColors;
			result = ApiServer::CmdResultGetColors;

			QList<QRgb> curentColors = lightpack->GetColors();

			for (int i = 0; i < curentColors.count(); i++)
			{
				QColor color(curentColors[i]);
				result += QStringLiteral("%1-%2,%3,%4;").arg(i).arg(color.red()).arg(color.green()).arg(color.blue());
			}
			result += QStringLiteral("\r\n");
		}
		else if (cmdBuffer == CmdGetFPS)
		{
			API_DEBUG_OUT << CmdGetFPS;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultFPS).arg(lightpack->GetFPS());
		}
		else if (cmdBuffer == CmdGetScreenSize)
		{
			API_DEBUG_OUT << CmdGetScreenSize;

			QRect screen = lightpack->GetScreenSize();

			result = QStringLiteral("%1%2,%3,%4,%5\r\n").arg(CmdResultScreenSize).arg(screen.x()).arg(screen.y()).arg(screen.width()).arg(screen.height());
		}
		else if (cmdBuffer == CmdGetCountMonitor)
		{
			API_DEBUG_OUT << CmdGetCountMonitor;

			int count = QGuiApplication::screens().count();

			result = QStringLiteral("%1%2\r\n").arg(CmdResultCountMonitor).arg(count);
		}
		else if (cmdBuffer.startsWith(CmdGetSizeMonitor))
		{
			API_DEBUG_OUT << CmdGetSizeMonitor;

				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				bool ok = false;
				int monitor = QString(cmdBuffer).toInt(&ok);

				if (ok)
				{
					API_DEBUG_OUT << CmdGetSizeMonitor << "OK:" << monitor;

					QScreen* screen = QGuiApplication::screens().value(monitor, nullptr);

					if (screen)
					{
						QRect geom = screen->geometry();

						result = QStringLiteral("%1%2,%3,%4,%5\r\n").arg(CmdResultSizeMonitor).arg(geom.x()).arg(geom.y()).arg(geom.width()).arg(geom.height());
					}
					else
					{
						API_DEBUG_OUT << CmdGetSizeMonitor << "Error (unknow monitor):" << monitor;
						result = CmdSetResult_Error;
					}

				} else {
						API_DEBUG_OUT << CmdGetSizeMonitor << "Error (convert fail):" << monitor;
						result = CmdSetResult_Error;
				}
		}
		else if (cmdBuffer == CmdGetBacklight)
		{
			API_DEBUG_OUT << CmdGetBacklight;

			Lightpack::Mode mode =	Settings::getLightpackMode();

			switch (mode)
			{
			case Lightpack::AmbilightMode:
				result = CmdResultBacklight_Ambilight;
				break;
			case Lightpack::MoodLampMode:
				result = CmdResultBacklight_Moodlamp;
				break;
#ifdef SOUNDVIZ_SUPPORT
			case Lightpack::SoundVisualizeMode:
				result = CmdResultBacklight_SoundViz;
				break;
#endif
			default:
				result = CmdSetResult_Error;
				break;
			}
		}
		else if (cmdBuffer == CmdGetGamma)
		{
			API_DEBUG_OUT << CmdGetGamma;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultGamma).arg(lightpack->GetGamma());
		}
		else if (cmdBuffer == CmdGetBrightness)
		{
			API_DEBUG_OUT << CmdGetBrightness;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultBrightness).arg(lightpack->GetBrightness());
		}
		else if (cmdBuffer == CmdGetSmooth)
		{
			API_DEBUG_OUT << CmdGetSmooth;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultSmooth).arg(lightpack->GetSmooth());
		}
#ifdef SOUNDVIZ_SUPPORT
		else if (cmdBuffer == CmdGetSoundVizColors)
		{
			API_DEBUG_OUT << CmdGetSoundVizColors;

			QPair<QColor, QColor> colors = lightpack->GetSoundVizColors();
			result = QStringLiteral("%1%2,%3,%4;%5,%6,%7\r\n").arg(CmdResultSoundVizColors)
				.arg(colors.first.red()).arg(colors.first.green()).arg(colors.first.blue())
				.arg(colors.second.red()).arg(colors.second.green()).arg(colors.second.blue());
		}
		else if (cmdBuffer == CmdGetSoundVizLiquid)
		{
			API_DEBUG_OUT << CmdGetSoundVizColors;

			result = QStringLiteral("%1%2\r\n").arg(CmdResultSoundVizLiquid).arg(lightpack->GetSoundVizLiquidMode() ? 1 : 0);
		}
#endif
		else if (cmdBuffer == CmdGetPersistOnUnlock)
		{
			API_DEBUG_OUT << CmdGetPersistOnUnlock;

			result = lightpack->GetPersistOnUnlock() ? CmdGetPersistOnUnlock_On : CmdGetPersistOnUnlock_Off;
		}
		else if (cmdBuffer.startsWith(CmdGuid))
		{
			API_DEBUG_OUT << CmdGuid;

			cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
			API_DEBUG_OUT << QString(cmdBuffer);

			QString guid = cmdBuffer;
			if (lightpack->VerifySessionKey(guid))
			{
				m_clients[client].sessionKey = guid;
				result = CmdSetResult_Ok;
			}
			else
			{
				result = CmdSetResult_Error;
			}
		}
		else if (cmdBuffer == CmdLockStatus)
		{
			API_DEBUG_OUT << CmdLockStatus;

			int res = lightpack->CheckLock(sessionKey);
			QString status = QStringLiteral("no");
			if (res == -1)
				status = QStringLiteral("busy");
			if (res == 1)
				status = QStringLiteral("ok");

			result = QStringLiteral("lockstatus:%1\r\n").arg(status);
		}
		else if (cmdBuffer == CmdLock)
		{
			API_DEBUG_OUT << CmdLock;

			bool res = lightpack->Lock(sessionKey);

			if (res)
			{
				m_clients[client].isAuthorized = true;
				result = CmdResultLock_Success;
			} else {
					result = CmdResultLock_Busy;
			}
		}
		else if (cmdBuffer == CmdUnlock)
		{
			API_DEBUG_OUT << CmdUnlock;

			bool res = lightpack->UnLock(sessionKey);
			if (!res)
			{
				result = CmdResultUnlock_NotLocked;
			} else {
				result = CmdResultUnlock_Success;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetColor))
		{
			API_DEBUG_OUT << CmdSetColor;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				if (m_isTaskSetColorDone)
				{
					m_isTaskSetColorDone = false;
					m_isTaskSetColorParseSuccess = false;

					// Start task
					emit startParseSetColorTask(cmdBuffer);

					// Wait signal from m_apiSetColorTask with success or fail result of parsing buffer.
					// After SignalWaitTimeoutMs milliseconds, the cycle of waiting will end and the
					// variable m_isTaskSetColorDone will be reset to 'true' state.
					// Also in cycle we process requests from over clients, if this request is setcolor when
					// it will be ignored because of m_isTaskSetColorDone == false

					m_timer.restart();

					while (m_isTaskSetColorDone == false && m_timer.hasExpired(SignalWaitTimeoutMs) == false)
					{
						QApplication::processEvents(
							QEventLoop::ExcludeSocketNotifiers | QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents,
							SignalWaitTimeoutMs);
					}

					if (m_isTaskSetColorDone)
					{
						if (m_isTaskSetColorParseSuccess)
						{
							lightpack->SetLockAlive(sessionKey);
							result = CmdSetResult_Ok;
						}
						else
							result = CmdSetResult_Error;
					} else {
						m_isTaskSetColorDone = true; // cmd setcolor is available
						result = CmdSetResult_Error;
						qWarning() << Q_FUNC_INFO << "Timeout waiting taskIsSuccess() signal from m_apiSetColorTask";
					}
				} else {
					result = CmdSetResult_Busy;
					qWarning() << Q_FUNC_INFO << "Task setcolor is not completed (you should increase the delay to not skip commands), skip setcolor.";
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetGamma))
		{
			API_DEBUG_OUT << CmdSetGamma;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				// Gamma can contain max five chars (0.00 -- 10.00)
				if (cmdBuffer.length() > 5)
				{
					API_DEBUG_OUT << CmdSetGamma << "Error (gamma max 5 chars)";
					result = CmdSetResult_Error;
				} else {
					// Try to convert gamma string to double
					bool ok = false;
					double gamma = QString(cmdBuffer).toDouble(&ok);

					if (ok)
					{
						API_DEBUG_OUT << CmdSetGamma << "OK:" << gamma;
						if (lightpack->SetGamma(sessionKey,gamma))
						{
							result = CmdSetResult_Ok;
						} else {
							API_DEBUG_OUT << CmdSetGamma << "Error (max min test fail):" << gamma;
							result = CmdSetResult_Error;
						}
					} else {
						API_DEBUG_OUT << CmdSetGamma << "Error (convert fail):" << gamma;
						result = CmdSetResult_Error;
					}
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetBrightness))
		{
			API_DEBUG_OUT << CmdSetBrightness;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				// Brightness can contain max three chars (0 -- 100)
				if (cmdBuffer.length() > 3)
				{
					API_DEBUG_OUT << CmdSetBrightness << "Error (smooth max 3 chars)";
					result = CmdSetResult_Error;
				} else {
					// Try to convert smooth string to int
					bool ok = false;
					int brightness = QString(cmdBuffer).toInt(&ok);

					if (ok)
					{
						if (lightpack->SetBrightness(sessionKey,brightness))
						{
							API_DEBUG_OUT << CmdSetBrightness << "OK:" << brightness;
							result = CmdSetResult_Ok;
						} else {
							API_DEBUG_OUT << CmdSetBrightness << "Error (max min test fail):" << brightness;
							result = CmdSetResult_Error;
						}
					} else {
						API_DEBUG_OUT << CmdSetBrightness << "Error (convert fail):" << brightness;
						result = CmdSetResult_Error;
					}
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetSmooth))
		{
			API_DEBUG_OUT << CmdSetSmooth;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				// Smooth can contain max three chars (0 -- 255)
				if (cmdBuffer.length() > 3)
				{
					API_DEBUG_OUT << CmdSetSmooth << "Error (smooth max 3 chars)";
					result = CmdSetResult_Error;
				} else {
					// Try to convert smooth string to int
					bool ok = false;
					int smooth = QString(cmdBuffer).toInt(&ok);

					if (ok)
					{
						if (lightpack->SetSmooth(sessionKey,smooth))
						{
							API_DEBUG_OUT << CmdSetSmooth << "OK:" << smooth;
							result = CmdSetResult_Ok;
						} else {
							API_DEBUG_OUT << CmdSetSmooth << "Error (max min test fail):" << smooth;
							result = CmdSetResult_Error;
						}
					} else {
						API_DEBUG_OUT << CmdSetSmooth << "Error (convert fail):" << smooth;
						result = CmdSetResult_Error;
					}
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
#ifdef SOUNDVIZ_SUPPORT
		else if (cmdBuffer.startsWith(CmdSetSoundVizColors))
		{
			API_DEBUG_OUT << CmdSetSoundVizColors;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);

				QString s = QString(cmdBuffer);
				API_DEBUG_OUT << s;

				QStringList colors = s.split(';');
				if (colors.size() == 2) {
					QStringList colorMin = colors.at(0).split(',');
					QStringList colorMax = colors.at(1).split(',');

					if (colorMin.size() == 3 && colorMax.size() == 3) {
						bool ok;
						int r, g, b;
						r = colorMin.at(0).toInt(&ok);
						if (!ok || r < 0 || r > 255) goto parsefail;
						g = colorMin.at(1).toInt(&ok);
						if (!ok || g < 0 || g > 255) goto parsefail;
						b = colorMin.at(2).toInt(&ok);
						if (!ok || b < 0 || b > 255) goto parsefail;
						QColor min = QColor(r, g, b);

						r = colorMax.at(0).toInt(&ok);
						if (!ok || r < 0 || r > 255) goto parsefail;
						g = colorMax.at(1).toInt(&ok);
						if (!ok || g < 0 || g > 255) goto parsefail;
						b = colorMax.at(2).toInt(&ok);
						if (!ok || b < 0 || b > 255) goto parsefail;
						QColor max = QColor(r, g, b);


						if (lightpack->SetSoundVizColors(sessionKey, min, max))
						{
							API_DEBUG_OUT << CmdSetSmooth << "OK:" << min << max;
							result = CmdSetResult_Ok;
						} else {
							API_DEBUG_OUT << CmdSetSmooth << "Error:" << min << max;
							result = CmdSetResult_Error;
						}
					} else {
					parsefail:
						API_DEBUG_OUT << CmdSetSmooth << "Error (color parse fail)";
						result = CmdSetResult_Error;
					}
				} else {
					API_DEBUG_OUT << CmdSetSmooth << "Error (need two colors)";
					result = CmdSetResult_Error;
				}

			} else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			} else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		} else if (cmdBuffer.startsWith(CmdSetSoundVizLiquid))
		{
			API_DEBUG_OUT << CmdSetSoundVizLiquid;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);

				QString s = QString(cmdBuffer);
				API_DEBUG_OUT << s;

				bool ok;
				int value = s.toInt(&ok);

				if (ok) {
					if (lightpack->SetSoundVizLiquidMode(sessionKey, value))
					{
						API_DEBUG_OUT << CmdSetSmooth << "OK:" << value;
						result = CmdSetResult_Ok;
					} else {
						API_DEBUG_OUT << CmdSetSmooth << "Error:" << value;
						result = CmdSetResult_Error;
					}
				} else {
					API_DEBUG_OUT << CmdSetSmooth << "Error (convert fail)";
					result = CmdSetResult_Error;
				}

			} else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			} else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
#endif
		else if (cmdBuffer.startsWith(CmdSetPersistOnUnlock))
		{
			API_DEBUG_OUT << CmdSetPersistOnUnlock;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				int status = -1;

				if (cmdBuffer == CmdSetPersistOnUnlock_On)
					status = 1;
				else if (cmdBuffer == CmdSetPersistOnUnlock_Off)
					status = 0;

				if (status != -1)
				{
					API_DEBUG_OUT << CmdSetPersistOnUnlock << "OK:" << status;

					lightpack->SetPersistOnUnlock(sessionKey, status);

					result = CmdSetResult_Ok;
				} else {
					API_DEBUG_OUT << CmdSetPersistOnUnlock << "Error (status not recognized):" << status;
					result = CmdSetResult_Error;
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetProfile))
		{
			API_DEBUG_OUT << CmdSetProfile;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);
				QString setProfileName = QString(cmdBuffer);
				if (lightpack->SetProfile(sessionKey, setProfileName))
				{
					API_DEBUG_OUT << CmdSetProfile << "OK:" << setProfileName;
					result = CmdSetResult_Ok;
				} else {
					API_DEBUG_OUT << CmdSetProfile << "Error (profile not found):" << setProfileName;
					result = CmdSetResult_Error;
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetDevice))
		{
			API_DEBUG_OUT << CmdSetDevice;

			API_DEBUG_OUT << CmdSetDevice << "Error (operation unsupported/deprecated)";
			result = CmdDeprecated;
		}
		else if (cmdBuffer.startsWith(CmdSetCountLeds))
		{
			API_DEBUG_OUT << CmdSetCountLeds;

			API_DEBUG_OUT << CmdSetDevice << "Error (operation unsupported/deprecated)";
			result = CmdDeprecated;
		}
		else if (cmdBuffer.startsWith(CmdSetLeds))
		{
			API_DEBUG_OUT << CmdSetLeds;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);
				int countleds = lightpack->GetCountLeds();
				QList<QRect> rectLeds;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
				QStringList leds = QString::fromUtf8(cmdBuffer).split(QLatin1Char(';'), Qt::SkipEmptyParts);
#else
				QStringList leds = QString::fromUtf8(cmdBuffer).split(QLatin1Char(';'), QString::SkipEmptyParts);
#endif
				bool parseOk = (countleds > 0 && leds.size() <= countleds);
				for (int i = 0; parseOk && i < leds.size(); ++i)
				{
					const QString &led = leds.at(i);
					const int dashIdx = led.indexOf(QLatin1Char('-'));
					if (dashIdx <= 0)
					{
						parseOk = false;
						break;
					}
					bool okNum = false, okX = false, okY = false, okW = false, okH = false;
					const int num = led.left(dashIdx).toInt(&okNum);
					const QStringList xywh = led.mid(dashIdx + 1).split(QLatin1Char(','));
					if (!okNum || num <= 0 || num > countleds || xywh.count() != 4)
					{
						parseOk = false;
						break;
					}
					const int x = xywh[0].toInt(&okX);
					const int y = xywh[1].toInt(&okY);
					const int w = xywh[2].toInt(&okW);
					const int h = xywh[3].toInt(&okH);
					if (!okX || !okY || !okW || !okH || w < 0 || h < 0)
					{
						parseOk = false;
						break;
					}
					rectLeds << QRect(x, y, w, h);
				}
				if (parseOk && !rectLeds.isEmpty())
				{
					lightpack->SetLeds(sessionKey, rectLeds);
					result = CmdSetResult_Ok;
				}
				else
				{
					result = CmdSetResult_Error;
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdNewProfile))
		{
			API_DEBUG_OUT << CmdNewProfile;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);
				QString newProfileName = QString(cmdBuffer);
				if (lightpack->NewProfile(sessionKey, newProfileName))
				{
						API_DEBUG_OUT << CmdNewProfile << "OK:" << newProfileName;
					result = CmdSetResult_Ok;
				}
				else
					result = CmdSetResult_Error;
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdDeleteProfile))
		{
			API_DEBUG_OUT << CmdDeleteProfile;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				QString deleteProfileName = QString(cmdBuffer);

				if (lightpack->DeleteProfile(sessionKey, deleteProfileName))
				{
					API_DEBUG_OUT << CmdDeleteProfile << "OK:" << deleteProfileName;
					result = CmdSetResult_Ok;
				} else {
					API_DEBUG_OUT << CmdDeleteProfile << "Error (profile not found):" << deleteProfileName;
					result = CmdSetResult_Error;
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetStatus))
		{
			API_DEBUG_OUT << CmdSetStatus;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				int status = -1;

				if (cmdBuffer == CmdSetStatus_On)
					status = 1;
				else if (cmdBuffer == CmdSetStatus_Off)
					status = 0;

				if (status != -1)
				{
					API_DEBUG_OUT << CmdSetStatus << "OK:" << status;

					lightpack->SetStatus(sessionKey,status);

					result = CmdSetResult_Ok;
				} else {
					API_DEBUG_OUT << CmdSetStatus << "Error (status not recognized):" << status;
					result = CmdSetResult_Error;
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else if (cmdBuffer.startsWith(CmdSetBacklight))
		{
			API_DEBUG_OUT << CmdSetBacklight;

			if (m_lockedClient == 1)
			{
				cmdBuffer.remove(0, cmdBuffer.indexOf(':') + 1);
				API_DEBUG_OUT << QString(cmdBuffer);

				int status =	0;

				if (cmdBuffer == CmdSetBacklight_Ambilight)
					status = 1;
				else if (cmdBuffer == CmdSetBacklight_Moodlamp)
					status = 2;
#ifdef SOUNDVIZ_SUPPORT
				else if (cmdBuffer == CmdSetBacklight_SoundViz)
					status = 3;
#endif

				if (status != 0)
				{
					API_DEBUG_OUT << CmdSetBacklight << "OK:" << status;
					lightpack->SetBacklight(sessionKey,status);
					result = CmdSetResult_Ok;
				} else {
					API_DEBUG_OUT << CmdSetBacklight << "Error (status not recognized):" << status;
					result = CmdSetResult_Error;
				}
			}
			else if (m_lockedClient == 0)
			{
				result = CmdSetResult_NotLocked;
			}
			else // m_lockedClient != client
			{
				result = CmdSetResult_Busy;
			}
		}
		else
		{
			qWarning() << Q_FUNC_INFO << CmdUnknown << cmdBuffer;
		}

		if (m_clients.contains(client))
		{
			if (result == QLatin1String(CmdUnknown))
			{
				if (++m_clients[client].unknownCmdCount >= MaxConsecutiveUnknownCommands)
				{
					writeData(client, result);
					qWarning() << Q_FUNC_INFO << "Too many consecutive unknown commands, disconnecting client";
					abortClient(client);
					return;
				}
			}
			else
			{
				m_clients[client].unknownCmdCount = 0;
			}
		}

		writeData(client, result);
	}

	if (m_clients.contains(client) && !client->canReadLine() && client->bytesAvailable() > MaxCommandLength)
	{
		qWarning() << Q_FUNC_INFO << "Oversized unterminated API command line, disconnecting client";
		abortClient(client);
	}
}

void ApiServer::taskSetColorIsSuccess(bool isSuccess)
{
	m_isTaskSetColorDone = true;
	m_isTaskSetColorParseSuccess = isSuccess;
}

void ApiServer::initPrivateVariables()
{
	m_apiPort = Settings::getApiPort();
	m_listenOnlyOnLoInterface = Settings::isListenOnlyOnLoInterface();
	m_apiAuthKey = Settings::getApiAuthKey();
	m_isAuthEnabled = !m_apiAuthKey.isEmpty();
}

void ApiServer::initApiSetColorTask()
{
	m_isTaskSetColorDone = true;

	m_apiSetColorTaskThread = new QThread();
	m_apiSetColorTask = new ApiServerSetColorTask();
	m_apiSetColorTask->setApiDeviceNumberOfLeds(Settings::getNumberOfLeds(Settings::getConnectedDevice()));

	//connect(m_apiSetColorTask, &ApiServerSetColorTask::taskParseSetColorDone(QList<QRgb>)), this, &ApiServer::updateLedsColors(QList<QRgb>)), Qt::QueuedConnection);
	connect(m_apiSetColorTask, &ApiServerSetColorTask::taskParseSetColorIsSuccess, this, &ApiServer::taskSetColorIsSuccess, Qt::QueuedConnection);

	connect(this, &ApiServer::startParseSetColorTask, m_apiSetColorTask, &ApiServerSetColorTask::startParseSetColorTask, Qt::QueuedConnection);
	connect(this, &ApiServer::updateApiDeviceNumberOfLeds,	m_apiSetColorTask, &ApiServerSetColorTask::setApiDeviceNumberOfLeds, Qt::QueuedConnection);
	connect(this, &ApiServer::clearColorBuffers,				m_apiSetColorTask, &ApiServerSetColorTask::reinitColorBuffers);


	m_apiSetColorTask->moveToThread(m_apiSetColorTaskThread);
	m_apiSetColorTaskThread->start();
}

void ApiServer::startListening()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << m_apiPort;

	QHostAddress address = m_listenOnlyOnLoInterface ? QHostAddress::LocalHost : QHostAddress::Any;
	bool ok = listen(address, m_apiPort);

	if (ok == false)
	{
		QString errorStr = tr("API server unable to start (port: %1): %2.")
				.arg(m_apiPort).arg(errorString());

		qCritical() << Q_FUNC_INFO << errorStr;

		emit errorOnStartListening(errorStr);
	}
}

void ApiServer::stopListening()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	// Closes the server. The server will no longer listen for incoming connections.
	close();

	QMap<QTcpSocket*, ClientInfo>::iterator i;
	for (i = m_clients.begin(); i != m_clients.end(); ++i)
	{

		QTcpSocket * client = i.key();

		QString sessionKey = m_clients[client].sessionKey;
		if (lightpack->CheckLock(sessionKey)==1)
			lightpack->UnLock(sessionKey);

		disconnect(client, &QTcpSocket::readyRead, this, &ApiServer::clientProcessCommands);
		disconnect(client, &QTcpSocket::disconnected, this, &ApiServer::clientDisconnected);

		client->abort();
		client->deleteLater();
	}

	m_clients.clear();
}

void ApiServer::writeData(QTcpSocket* client, const QString & data)
{
	if (m_clients.contains(client) == false)
	{
		API_DEBUG_OUT << Q_FUNC_INFO << "client disconected, cancel writing buffer = " << data;
		return;
	}

	API_DEBUG_OUT << Q_FUNC_INFO << data;
	client->write(data.toUtf8());
}

QString ApiServer::formatHelp(const QString & cmd)
{
	return QStringLiteral("\t\t \"%1\" \r\n").arg(cmd.trimmed());
}

QString ApiServer::formatHelp(const QString & cmd, const QString & description)
{
	return QString(
				QString(80,'.') + "\r\n"
				"%1 \r\n"				// Command
				"\t %2 \r\n"			// Description
				).arg(cmd, description);
}

QString ApiServer::formatHelp(const QString & cmd, const QString & description, const QString & results)
{
	return QString(
				formatHelp(cmd, description) +
				"\t Results: \r\n"	// Return values
				"%1"
				).arg(results);
}

QString ApiServer::formatHelp(const QString & cmd, const QString & description, const QString & examples, const QString & results)
{
	return QString(
				formatHelp(cmd, description) +
				"\t Examples: \r\n"
				"%1"
				"\t Results: \r\n"	// Return values
				"%2"
				).arg(examples, results);
}

void ApiServer::initHelpMessage()
{
	m_helpMessage += QStringLiteral("\r\n");
	m_helpMessage += QStringLiteral("Prismatik " VERSION_STR ", API " API_VERSION "\r\n");
	m_helpMessage += QStringLiteral("Prismatik API is a fork of the original API\r\n");
	m_helpMessage += QStringLiteral("It is backwards compatible to Lightpack API 1.4\r\n");
	m_helpMessage += QStringLiteral("\r\n");

	m_helpMessage += formatHelp(
				CmdApiKey,
				QStringLiteral("Command for enter an authorization key (see key in GUI)"),
				formatHelp(CmdApiKey + QStringLiteral("{1ccf5dca-119d-45a0-a683-7d90a00c418f}")) +
				formatHelp(CmdApiKey + QStringLiteral("IDDQD")),
				formatHelp(CmdApiKeyResult_Ok) +
				formatHelp(CmdApiKeyResult_Fail)
				);

	m_helpMessage += formatHelp(
				CmdLock,
				QStringLiteral("Opens access to set-commands, If success - suspends capture and blocking access for other clients to set-commands."),
				formatHelp(CmdResultLock_Success) +
				formatHelp(CmdResultLock_Busy)
				);

	m_helpMessage += formatHelp(
				CmdUnlock,
				QStringLiteral("Closes access to set-commands. Restores device settings from the current profile, and continues the normal execution of the application."),
				formatHelp(CmdResultUnlock_Success) +
				formatHelp(CmdResultUnlock_NotLocked)
				);

	// Get-commands
	m_helpMessage += formatHelp(
				CmdGetStatus,
				QStringLiteral("Get status of the backlight"),
				formatHelp(CmdResultStatus_On) +
				formatHelp(CmdResultStatus_Off) +
				formatHelp(CmdResultStatus_DeviceError) +
				formatHelp(CmdResultStatus_Unknown)
				);
	m_helpMessage += formatHelp(
				CmdGetStatusAPI,
				QStringLiteral("Get status of the lightpack API"),
				formatHelp(CmdResultStatusAPI_Busy) +
				formatHelp(CmdResultStatusAPI_Idle)
				);
	m_helpMessage += formatHelp(
				CmdGetProfile,
				QStringLiteral("Get the name of the current profile"),
				formatHelp(CmdResultProfile + QStringLiteral("SampleProfileName"))
				);
	m_helpMessage += formatHelp(
				CmdGetProfiles,
				QStringLiteral("Get names of the all available profiles"),
				formatHelp(CmdResultProfiles + QStringLiteral("Lightpack;New profile 1;New profile 2;"))
				);
	m_helpMessage += formatHelp(
				CmdGetCountLeds,
				QStringLiteral("Get count leds of the current profile"),
				formatHelp(CmdResultCountLeds + QStringLiteral("10"))
				);
	m_helpMessage += formatHelp(
				CmdGetLeds,
				QStringLiteral("Get curent rect areas. Format: \"N-X,Y,W,H;\", where N - number of area, X,Y - position, H,W-size."),
				formatHelp(CmdResultLeds + QStringLiteral("1-0,0,100,100;2-0,200,100,100;"))
				);
	m_helpMessage += formatHelp(
				CmdGetColors,
				QStringLiteral("Get curent color leds. Format: \"N-R,G,B;\", where N - number of led, R, G, B - red, green and blue color components."),
				formatHelp(CmdResultGetColors + QStringLiteral("1-0,120,200;2-0,234,23;"))
				);
	m_helpMessage += formatHelp(
				CmdGetFPS,
				QStringLiteral("Get FPS grabing"),
				formatHelp(CmdResultFPS + QStringLiteral("25.57"))
				);
	m_helpMessage += formatHelp(
				CmdGetScreenSize,
				QStringLiteral("Get size screen"),
				formatHelp(CmdResultScreenSize + QStringLiteral("0,0,1024,768"))
				);
	m_helpMessage += formatHelp(
				CmdGetBacklight,
				QStringLiteral("Get mode of the current profile"),
				formatHelp(CmdResultBacklight_Ambilight) +
				formatHelp(CmdResultBacklight_Moodlamp)
#ifdef SOUNDVIZ_SUPPORT
				+ formatHelp(CmdResultBacklight_Moodlamp)
#endif
				);
	m_helpMessage += formatHelp(
				CmdGetGamma,
				QStringLiteral("Get the current gamma correction value"),
				formatHelp(CmdResultGamma + QStringLiteral("2.004"))
				);
	m_helpMessage += formatHelp(
				CmdGetBrightness,
				QStringLiteral("Get the current brightness value"),
				formatHelp(CmdResultBrightness + QStringLiteral("100"))
				);
	m_helpMessage += formatHelp(
				CmdGetSmooth,
				QStringLiteral("Get the current smooth value"),
				formatHelp(CmdResultSmooth + QStringLiteral("1"))
				);
#ifdef SOUNDVIZ_SUPPORT
	m_helpMessage += formatHelp(
		CmdGetSoundVizColors,
		QStringLiteral("Get min and max color for sound visualization. Format: \"R,G,B;R,G,B\". Since API 2.1"),
		formatHelp(CmdResultSoundVizColors + QStringLiteral("0,0,0;255,255,255"))
		);
	m_helpMessage += formatHelp(
		CmdGetSoundVizLiquid,
		QStringLiteral("Get wether or not sound visualization is in liquid color mode. Since API 2.1"),
		formatHelp(CmdResultSoundVizLiquid + QStringLiteral("1"))
		);
#endif
	m_helpMessage += formatHelp(
		CmdGetPersistOnUnlock,
		QStringLiteral("Get wether or not the last set colors should persist when unlocking"),
		formatHelp(CmdGetPersistOnUnlock_On)
		);

	// Set-commands

	QString helpCmdSetResults =
			formatHelp(CmdSetResult_Ok) +
			formatHelp(CmdSetResult_Error) +
			formatHelp(CmdSetResult_Busy) +
			formatHelp(CmdSetResult_NotLocked);

	m_helpMessage += formatHelp(
				CmdSetColor,
				QStringLiteral("Set colors on several LEDs. Format: \"N-R,G,B;\", where N - number of led, R, G, B - red, green and blue color components. Works only on locking time (see lock)."),
				formatHelp(CmdSetColor + QStringLiteral("1-255,255,30;")) +
				formatHelp(CmdSetColor + QStringLiteral("1-255,255,30;2-12,12,12;3-1,2,3;")),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetLeds,
				QStringLiteral("Set areas on several LEDs. Format: \"N-X,Y,W,H;\", where N - number of led, X,Y - position, H,W-size. Works only on locking time (see lock)."),
				formatHelp(CmdSetLeds + QStringLiteral("1-0,0,100,100;")) +
				formatHelp(CmdSetLeds + QStringLiteral("1-0,0,100,100;2-0,100,100,100;3-100,0,100,100;")),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetGamma,
				QStringLiteral("Set device gamma correction value [%1 - %2]. Works only on locking time (see lock).")
				.arg(SettingsScope::Profile::Device::GammaMin, SettingsScope::Profile::Device::GammaMax),
				formatHelp(CmdSetGamma + QStringLiteral("2.5")),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetBrightness,
				QStringLiteral("Set device brightness value [%1 - %2]. Works only on locking time (see lock).")
				.arg(SettingsScope::Profile::Device::BrightnessMin, SettingsScope::Profile::Device::BrightnessMax),
				formatHelp(CmdSetBrightness + QStringLiteral("0")) +
				formatHelp(CmdSetBrightness + QStringLiteral("93")),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetSmooth,
				QStringLiteral("Set device smooth value [%1 - %2]. Works only on locking time (see lock).")
				.arg(SettingsScope::Profile::Device::SmoothMin, SettingsScope::Profile::Device::SmoothMax),
				formatHelp(CmdSetSmooth + QStringLiteral("10")) +
				formatHelp(CmdSetSmooth + QStringLiteral("128")),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetProfile,
				QStringLiteral("Set current profile. Works only on locking time (see lock)."),
				formatHelp(CmdSetProfile + QStringLiteral("Lightpack")) +
				formatHelp(CmdSetProfile + QStringLiteral("16x9")),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdNewProfile,
				QStringLiteral("Create new profile. Works only on locking time (see lock)."),
				formatHelp(CmdNewProfile + QStringLiteral("16x9")) +
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdDeleteProfile,
				QStringLiteral("Delete profile. Works only on locking time (see lock)."),
				formatHelp(CmdDeleteProfile + QStringLiteral("16x9")) +
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetStatus,
				QStringLiteral("Set backlight status. Works only on locking time (see lock)."),
				formatHelp(CmdSetStatus + QString(CmdSetStatus_On)) +
				formatHelp(CmdSetStatus + QString(CmdSetStatus_Off)),
				helpCmdSetResults);

	m_helpMessage += formatHelp(
				CmdSetBacklight,
				QStringLiteral("Set backlight mode. Works only on locking time (see lock)."),
				formatHelp(CmdSetBacklight + QString(CmdSetBacklight_Ambilight)) +
				formatHelp(CmdSetBacklight + QString(CmdSetBacklight_Moodlamp))
#ifdef SOUNDVIZ_SUPPORT
				+ formatHelp(CmdSetBacklight + QString(CmdSetBacklight_SoundViz))
#endif
				,
				helpCmdSetResults);

#ifdef SOUNDVIZ_SUPPORT
	m_helpMessage += formatHelp(
		CmdSetSoundVizColors,
		QStringLiteral("Set min and max color for sound visualization. Format: \"R,G,B;R,G,B\". Since API 2.1"),
		formatHelp(CmdSetSoundVizColors + QStringLiteral("0,0,0;255,255,255")),
		helpCmdSetResults);
	m_helpMessage += formatHelp(
		CmdSetSoundVizLiquid,
		QStringLiteral("Set wether or not sound visualization is in liquid color mode. Since API 2.1"),
		formatHelp(CmdSetSoundVizLiquid + QStringLiteral("0")),
		helpCmdSetResults);
#endif
	m_helpMessage += formatHelp(
		CmdSetPersistOnUnlock,
		QStringLiteral("Set wether or not the last set colors should persist when unlocking. Since API 2.2"),
		formatHelp(CmdSetPersistOnUnlock + QString(CmdSetPersistOnUnlock_On)),
		helpCmdSetResults);


	m_helpMessage += formatHelp(CmdHelpShort, QStringLiteral("Short version of this help"));

	m_helpMessage += formatHelp(CmdExit, QStringLiteral("Closes connection"));

	m_helpMessage += QStringLiteral("\r\n");
}

void ApiServer::initShortHelpMessage()
{
	m_shortHelpMessage += QStringLiteral("\r\n");
	m_shortHelpMessage += QStringLiteral("List of available commands:\r\n");

	QList<QString> cmds;
	cmds << CmdApiKey << CmdLock << CmdUnlock
			<< CmdGetStatus << CmdGetStatusAPI
			<< CmdGetProfile << CmdGetProfiles
			<< CmdGetCountLeds << CmdGetLeds << CmdGetColors
			<< CmdGetFPS << CmdGetScreenSize << CmdGetBacklight
			<< CmdGetGamma << CmdGetBrightness << CmdGetSmooth
#ifdef SOUNDVIZ_SUPPORT
			<< CmdGetSoundVizColors << CmdGetSoundVizLiquid
#endif
			<< CmdGetPersistOnUnlock
			<< CmdSetColor << CmdSetLeds
			<< CmdSetGamma << CmdSetBrightness << CmdSetSmooth
			<< CmdSetProfile << CmdNewProfile << CmdDeleteProfile
			<< CmdSetStatus << CmdSetBacklight
#ifdef SOUNDVIZ_SUPPORT
			<< CmdSetSoundVizColors << CmdSetSoundVizLiquid
#endif
			<< CmdSetPersistOnUnlock
			<< CmdExit << CmdHelp << CmdHelpShort;

	QString line = QStringLiteral("	");

	for (int i = 0; i < cmds.count(); i++)
	{
		if (line.length() + cmds[i].length() < 80)
		{
			line += cmds[i].remove(':');
		} else {
			m_shortHelpMessage += line + QStringLiteral("\r\n");
			line = QStringLiteral("	") + cmds[i].remove(':');
		}

		if (i != cmds.count() - 1)
				line += QStringLiteral(", ");
	}

	m_shortHelpMessage += line + QStringLiteral("\r\n");

	m_shortHelpMessage += QStringLiteral("\r\n");
	m_shortHelpMessage += QStringLiteral("Detailed version is available by \"help\" command. \r\n");
	m_shortHelpMessage += QStringLiteral("\r\n");
}

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**	* Redistributions of source code must retain the above copyright
**		notice, this list of conditions and the following disclaimer.
**	* Redistributions in binary form must reproduce the above copyright
**		notice, this list of conditions and the following disclaimer in
**		the documentation and/or other materials provided with the
**		distribution.
**	* Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**		of its contributors may be used to endorse or promote products derived
**		from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qtlocalpeer.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QElapsedTimer>
#include <QTime>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRegularExpression>
#endif

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif
#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif

namespace QtLP_Private {
#include "qtlockedfile.cpp"
#if defined(Q_OS_WIN)
#include "qtlockedfile_win.cpp"
#else
#include "qtlockedfile_unix.cpp"
#endif
}

const char* QtLocalPeer::ack = "ack";

QtLocalPeer::QtLocalPeer(QObject* parent, const QString &appId)
	: QObject(parent), id(appId)
{
	QString prefix = id;
	if (id.isEmpty()) {
		id = QCoreApplication::applicationFilePath();
#if defined(Q_OS_WIN)
		id = id.toLower();
#endif
		prefix = id.section(QLatin1Char('/'), -1);
	}
	#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	prefix.remove(QRegularExpression("[^a-zA-Z]"));
	#else
	prefix.remove(QRegExp("[^a-zA-Z]"));
	#endif
	prefix.truncate(6);

	QByteArray idc = id.toUtf8();
	#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	quint16 idNum = qChecksum(QByteArrayView(idc));
	#else
	quint16 idNum = qChecksum(idc.constData(), idc.size());
	#endif
	socketName = QLatin1String("qtsingleapp-") + prefix
					+ QLatin1Char('-') + QString::number(idNum, 16);

#if defined(Q_OS_WIN)
	DWORD sessionId = 0;
	if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId)) {
		socketName += QLatin1Char('-') + QString::number(sessionId, 16);
	}
#else
	socketName += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif

	server = new QLocalServer(this);
	QString lockName = QDir(QDir::tempPath()).absolutePath()
						+ QLatin1Char('/') + socketName
						+ QLatin1String("-lockfile");
	lockFile.setFileName(lockName);
	lockFile.open(QIODevice::ReadWrite);
}



bool QtLocalPeer::isClient()
{
	if (lockFile.isLocked())
		return false;

	if (!lockFile.lock(QtLP_Private::QtLockedFile::WriteLock, false))
		return true;

	server->setSocketOptions(QLocalServer::UserAccessOption);
	bool res = server->listen(socketName);
#if defined(Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(4,5,0))
	// ### Workaround
	if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
		QFile::remove(QDir::cleanPath(QDir::tempPath())+QLatin1Char('/')+socketName);
		res = server->listen(socketName);
	}
#endif
	if (!res)
		qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qPrintable(server->errorString()));
	QObject::connect(server, &QLocalServer::newConnection, this, &QtLocalPeer::receiveConnection);
	return false;
}


bool QtLocalPeer::sendMessage(const QString &message, int timeout)
{
	if (!isClient())
		return false;

	QLocalSocket socket;
	bool connOk = false;
	for(int i = 0; i < 2; i++) {
		// Try twice, in case the other instance is just starting up
		socket.connectToServer(socketName);
		connOk = socket.waitForConnected(timeout/2);
		if (connOk || i)
			break;
		int ms = 250;
#if defined(Q_OS_WIN)
		Sleep(DWORD(ms));
#else
		struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
		nanosleep(&ts, NULL);
#endif
	}
	if (!connOk)
		return false;

	QByteArray uMsg(message.toUtf8());
	QDataStream ds(&socket);
	ds.writeBytes(uMsg.constData(), uMsg.size());
	bool res = socket.waitForBytesWritten(timeout);
	if (res) {
		res &= socket.waitForReadyRead(timeout);	// wait for ack
		if (res)
			res &= (socket.read(qstrlen(ack)) == ack);
	}
	return res;
}


void QtLocalPeer::receiveConnection()
{
	QLocalSocket* socket = server->nextPendingConnection();
	if (!socket)
		return;

	static constexpr qint64 kTotalTimeoutMs = 2000;
	QElapsedTimer totalTimer;
	totalTimer.start();

	while (socket->bytesAvailable() < (int)sizeof(quint32)) {
		const qint64 remMs = kTotalTimeoutMs - totalTimer.elapsed();
		if (remMs <= 0 || !socket->waitForReadyRead(static_cast<int>(remMs))) {
			delete socket;
			return;
		}
	}
	QDataStream ds(socket);
	quint32 remaining = 0;
	ds >> remaining;
	if (remaining > 4096) {
		qWarning("QtLocalPeer: Rejecting oversized IPC message (%u bytes)", remaining);
		delete socket;
		return;
	}
	QByteArray uMsg(static_cast<int>(remaining), '\0');
	char* uMsgBuf = uMsg.data();
	while (remaining > 0) {
		if (socket->bytesAvailable() == 0) {
			const qint64 remMs = kTotalTimeoutMs - totalTimer.elapsed();
			if (remMs <= 0 || !socket->waitForReadyRead(static_cast<int>(remMs))) {
				qWarning("QtLocalPeer: Message reception timed out (%u bytes remaining)", remaining);
				delete socket;
				return;
			}
		}
		const int got = ds.readRawData(uMsgBuf, static_cast<int>(remaining));
		if (got <= 0) {
			qWarning("QtLocalPeer: Message reception failed %s", socket->errorString().toLatin1().constData());
			delete socket;
			return;
		}
		remaining -= static_cast<quint32>(got);
		uMsgBuf += got;
	}
	QString message(QString::fromUtf8(uMsg));
	socket->write(ack, qstrlen(ack));
	socket->waitForBytesWritten(500);
	socket->waitForDisconnected(500); // make sure client reads ack
	delete socket;
	emit messageReceived(message); //### (might take a long time to return)
}

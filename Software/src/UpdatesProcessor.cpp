/*
 * UpdatesProcessor.cpp
 *
 *	Created on: 4/11/2014
 *		Project: Prismatik
 *
 *	Copyright (c) 2014 tim
 *
 *	Lightpack is an open-source, USB content-driving ambient lighting
 *	hardware.
 *
 *	Prismatik is a free, open-source software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published
 *	by the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Prismatik and Lightpack files is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QProcess>
#include "version.h"
#include "debug.h"
#include "UpdatesProcessor.hpp"
#include "Settings.hpp"

//#define UPDATE_CHECK_URL "https://psieg.de/lightpack/update.xml"
#define UPDATE_CHECK_URL "https://psieg.github.io/Lightpack/update.xml"

const AppVersion kCurVersion(VERSION_STR);

UpdatesProcessor::UpdatesProcessor(QObject * parent)
	: QObject(parent)
	, _reply(NULL)
{
}

void UpdatesProcessor::error(QNetworkReply::NetworkError code)
{
	qWarning() << Q_FUNC_INFO << "Updatecheck/updateload failed: " << code << ": " << _reply->errorString();
}

void UpdatesProcessor::requestUpdates()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "checking" << UPDATE_CHECK_URL;
	if(_reply != NULL) {
		_reply->disconnect();
		delete _reply;
		_reply = NULL;
	}

	QNetworkRequest request(QUrl(UPDATE_CHECK_URL));
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	_reply = _networkMan.get(request);
	connect(_reply, &QNetworkReply::finished, this, &UpdatesProcessor::readyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_reply, &QNetworkReply::errorOccurred, this, &UpdatesProcessor::error);
#else
	connect(_reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &UpdatesProcessor::error);
#endif
}

QList<UpdateInfo> UpdatesProcessor::readUpdates()
{
	QList<UpdateInfo> updates;

	QXmlStreamReader xmlReader(_reply->readAll());

	if (xmlReader.readNextStartElement()) {
		if (xmlReader.name() == QStringLiteral("updates")) {
			readUpdates(&updates, &xmlReader);
			QList<UpdateInfo>::iterator it = updates.begin();
			while (it != updates.end()) {
				UpdateInfo updateInfo = *it;
				if (!updateInfo.softwareVersion.isEmpty() && !isVersionMatches(updateInfo.softwareVersion, kCurVersion)) {
					updates.removeAll(updateInfo);
				} else {
					it++;
				}
			}
		}
	}
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << updates.size() << "updates available";
	return updates;
}

#ifdef Q_OS_WIN
namespace {
constexpr qint64 kMaxUpdatePkgBytes = 512LL * 1024 * 1024;
constexpr qint64 kMaxUpdateSigBytes = 1024;

bool isAllowedHttpsUrl(const QString& rawUrl, QUrl& outUrl)
{
	outUrl = QUrl(rawUrl, QUrl::StrictMode);
	return outUrl.isValid() &&
		outUrl.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0 &&
		!outUrl.host().isEmpty();
}

bool writeReplyStreamSafely(QNetworkReply* reply, const QString& destPath, qint64 maxBytes, qint64 minBytes = 1)
{
	QFileInfo fi(destPath);
	if (fi.isSymLink()) {
		qCritical() << "Refusing to write update artifact through symlink:" << destPath;
		return false;
	}

	QFile::remove(destPath);
	QFile f(destPath);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qCritical() << "Failed to open update artifact for writing:" << destPath;
		return false;
	}

	qint64 totalWritten = 0;
	char buf[65536];
	while (!reply->atEnd()) {
		const qint64 n = reply->read(buf, sizeof(buf));
		if (n < 0) {
			f.close();
			QFile::remove(destPath);
			return false;
		}
		if (n == 0)
			break;
		totalWritten += n;
		if (totalWritten > maxBytes) {
			qCritical() << "Update artifact exceeded size limit:" << destPath;
			f.close();
			QFile::remove(destPath);
			return false;
		}
		if (f.write(buf, n) != n) {
			qCritical() << "Short write on update artifact:" << destPath;
			f.close();
			QFile::remove(destPath);
			return false;
		}
	}

	if (totalWritten < minBytes || !f.flush()) {
		f.close();
		QFile::remove(destPath);
		return false;
	}
	f.close();
	return true;
}
} // namespace
#endif

void UpdatesProcessor::loadUpdate(UpdateInfo& info)
{
#ifdef Q_OS_WIN
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "fetching" << info.pkgUrl;
	if (info.pkgUrl.isEmpty() || info.sigUrl.isEmpty()) {
		qCritical() << Q_FUNC_INFO << "UpdateInfo is missing required links";
		return;
	}

	QUrl pkgUrlParsed;
	QUrl sigUrlParsed;
	if (!isAllowedHttpsUrl(info.pkgUrl, pkgUrlParsed) || !isAllowedHttpsUrl(info.sigUrl, sigUrlParsed)) {
		qCritical() << Q_FUNC_INFO << "Rejected non-HTTPS or invalid update package/signature URL";
		return;
	}

	_sigUrl = info.sigUrl;
	if (_reply != NULL) {
		_reply->disconnect();
		delete _reply;
		_reply = NULL;
	}

	QNetworkRequest request(pkgUrlParsed);
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#else
#	if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#	endif
#endif

	_reply = _networkMan.get(request);
	QNetworkReply* pkgReply = _reply;
	connect(pkgReply, &QNetworkReply::downloadProgress, this, [pkgReply](qint64 bytesReceived, qint64 bytesTotal) {
		if (bytesReceived > kMaxUpdatePkgBytes || bytesTotal > kMaxUpdatePkgBytes) {
			qCritical() << "Aborting oversized update package download";
			pkgReply->abort();
		}
	});
	connect(_reply, &QNetworkReply::finished, this, &UpdatesProcessor::updatePgkLoaded);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_reply, &QNetworkReply::errorOccurred, this, &UpdatesProcessor::error);
#else
	connect(_reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &UpdatesProcessor::error);
#endif

#else
	qWarning() << Q_FUNC_INFO << "Trying to load update on non-windows platform -- ignored";
#endif
}

#ifdef Q_OS_WIN
void UpdatesProcessor::updatePgkLoaded()
{
	if (!_reply)
		return;
	if (_reply->error() != QNetworkReply::NetworkError::NoError) {
		_reply->deleteLater();
		_reply = NULL;
		return;
	}

	DEBUG_MID_LEVEL << Q_FUNC_INFO << "fetching " << _sigUrl;

	const QString pkgPath = QDir::tempPath() + "\\PsiegUpdateElevate_Prismatik.exe";
	const bool wrotePkg = writeReplyStreamSafely(_reply, pkgPath, kMaxUpdatePkgBytes, 1024);
	_reply->deleteLater();
	_reply = NULL;
	if (!wrotePkg) {
		qCritical() << Q_FUNC_INFO << "Failed to write update package";
		return;
	}

	QUrl sigUrlParsed;
	if (!isAllowedHttpsUrl(_sigUrl, sigUrlParsed)) {
		QFile::remove(pkgPath);
		return;
	}

	QNetworkRequest request = QNetworkRequest(sigUrlParsed);
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#else
#	if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#	endif
#endif

	_reply = _networkMan.get(request);
	QNetworkReply* sigReply = _reply;
	connect(sigReply, &QNetworkReply::downloadProgress, this, [sigReply](qint64 bytesReceived, qint64 bytesTotal) {
		if (bytesReceived > kMaxUpdateSigBytes || bytesTotal > kMaxUpdateSigBytes) {
			qCritical() << "Aborting oversized update signature download";
			sigReply->abort();
		}
	});
	connect(_reply, &QNetworkReply::finished, this, &UpdatesProcessor::updateSigLoaded);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_reply, &QNetworkReply::errorOccurred, this, &UpdatesProcessor::error);
#else
	connect(_reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &UpdatesProcessor::error);
#endif
}

void UpdatesProcessor::updateSigLoaded()
{
	if (!_reply)
		return;
	if (_reply->error() != QNetworkReply::NetworkError::NoError) {
		_reply->deleteLater();
		_reply = NULL;
		return;
	}
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	const QString sigPath = QDir::tempPath() + "\\PsiegUpdateElevate_Prismatik.exe.sig";
	const bool wroteSig = writeReplyStreamSafely(_reply, sigPath, kMaxUpdateSigBytes, 64);
	_reply->deleteLater();
	_reply = NULL;
	if (!wroteSig) {
		qCritical() << Q_FUNC_INFO << "Failed to write update signature";
		QFile::remove(QDir::tempPath() + "\\PsiegUpdateElevate_Prismatik.exe");
		return;
	}

	// TODO: ensure user is not using the program
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "triggering update process";
	SettingsScope::Settings::setAutoUpdatingVersion(VERSION_STR);

	QStringList args;
	args.append("request");
	args.append(QDir::tempPath());
	args.append(QCoreApplication::applicationFilePath());
	if (QProcess::startDetached(QCoreApplication::applicationDirPath() + "\\UpdateElevate.exe", args)) {
		QCoreApplication::quit();
	} else {
		qCritical() << Q_FUNC_INFO << "Failed to start UpdateElevate.exe";
	}
}
#endif

bool UpdatesProcessor::isVersionMatches(const QString &predicate, const AppVersion &version)
{
	enum Condition {
		LT,
		GT,
		EQ,
		LE,
		GE
	};

	Condition cond;
	QString predicateRef(predicate);

	QString predicateVerStr = predicate.mid(2).trimmed();

	QString condStr = predicateRef.left(2);

	if (condStr == QStringLiteral("lt")) {
		cond = LT;
	} else if (condStr == QStringLiteral("gt")) {
		cond = GT;
	} else if (condStr == QStringLiteral("eq")) {
		cond = EQ;
	} else if (condStr == QStringLiteral("le")) {
		cond = LE;
	} else if (condStr == QStringLiteral("ge")) {
		cond = GE;
	} else {
		cond = EQ;
		predicateVerStr = predicateRef.trimmed();
	}

	AppVersion predicateVer(predicateVerStr);

	if (!version.isValid() || !predicateVer.isValid())
		return false;

	switch (cond) {
	case LT:
		return version < predicateVer;
		break;
	case GT:
		return version > predicateVer;
		break;
	case EQ:
		return version == predicateVer;
		break;
	case LE:
		return version <= predicateVer;
		break;
	case GE:
		return version >= predicateVer;
		break;
	}

	Q_ASSERT(false);
	return false;
}

QList<UpdateInfo> * UpdatesProcessor::readUpdates(QList<UpdateInfo> *updates, QXmlStreamReader *xmlReader)
{
	if (xmlReader->readNextStartElement()) {
		while (xmlReader->name() == QStringLiteral("update")) {
			UpdateInfo updateInfo;
			while(xmlReader->readNextStartElement()) {
				if (xmlReader->name() == QStringLiteral("id")) {
					xmlReader->readNext();
					QString id = xmlReader->text().toString();
					updateInfo.id = id.toUInt();
				} else if (xmlReader->name() == QStringLiteral("url")) {
					xmlReader->readNext();
					updateInfo.url = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("pkgUrl")) {
					xmlReader->readNext();
					updateInfo.pkgUrl = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("sigUrl")) {
					xmlReader->readNext();
					updateInfo.sigUrl = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("title")) {
					xmlReader->readNext();
					updateInfo.title = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("text")) {
					xmlReader->readNext();
					updateInfo.text = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("softwareVersion")) {
					xmlReader->readNext();
					updateInfo.softwareVersion = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("firmwareVersion")) {
					xmlReader->readNext();
					updateInfo.firmwareVersion = xmlReader->text().toString();
				} else if (xmlReader->name() == QStringLiteral("update")) {
					break;
				}
				xmlReader->skipCurrentElement();
			}
			// add updates with defined id only
			if(updateInfo.id > 0)
				updates->append(updateInfo);
		}
	}
	return updates;
}

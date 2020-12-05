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
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QProcess>
#include "version.h"
#include "debug.h"
#include "UpdatesProcessor.hpp"
#include "Settings.hpp"

// #define UPDATE_CHECK_URL "https://psieg.de/lightpack/update.xml"
// #define UPDATE_CHECK_URL "https://psieg.github.io/Lightpack/update.xml"
#define UPDATE_CHECK_URL "https://api.github.com/repos/psieg/Lightpack/releases/latest"

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

	QNetworkRequest request(QUrl(QStringLiteral(UPDATE_CHECK_URL)));
	request.setHeader(QNetworkRequest::UserAgentHeader, QVariant("psieg/Lightpack"));
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	_reply = _networkMan.get(request);
	connect(_reply, SIGNAL(finished()), this, SIGNAL(readyRead()));
	connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

UpdateInfo UpdatesProcessor::readUpdates()
{
	const QJsonObject& releaseObject = QJsonDocument::fromJson(_reply->readAll()).object();

	UpdateInfo update;

	update.id = releaseObject.value(QStringLiteral("id")).toInt();
	update.url = releaseObject.value(QStringLiteral("html_url")).toString();
	update.text = releaseObject.value(QStringLiteral("body")).toString();
	update.title = releaseObject.value(QStringLiteral("name")).toString();
	update.softwareVersion = QVersionNumber::fromString(releaseObject.value(QStringLiteral("tag_name")).toString());
	// find firmware in assets?
	// update.firmwareVersion = ...
	for (const QJsonValue& assetValue : releaseObject.value(QStringLiteral("assets")).toArray()) {
		const QJsonObject& obj = assetValue.toObject();
		if (obj.value(QStringLiteral("name")).toString().endsWith(QStringLiteral(".exe")))
			update.pkgUrl = obj.value(QStringLiteral("browser_download_url")).toString();
		else if (obj.value(QStringLiteral("name")).toString().endsWith(QStringLiteral(".exe.updater_signature")))
			update.sigUrl = obj.value(QStringLiteral("browser_download_url")).toString();
	}
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "latest available version" << update.softwareVersion;
	return update;
}

void UpdatesProcessor::loadUpdate(const UpdateInfo& info)
{
#ifdef Q_OS_WIN
	DEBUG_MID_LEVEL << Q_FUNC_INFO << "fetching" << info.pkgUrl;
	if (info.pkgUrl.isEmpty() || info.sigUrl.isEmpty()) {
		qCritical() << Q_FUNC_INFO << "UpdateInfo is missing required links";
		return;
	}

	_sigUrl = info.sigUrl;
	if (_reply != NULL) {
		_reply->disconnect();
		delete _reply;
		_reply = NULL;
	}

	QNetworkRequest request(QUrl(info.pkgUrl));
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#else
#	if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#	endif
#endif

	_reply = _networkMan.get(request);
	connect(_reply, SIGNAL(finished()), this, SLOT(updatePgkLoaded()));
	connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
#else
	Q_UNUSED(info)
	qWarning() << Q_FUNC_INFO << "Trying to load update on non-windows platform -- ignored";
#endif
}

#ifdef Q_OS_WIN
void UpdatesProcessor::updatePgkLoaded()
{
	if (!(_reply->error() == QNetworkReply::NetworkError::NoError))
		return;

	DEBUG_MID_LEVEL << Q_FUNC_INFO << "fetching " << _sigUrl;

	QFile f(QDir::tempPath() + QStringLiteral("\\PsiegUpdateElevate_Prismatik.exe"));
	if (!f.open(QIODevice::WriteOnly)) {
		qCritical() << Q_FUNC_INFO << "Failed to write update package";
	}
	f.write(_reply->readAll());
	f.close();

	_reply->deleteLater();
	_reply = NULL;

	QNetworkRequest request = QNetworkRequest(QUrl(_sigUrl));
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#else
#	if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#	endif
#endif

	_reply = _networkMan.get(request);
	connect(_reply, SIGNAL(finished()), this, SLOT(updateSigLoaded()));
	connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

void UpdatesProcessor::updateSigLoaded()
{
	if (!(_reply->error() == QNetworkReply::NetworkError::NoError))
		return;
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	QFile f(QDir::tempPath() + QStringLiteral("\\PsiegUpdateElevate_Prismatik.exe.sig"));
	if (!f.open(QIODevice::WriteOnly)) {
		qCritical() << Q_FUNC_INFO << "Failed to write update signature";
	}
	f.write(_reply->readAll());
	f.close();

	_reply->deleteLater();
	_reply = NULL;

	// TODO: ensure user is not using the program
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "triggering update process";
	SettingsScope::Settings::setAutoUpdatingVersion(VERSION_STR);

	QStringList args;
	args.append("request");
	args.append(QDir::tempPath());
	args.append(QCoreApplication::applicationFilePath());
	if (QProcess::startDetached(QCoreApplication::applicationDirPath() + QStringLiteral("\\UpdateElevate.exe"), args)) {
		QCoreApplication::quit();
	} else {
		qCritical() << Q_FUNC_INFO << "Failed to start UpdateElevate.exe";
	}
}
#endif

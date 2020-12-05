/*
 * UpdatesProcessor.hpp
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

#ifndef NEWSPROCESSOR_HPP
#define NEWSPROCESSOR_HPP

#include <QStringList>
#include <QString>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>

class QNetworkReply;

struct UpdateInfo
{
	UpdateInfo()
		: id(0)
//		, url(NULL)
//		, text(NULL)
//		, softwareVersion(NULL)
//		, firmwareVersion(NULL)
	{}
	unsigned int id;
	QString		url;
	QString		pkgUrl;
	QString		sigUrl;
	QString		title;
	QString		text;
	QVersionNumber		softwareVersion;
	// QVersionNumber		firmwareVersion;

	bool operator== (const UpdateInfo &other) const {
		return this->id == other.id;
	}

	bool operator!= (const UpdateInfo &other) const {
		return !(*this == other);
	}
};

class UpdatesProcessor: public QObject
{
	Q_OBJECT
public:
	UpdatesProcessor(QObject * parent = NULL);
	void requestUpdates();
	UpdateInfo readUpdates();
	void loadUpdate(const UpdateInfo& info);

signals:
	void readyRead();

private slots:
	void error(QNetworkReply::NetworkError code);
#ifdef Q_OS_WIN
	void updatePgkLoaded();
	void updateSigLoaded();
#endif

private:
	QNetworkAccessManager _networkMan;
	QNetworkReply * _reply;
	QString _sigUrl;
};

#endif // NEWSPROCESSOR_HPP

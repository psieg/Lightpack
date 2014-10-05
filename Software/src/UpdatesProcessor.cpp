/*
 * UpdatesProcessor.cpp
 *
 *  Created on: 4/11/2014
 *     Project: Prismatik
 *
 *  Copyright (c) 2014 tim
 *
 *  Lightpack is an open-source, USB content-driving ambient lighting
 *  hardware.
 *
 *  Prismatik is a free, open-source software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Prismatik and Lightpack files is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QXmlStreamReader>
#include <QNetworkReply>
#include "version.h"
#include "common/DebugOut.hpp"
#include "UpdatesProcessor.hpp"

//#define UPDATE_CHECK_URL "https://psieg.de/lightpack/update.xml"
#define UPDATE_CHECK_URL "https://psieg.github.io/Lightpack/update.xml"

const AppVersion kCurVersion(VERSION_STR);

UpdatesProcessor::UpdatesProcessor(QObject * parent)
    : QObject(parent)
    , _reply(NULL)
{
}

void UpdatesProcessor::error(QNetworkReply::NetworkError code){
	qWarning() << Q_FUNC_INFO << "Updatecheck failed: " << code << ": " << _reply->errorString();
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
	connect(_reply, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
	connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

QList<UpdateInfo> UpdatesProcessor::readUpdates()
{
    QList<UpdateInfo> updates;

    QXmlStreamReader xmlReader(_reply->readAll());

    if (xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "updates") {
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
	DEBUG_MID_LEVEL << Q_FUNC_INFO << updates.size() << "updates available";
    return updates;
}

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
    QStringRef predicateRef(&predicate);

    QStringRef predicateVerStr = predicateRef.mid(2).trimmed();

    QStringRef condStr = predicateRef.left(2);

    if (condStr == "lt") {
        cond = LT;
    } else if (condStr == "gt") {
        cond = GT;
    } else if (condStr == "eq") {
        cond = EQ;
    } else if (condStr == "le") {
        cond = LE;
    } else if (condStr == "ge") {
        cond = GE;
    } else {
        cond = EQ;
        predicateVerStr = predicateRef.trimmed();
    }

    AppVersion predicateVer(predicateVerStr.toString());

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
        while (xmlReader->name() == "update") {
            UpdateInfo updateInfo;
            while(xmlReader->readNextStartElement()) {
                if (xmlReader->name() == "id") {
                    xmlReader->readNext();
                    QStringRef id = xmlReader->text();
                    updateInfo.id = id.toUInt();
                } else if (xmlReader->name() == "url") {
                    xmlReader->readNext();
                    updateInfo.url = xmlReader->text().toString();
                } else if (xmlReader->name() == "title") {
                    xmlReader->readNext();
                    updateInfo.title = xmlReader->text().toString();
                } else if (xmlReader->name() == "text") {
                    xmlReader->readNext();
                    updateInfo.text = xmlReader->text().toString();
                } else if (xmlReader->name() == "softwareVersion") {
                    xmlReader->readNext();
                    updateInfo.softwareVersion = xmlReader->text().toString();
                } else if (xmlReader->name() == "firmwareVersion") {
                    xmlReader->readNext();
                    updateInfo.firmwareVersion = xmlReader->text().toString();
                } else if (xmlReader->name() == "update") {
                    break;
                } else {
                    xmlReader->skipCurrentElement();
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

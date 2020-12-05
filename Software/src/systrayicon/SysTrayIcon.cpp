/*
 * SysTrayIcon.cpp
 *
 *	Created on: 11/23/2013
 *		Project: Prismatik
 *
 *	Copyright (c) 2013 tim
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
#include "SysTrayIcon.hpp"

#include <QStringList>
#include <QSystemTrayIcon>
#include <QDesktopServices>
#include <QNetworkReply>
#include "Settings.hpp"
#include "version.h"

SysTrayIcon::SysTrayIcon(QObject *parent) :
	QObject(parent)
{

	QMenu *trayIconMenu = new QMenu();

	createActions();

	fillProfilesFromSettings();

	trayIconMenu->addAction(_switchOnBacklightAction);
	trayIconMenu->addAction(_switchOffBacklightAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addMenu(_profilesMenu);
	trayIconMenu->addAction(_settingsAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(_quitAction);

	_qsystray = new QSystemTrayIcon();
	_qsystray->setContextMenu(trayIconMenu);
}

SysTrayIcon::~SysTrayIcon()
{
	delete _qsystray->contextMenu();
	delete _profilesMenu;
	delete _qsystray;
}

void SysTrayIcon::init()
{
	connect(_qsystray, SIGNAL(activated(QSystemTrayIcon::ActivationReason))
		, this, SLOT(onTrayIcon_Activated(QSystemTrayIcon::ActivationReason)));
	connect(_qsystray, SIGNAL(messageClicked())
		, this, SLOT(onTrayIcon_MessageClicked()));

	connect(&_updatesProcessor, SIGNAL(readyRead()), this, SLOT(onCheckUpdate_Finished()));

	_pixmapCache.insert(QStringLiteral("lock32"), new QPixmap(QPixmap(QStringLiteral(":/icons/lock.png")).scaledToWidth(32, Qt::SmoothTransformation)));
	_pixmapCache.insert(QStringLiteral("on32"), new QPixmap(QPixmap(QStringLiteral(":/icons/on.png")).scaledToWidth(32, Qt::SmoothTransformation)));
	_pixmapCache.insert(QStringLiteral("off32"), new QPixmap(QPixmap(QStringLiteral(":/icons/off.png")).scaledToWidth(32, Qt::SmoothTransformation)));
	_pixmapCache.insert(QStringLiteral("error32"), new QPixmap(QPixmap(QStringLiteral(":/icons/error.png")).scaledToWidth(32, Qt::SmoothTransformation)));
	_pixmapCache.insert(QStringLiteral("persist32"), new QPixmap(QPixmap(QStringLiteral(":/icons/persist.png")).scaledToWidth(32, Qt::SmoothTransformation)));

	setStatus(SysTrayIcon::StatusOn);
	_qsystray->show();
}

bool SysTrayIcon::isVisible() const
{
	return _qsystray->isVisible();
}

void SysTrayIcon::showMessage(const QString &title, const QString &text, QSystemTrayIcon::MessageIcon icon)
{
	_trayMessage = SysTrayIcon::MessageGeneric;
	_qsystray->showMessage(title, text, icon);
}

void SysTrayIcon::showMessage(const Message msg)
{
	switch (msg)
	{
		case SysTrayIcon::MessageAnotherInstance:
			_qsystray->showMessage(tr("Prismatik"), tr("Application already running"));
			break;
		case SysTrayIcon::MessageUpdateFirmware:
			_qsystray->showMessage(tr("Lightpack firmware update"), tr("Click on this message to open lightpack downloads page"));
			break;
		default:
			_trayMessage = SysTrayIcon::MessageGeneric;
			return;
	}
	_trayMessage = msg;
}

void SysTrayIcon::show()
{
	_qsystray->show();
}

void SysTrayIcon::hide()
{
	_qsystray->hide();
}

QString SysTrayIcon::toolTip() const
{
	return _qsystray->toolTip();
}

void SysTrayIcon::retranslateUi()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	_switchOnBacklightAction->setText(tr("&Turn on"));
	_switchOffBacklightAction->setText(tr("&Turn off"));
	_settingsAction->setText(tr("&Settings"));
	_quitAction->setText(tr("&Quit"));
	_profilesMenu->setTitle(tr("&Profiles"));

	fillProfilesFromSettings();

	switch (_status)
	{
		case SysTrayIcon::StatusOn:
			_qsystray->setToolTip(tr("Enabled profile: %1").arg(SettingsScope::Settings::getCurrentProfileName()));
			break;
		case SysTrayIcon::StatusOff:
			_qsystray->setToolTip(tr("Disabled"));
			break;
		case SysTrayIcon::StatusError:
			_qsystray->setToolTip(tr("Error with connection device, verbose in logs"));
			break;
		default:
			qWarning() << Q_FUNC_INFO << "_status contains crap =" << _status;
			break;
	}
}

void SysTrayIcon::checkUpdate()
{
	_updatesProcessor.requestUpdates();
}

void SysTrayIcon::setStatus(const Status status, const QString *arg)
{
	switch (status)
	{
		case SysTrayIcon::StatusOn:
			_switchOnBacklightAction->setEnabled(false);
			_switchOffBacklightAction->setEnabled(true);
			_qsystray->setIcon(QIcon(*_pixmapCache[QStringLiteral("on32")]));

			if (SettingsScope::Settings::isProfileLoaded())
				_qsystray->setToolTip(tr("Enabled profile: %1").arg(SettingsScope::Settings::getCurrentProfileName()));
			break;

		case SysTrayIcon::StatusLockedByApi:
			_switchOnBacklightAction->setEnabled(false);
			_switchOffBacklightAction->setEnabled(true);
			_qsystray->setIcon(QIcon(*_pixmapCache[QStringLiteral("lock32")]));
			_qsystray->setToolTip(tr("Device locked via API"));
			break;
		case SysTrayIcon::StatusLockedByPlugin:
			_qsystray->setIcon(QIcon(*_pixmapCache[QStringLiteral("lock32")]));
			_qsystray->setToolTip(tr("Device locked via Plugin") + " (" + (*arg) + ")");
			break;
		case SysTrayIcon::StatusApiPersist:
			_switchOnBacklightAction->setEnabled(true);
			_switchOffBacklightAction->setEnabled(true);
			_qsystray->setIcon(QIcon(*_pixmapCache[QStringLiteral("persist32")]));
			_qsystray->setToolTip(tr("API colors persisted"));
			break;

		case SysTrayIcon::StatusOff:
			_switchOnBacklightAction->setEnabled(true);
			_switchOffBacklightAction->setEnabled(false);
			_qsystray->setIcon(QIcon(*_pixmapCache[QStringLiteral("off32")]));
			_qsystray->setToolTip(tr("Disabled"));
			break;

		case SysTrayIcon::StatusError:
			_switchOnBacklightAction->setEnabled(false);
			_switchOffBacklightAction->setEnabled(true);
			_qsystray->setIcon(QIcon(*_pixmapCache[QStringLiteral("error32")]));
			_qsystray->setToolTip(tr("Error with connection device, verbose in logs"));
			break;
		default:
			qWarning() << Q_FUNC_INFO << "status = " << status;
			return;
			break;
	}
	_status = status;
}

void SysTrayIcon::updateProfiles()
{
	fillProfilesFromSettings();
	if (_status == SysTrayIcon::StatusOn) {
		QString profileName = SettingsScope::Settings::getCurrentProfileName();
		setStatus(_status, &profileName);
	}
}


void SysTrayIcon::onCheckUpdate_Finished()
{
	using namespace SettingsScope;
	const UpdateInfo& update = _updatesProcessor.readUpdates();
	const QVersionNumber& thisVersion = QVersionNumber::fromString(QStringLiteral(VERSION_STR));
	const int versionDifference = QVersionNumber::compare(update.softwareVersion, thisVersion);

	if (versionDifference > 0) {
		if (versionDifference > 1) {
			_trayMessage = SysTrayIcon::MessageGeneric;
			_trayMsgUrl = QUrl(QStringLiteral("https://github.com/psieg/Lightpack/releases"));
			_qsystray->showMessage(QStringLiteral("Multiple updates are available"), QStringLiteral("Click to open the downloads page"));
		} else {
#ifdef Q_OS_WIN
			if (Settings::isInstallUpdatesEnabled() && !update.pkgUrl.isEmpty() && !update.sigUrl.isEmpty()) {
				_trayMessage = SysTrayIcon::MessageNoAction;
				_trayMsgUrl = QUrl("");
				_qsystray->showMessage(QStringLiteral("Prismatik Update"), QStringLiteral("An update is being downloaded and will be applied shortly."));
				_updatesProcessor.loadUpdate(update);
				return;
			}
#endif
			_trayMessage = SysTrayIcon::MessageGeneric;
			_trayMsgUrl = QUrl(update.url);
			_qsystray->showMessage(update.title, update.text);
		}
	}
}

void SysTrayIcon::onProfileAction_Triggered()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	QString profileName = static_cast<QAction*>(sender())->text();
	emit profileSwitched(profileName);
}

void SysTrayIcon::onTrayIcon_MessageClicked()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _trayMessage;

	switch (_trayMessage)
	{
		case SysTrayIcon::MessageUpdateFirmware:
			if (SettingsScope::Settings::getConnectedDevice() == SupportedDevices::DeviceTypeLightpack)
			{
				// Open lightpack downloads page
				QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/psieg/Lightpack/releases"), QUrl::TolerantMode));
			}
			break;
		case SysTrayIcon::MessageGeneric:
			QDesktopServices::openUrl(_trayMsgUrl);
			break;

		default:
			break;
	}
}

void SysTrayIcon::onTrayIcon_Activated(QSystemTrayIcon::ActivationReason reason)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	switch (reason)
	{
		case QSystemTrayIcon::DoubleClick:
			switch (_status)
			{
				case SysTrayIcon::StatusOff:
					setStatus(SysTrayIcon::StatusOn);
					emit backlightOn();
					break;
				case SysTrayIcon::StatusOn:
				case SysTrayIcon::StatusLockedByApi:
				case SysTrayIcon::StatusLockedByPlugin:
				case SysTrayIcon::StatusError:
					setStatus(SysTrayIcon::StatusOff);
					emit backlightOff();
					break;
				default:
					qWarning() << Q_FUNC_INFO << "_status = " << _status;
					break;
			}
			break;

		case QSystemTrayIcon::MiddleClick:
			emit toggleSettings();
			break;

			//	#	ifdef Q_OS_WIN
		case QSystemTrayIcon::Context:
			// Hide the tray after losing focus
			//
			// In Linux (Ubuntu 10.04) this code grab keyboard input and
			// not give it back to the text editor after close application with
			// "Quit" button in the tray menu
			_qsystray->contextMenu()->activateWindow();
			break;
			//	#	endif


		default:
			;
	}
}

void SysTrayIcon::createActions()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	_switchOnBacklightAction = new QAction(QIcon(QStringLiteral(":/icons/on.png")), tr("&Turn on"), this);
	_switchOnBacklightAction->setIconVisibleInMenu(true);
	connect(_switchOnBacklightAction, SIGNAL(triggered()), this, SIGNAL(backlightOn()));

	_switchOffBacklightAction = new QAction(QIcon(QStringLiteral(":/icons/off.png")), tr("&Turn off"), this);
	_switchOffBacklightAction->setIconVisibleInMenu(true);
	connect(_switchOffBacklightAction, SIGNAL(triggered()), this, SIGNAL(backlightOff()));


	_profilesMenu = new QMenu(tr("&Profiles"));
	_profilesMenu->setIcon(QIcon(QStringLiteral(":/icons/profiles.png")));
	_profilesMenu->clear();

	_settingsAction = new QAction(QIcon(QStringLiteral(":/icons/settings.png")), tr("&Settings"), this);
	_settingsAction->setIconVisibleInMenu(true);
	connect(_settingsAction, SIGNAL(triggered()), this, SIGNAL(showSettings()));

	_quitAction = new QAction(tr("&Quit"), this);
	connect(_quitAction, SIGNAL(triggered()), this, SIGNAL(quit()));
}

void SysTrayIcon::fillProfilesFromSettings()
{
	using namespace SettingsScope;
	QAction *profileAction;

	_profilesMenu->clear();

	QStringList profiles = Settings::findAllProfiles();

	for (int i = 0; i < profiles.count(); i++) {
		profileAction = new QAction(profiles[i], _profilesMenu);
		profileAction->setCheckable(true);
		if (SettingsScope::Settings::isProfileLoaded() && profiles[i].compare(Settings::getCurrentProfileName()) == 0) {
			profileAction->setChecked(true);
		}
		_profilesMenu->addAction(profileAction);
		connect(profileAction, SIGNAL(triggered()), this, SLOT(onProfileAction_Triggered()));
	}
}

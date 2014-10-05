/*
 * MonitorsConfigurationPage.cpp
 *
 *  Created on: 10/23/2013
 *     Project: Prismatik
 *
 *  Copyright (c) 2013 Tim
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

#include "MonitorConfigurationPage.hpp"

#include <QDesktopWidget>
#include <QRadioButton>

#include "common/DebugOut.hpp"
#include "ui_MonitorConfigurationPage.h"
#include "MonitorIdForm.hpp"

MonitorConfigurationPage::MonitorConfigurationPage(bool isInitFromSettings, TransientSettings *ts, QWidget *parent) :
    QWizardPage(parent),
    SettingsAwareTrait(isInitFromSettings, ts),
    _ui(new Ui::MonitorsConfigurationPage)
{
    _ui->setupUi(this);
    registerField("screenId", _ui->cbMonitor);
}

MonitorConfigurationPage::~MonitorConfigurationPage()
{
    delete _ui;
}

void MonitorConfigurationPage::initializePage()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	_ui->cbMonitor->clear();
    int screenCount = QApplication::desktop()->screenCount();
    for (int i = 0; i < screenCount; i++) {
        QRect geom = QApplication::desktop()->screenGeometry(i);
        MonitorIdForm *monitorIdForm = new MonitorIdForm();

        monitorIdForm->setWindowFlags(Qt::FramelessWindowHint);

        monitorIdForm->move(geom.topLeft());
        monitorIdForm->resize(geom.width(), geom.height());

        monitorIdForm->setId(i+1);

        QString text = QString("Monitor %0, %1x%2").arg(QString::number(i+1), QString::number(geom.width()), QString::number(geom.height()));
        _ui->cbMonitor->addItem(text, (int)i);

        DEBUG_LOW_LEVEL << Q_FUNC_INFO << QString("Monitor %0, %1x%2+%3+%4").arg(QString::number(i+1), QString::number(geom.width()), QString::number(geom.height()), QString::number(geom.left()), QString::number(geom.top()) );

        monitorIdForm->show();

        _monitorForms.append(monitorIdForm);
    }
}

bool MonitorConfigurationPage::validatePage()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;
    cleanupMonitors();
    return true;
}

void MonitorConfigurationPage::cleanupPage()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;
    cleanupMonitors();
}

void MonitorConfigurationPage::cleanupMonitors()
{
    foreach ( MonitorIdForm *monitorIdForm, _monitorForms ) {
        delete monitorIdForm;
    }
    _monitorForms.clear();
}

void MonitorConfigurationPage::addMonitor(int id)\
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;
    QRadioButton *rbMon = new QRadioButton(this);

    char buf[10];
    QString name = QString("rb%0").arg(QString::number(id));

    rbMon->setObjectName(name);
    rbMon->setText(buf);

    _ui->gridLayout->addWidget(rbMon, id + 1, 1, 1, 1);

}

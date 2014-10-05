/*
 * SelectProfilePage.cpp
 *
 *  Created on: 11/1/2013
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

#include "SelectProfilePage.hpp"

#include <QLineEdit>

#include "common/DebugOut.hpp"
#include "ui_SelectProfilePage.h"
#include "Settings.hpp"

using namespace SettingsScope;

SelectProfilePage::SelectProfilePage(bool isInitFromSettings, TransientSettings *ts, QWidget *parent):
    QWizardPage(parent),
    SettingsAwareTrait(isInitFromSettings, ts),
    ui(new Ui::SelectProfilePage)
{
    ui->setupUi(this);
}

SelectProfilePage::~SelectProfilePage()
{
    delete ui;
}

void SelectProfilePage::initializePage()
{
    QStringList profiles = Settings::findAllProfiles();
    ui->cbProfile->addItems(profiles);

    QRegExpValidator *validatorProfileName = new QRegExpValidator(QRegExp("[^<>:\"/\\|?*]*"), this);
    ui->cbProfile->lineEdit()->setValidator(validatorProfileName);
    connect(ui->cbProfile->lineEdit(), SIGNAL(editingFinished()) /* or returnPressed() */, this, SLOT(profileRename()));
    ui->cbProfile->setCurrentText(Settings::getCurrentProfileName());

}

void SelectProfilePage::profileRename()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;

    QString configName = ui->cbProfile->currentText().trimmed();
    ui->cbProfile->lineEdit()->setText(configName);

    // Signal editingFinished() will be emited if focus wasn't lost (for example when return pressed),
    // and profileRename() function will be called again here
    this->setFocus(Qt::OtherFocusReason);

    if (Settings::getCurrentProfileName() == configName)
    {
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Nothing has changed";
        return;
    }

    if (configName == "")
    {
        configName = Settings::getCurrentProfileName();
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Profile name is empty, return back to" << configName;
    }
    else
    {
        Settings::renameCurrentProfile(configName);
    }

    ui->cbProfile->lineEdit()->setText(configName);
    ui->cbProfile->setItemText(ui->cbProfile->currentIndex(), configName);
}

bool SelectProfilePage::validatePage()
{
    if(Settings::getCurrentProfileName().compare(ui->cbProfile->currentText()) != 0)
        Settings::loadOrCreateProfile(ui->cbProfile->currentText());
    return true;
}

void SelectProfilePage::on_pbAddProfile_clicked()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;

    QString profileName = tr("New profile");

    if(ui->cbProfile->findText(profileName) != -1){
        int i = 1;
        while(ui->cbProfile->findText(profileName +" "+ QString::number(i)) != -1){
            i++;
        }
        profileName += + " " + QString::number(i);
    }

    ui->cbProfile->insertItem(0, profileName);
    ui->cbProfile->setCurrentIndex(0);

    ui->cbProfile->lineEdit()->selectAll();
    ui->cbProfile->lineEdit()->setFocus(Qt::MouseFocusReason);
    Settings::loadOrCreateProfile(ui->cbProfile->currentText());
}

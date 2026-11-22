/*
 * LightpacksDiscoveryPage.cpp
 *
 *	Created on: 10/23/2013
 *		Project: Prismatik
 *
 *	Copyright (c) 2013 Tim
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

#include "LightpackDiscoveryPage.hpp"
#include "ui_LightpackDiscoveryPage.h"
#include "LedDeviceLightpack.hpp"
#include "Settings.hpp"
#include "Wizard.hpp"

LightpackDiscoveryPage::LightpackDiscoveryPage(bool isInitFromSettings, TransientSettings *ts, QWidget *parent) :
	QWizardPage(parent),
	SettingsAwareTrait(isInitFromSettings, ts),
	_ui(new Ui::LightpacksDiscoveryPage)
{
	_ui->setupUi(this);
}

LightpackDiscoveryPage::~LightpackDiscoveryPage()
{
	delete _ui;
}

void LightpackDiscoveryPage::initializePage() {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;


	LedDeviceLightpack *lpack = new LedDeviceLightpack();
	_transSettings->ledDevice.reset(lpack);

	lpack->open();
	if (lpack->lightpacksFound() > 0) {
		lpack->updateDeviceSettings();
		_ui->rbChooseAnotherDevice->setEnabled(true);
		_ui->rbLightpackSelected->setEnabled(true);

		_ui->rbLightpackSelected->setChecked(true);

		QString caption = tr("%n Lightpack(s) found", 0, lpack->lightpacksFound());
		QString caption2 = tr("%n zones are available", 0, lpack->maxLedsCount());
		_ui->labelLightpacksCount->setText(caption);
		_ui->labelZonesAvailable->setText(caption2);
	}
}

bool LightpackDiscoveryPage::validatePage() {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	return true;
}

int LightpackDiscoveryPage::nextId() const {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if (_ui->rbLightpackSelected->isChecked())
		return Page_ChooseProfile;
	else
		return Page_ChooseDevice;
}


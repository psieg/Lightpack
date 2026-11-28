/*
 * Wizard.cpp
 *
 *	Created on: 10/22/2013
 *		Project: %PROJECT% (Use "Lightpack" for hardware/firmware, or "Prismatik" for software)
 *
 *	Copyright (c) 2013 %NICKNAME%
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

#include "Wizard.hpp"
#include "ui_Wizard.h"
#include "LightpackDiscoveryPage.hpp"
#include "ZonePlacementPage.hpp"
#include "SelectProfilePage.hpp"
#include "SelectDevicePage.hpp"
#include "ConfigureDevicePage.hpp"
#include "ConfigureUdpDevicePage.hpp"
#include "ConfigureDevicePowerPage.hpp"
#include "GlobalColorCoefPage.hpp"
#include "AbstractLedDevice.hpp"

Wizard::Wizard(bool isInitFromSettings, QWidget *parent) :
	QWizard(parent),
	SettingsAwareTrait(isInitFromSettings, NULL),
	_ui(new Ui::Wizard)
{
	_transSettings = new TransientSettings;

	_ui->setupUi(this);
	_defaultStyle = wizardStyle();
	this->setPage(Page_LightpackDiscovery, new LightpackDiscoveryPage(_isInitFromSettings, _transSettings) );
	this->setPage(Page_ChooseDevice, new SelectDevicePage(_isInitFromSettings, _transSettings) );
	this->setPage(Page_ConfigureDevice, new ConfigureDevicePage(_isInitFromSettings, _transSettings) );
	this->setPage(Page_ConfigureUdpDevice, new ConfigureUdpDevicePage(_isInitFromSettings, _transSettings));
	this->setPage(Page_ChooseProfile, new SelectProfilePage(_isInitFromSettings, _transSettings));
	this->setPage(Page_ZonePlacement, new ZonePlacementPage(_isInitFromSettings, _transSettings));
	this->setPage(Page_ConfigureDevicePower, new ConfigureDevicePowerPage(_isInitFromSettings, _transSettings));
	this->setPage(Page_GlobalColorCoef, new GlobalColorCoefPage(_isInitFromSettings, _transSettings));

	updateWizardStyle();
}

void Wizard::changeEvent(QEvent *event)
{
	QWizard::changeEvent(event);
	if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
		updateWizardStyle();
}

void Wizard::updateWizardStyle()
{
#ifdef Q_OS_WIN
	// The Windows default AeroStyle paints the page area white with a hardcoded
	// blue title, ignoring the palette. With a dark palette (Qt 6 dark mode) the
	// page text becomes white-on-white, so fall back to ModernStyle, which
	// follows the palette.
	const QPalette pal = palette();
	const bool isDark = pal.color(QPalette::Window).lightness() < pal.color(QPalette::WindowText).lightness();
	const WizardStyle wanted = isDark ? ModernStyle : _defaultStyle;
	if (wizardStyle() != wanted)
		setWizardStyle(wanted);
#endif
}

Wizard::~Wizard()
{
//	if (_transSettings->ledDevice != NULL)
//		delete _transSettings->ledDevice;
	delete _transSettings;
	delete _ui;
}

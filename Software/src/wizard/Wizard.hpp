/*
 * Wizard.hpp
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

#ifndef WIZARD_HPP
#define WIZARD_HPP

#include <QApplication>
#include <QWizard>
#include "SettingsAwareTrait.hpp"

namespace Ui {
class Wizard;
}

enum {
	Page_LightpackDiscovery,
	Page_ChooseDevice,
	Page_ConfigureDevice,
	Page_ConfigureUdpDevice,
	Page_ChooseProfile,
	Page_ZonePlacement,
	Page_ConfigureDevicePower,
	Page_GlobalColorCoef
};

class Wizard : public QWizard, SettingsAwareTrait
{
	Q_OBJECT

public:
	explicit Wizard(bool isInitFromSettings, QWidget *parent = 0);
	~Wizard();

public slots:

protected:
	void changeEvent(QEvent *event) override;

private:
	void updateWizardStyle();

	Ui::Wizard *_ui;
	WizardStyle _defaultStyle;
};

#endif // WIZARD_HPP

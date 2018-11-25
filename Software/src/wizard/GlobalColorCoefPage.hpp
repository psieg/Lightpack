/*
 * GlobalColorCoefPage.hpp
 *
 *	Created on: 15.2.2017
 *		Project: Prismatik
 *
 *	Copyright (c) 2017 Patrick Siegler
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

#ifndef GLOBALCOLORCOEFPAGE_HPP
#define GLOBALCOLORCOEFPAGE_HPP

#include <QList>
#include "WizardPageUsingDevice.hpp"
#include "SettingsAwareTrait.hpp"

namespace Ui {
class GlobalColorCoefPage;
}
class MonitorIdForm;

class GlobalColorCoefPage : public WizardPageUsingDevice
{
	Q_OBJECT

public:
	explicit GlobalColorCoefPage(bool isInitFromSettings, TransientSettings *ts, QWidget *parent = 0);
	~GlobalColorCoefPage();

protected:
	virtual void initializePage();
	virtual void cleanupPage();
	virtual bool validatePage();

private slots:
	void onCoefValueChanged();
	void onColorTemperatureValueChanged();

private:
	void addMonitor(int id);
	void cleanupMonitors();

	Ui::GlobalColorCoefPage *_ui;
	int _screenId;
	QList<MonitorIdForm*> _monitorForms;
};

#endif // GLOBALCOLORCOEFPAGE_HPP

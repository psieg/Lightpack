/*
 * MonitorsConfigurationPage.cpp
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

#include <QRadioButton>
#include "GlobalColorCoefPage.hpp"
#include "ui_GlobalColorCoefPage.h"
#include "MonitorIdForm.hpp"
#include "AbstractLedDevice.hpp"
#include "Settings.hpp"
#include "debug.h"
#include "PrismatikMath.hpp"
#include "GrabWidget.hpp"

GlobalColorCoefPage::GlobalColorCoefPage(bool isInitFromSettings, TransientSettings *ts, QWidget *parent) :
	WizardPageUsingDevice(isInitFromSettings, ts, parent),
	_ui(new Ui::GlobalColorCoefPage)
{
	_ui->setupUi(this);

	// The color swatches paint their palette's Window color. Without an explicit
	// background role they inherit the parent's, which QWizard::ModernStyle (used
	// in dark mode) sets to Base, so they would paint white instead.
	const QList<QLabel *> labels = findChildren<QLabel *>();
	for (QLabel *label : labels) {
		if (label->autoFillBackground() && label->palette().isBrushSet(QPalette::Active, QPalette::Window))
			label->setBackgroundRole(QPalette::Window);
	}
}

GlobalColorCoefPage::~GlobalColorCoefPage()
{
	delete _ui;
}

void GlobalColorCoefPage::initializePage()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	int i = 0;
	for (const QScreen* const screen : QGuiApplication::screens()) {
		const QString& displayName = QStringLiteral("Display %1").arg(QString::number(i + 1));
		MonitorIdForm* const monitorIdForm = new MonitorIdForm(displayName, screen->geometry());
		monitorIdForm->show();
		_monitorForms.append(monitorIdForm);

		_ui->cbMonitorSelect->addItem(displayName, i);
		MonitorSettings settings;
		settings.screen = screen;
		settings.red = _ui->sbRed->value();
		settings.green = _ui->sbGreen->value();
		settings.blue = _ui->sbBlue->value();
		settings.colorTemp = _ui->hsColorTemperature->value();
		_screens.insert(i, settings);
		++i;
	}
	this->activateWindow();

	// RESETS WBA
	resetDeviceSettings();

	bool firstPass = true;
	for (MonitorSettings& settings : _screens) {
		int firstId = -1;
		for (int i = 0; i < _transSettings->ledCount; i++) {
			if (_transSettings->zonePositions.contains(i)) {
				// grab first ID for each screen to use as base setting
				if (settings.screen->geometry().contains(_transSettings->zonePositions[i]))
					firstId = firstId < 0 ? i : std::min(firstId, i);
				// add areas only on first pass
				if (firstPass)
					addGrabArea(i);
			}
		}
		if (firstId > -1) {
			const GrabWidget* const firstWidget = _grabAreas[firstId];
			settings.red = firstWidget->getCoefRed() * _ui->sbRed->maximum();
			settings.green = firstWidget->getCoefGreen() * _ui->sbGreen->maximum();
			settings.blue = firstWidget->getCoefBlue() * _ui->sbBlue->maximum();
		}
		firstPass = false;
	}

	onMonitor_currentIndexChanged(_ui->cbMonitorSelect->currentIndex());

	connect(_ui->sbRed, qOverload<int>(&QSpinBox::valueChanged), this, &GlobalColorCoefPage::onCoefValueChanged);
	connect(_ui->sbGreen, qOverload<int>(&QSpinBox::valueChanged), this, &GlobalColorCoefPage::onCoefValueChanged);
	connect(_ui->sbBlue, qOverload<int>(&QSpinBox::valueChanged), this, &GlobalColorCoefPage::onCoefValueChanged);
	connect(_ui->hsColorTemperature, &QSlider::valueChanged, this, &GlobalColorCoefPage::onColorTemperatureValueChanged);
	connect(_ui->cbMonitorSelect, qOverload<int>(&QComboBox::currentIndexChanged), this, &GlobalColorCoefPage::onMonitor_currentIndexChanged);

	turnLightsOn(qRgb(255, 255, 255));

	// prevent some firmwares/devices from timing out during this phase
	// also avoids tracking widget coef signals
	connect(&_keepAlive, &QTimer::timeout, this, &GlobalColorCoefPage::updateDevice);
	using namespace std::chrono_literals;
	_keepAlive.start(200ms);
}

bool GlobalColorCoefPage::validatePage()
{
	using namespace SettingsScope;
	const QString deviceName = device()->name();
	SupportedDevices::DeviceType devType;
	if (deviceName.compare(QStringLiteral("lightpack"), Qt::CaseInsensitive) == 0) {
		devType = SupportedDevices::DeviceTypeLightpack;
	}
	else if (deviceName.compare(QStringLiteral("adalight"), Qt::CaseInsensitive) == 0) {
		devType = SupportedDevices::DeviceTypeAdalight;
		Settings::setAdalightSerialPortName(field(QStringLiteral("serialPort")).toString());
		Settings::setAdalightSerialPortBaudRate(field(QStringLiteral("baudRate")).toString());
		Settings::setColorSequence(devType, field(QStringLiteral("colorFormat")).toString());

	}
	else if (deviceName.compare(QStringLiteral("ardulight"), Qt::CaseInsensitive) == 0) {
		devType = SupportedDevices::DeviceTypeArdulight;
		Settings::setArdulightSerialPortName(field(QStringLiteral("serialPort")).toString());
		Settings::setArdulightSerialPortBaudRate(field(QStringLiteral("baudRate")).toString());
		Settings::setColorSequence(devType, field(QStringLiteral("colorFormat")).toString());

	}
	else if (deviceName.compare(QStringLiteral("drgb"), Qt::CaseInsensitive) == 0) {
		devType = SupportedDevices::DeviceTypeDrgb;
		Settings::setDrgbAddress(field(QStringLiteral("address")).toString());
		Settings::setDrgbPort(field(QStringLiteral("port")).toString());
		Settings::setDrgbTimeout(field(QStringLiteral("timeout")).toInt());
	}
	else if (deviceName.compare(QStringLiteral("dnrgb"), Qt::CaseInsensitive) == 0) {
		devType = SupportedDevices::DeviceTypeDnrgb;
		Settings::setDnrgbAddress(field(QStringLiteral("address")).toString());
		Settings::setDnrgbPort(field(QStringLiteral("port")).toString());
		Settings::setDnrgbTimeout(field(QStringLiteral("timeout")).toInt());
	}
	else if (deviceName.compare(QStringLiteral("warls"), Qt::CaseInsensitive) == 0) {
		devType = SupportedDevices::DeviceTypeWarls;
		Settings::setWarlsAddress(field(QStringLiteral("address")).toString());
		Settings::setWarlsPort(field(QStringLiteral("port")).toString());
		Settings::setWarlsTimeout(field(QStringLiteral("timeout")).toInt());
	}
	else {
		devType = SupportedDevices::DeviceTypeVirtual;
	}
	Settings::setConnectedDevice(devType);
	Settings::setNumberOfLeds(devType, _transSettings->zonePositions.size());

	for (const GrabWidget * const widget : _grabAreas) {
		const int id = widget->getId();
		Settings::setLedPosition(id, _transSettings->zonePositions[id]);
		Settings::setLedSize(id, _transSettings->zoneSizes[id]);
		Settings::setLedEnabled(id, _transSettings->zoneEnabled[id]);
		Settings::setLedCoefRed(id, widget->getCoefRed());
		Settings::setLedCoefGreen(id, widget->getCoefGreen());
		Settings::setLedCoefBlue(id, widget->getCoefBlue());
	}

	cleanupPage();
	return true;
}

void GlobalColorCoefPage::cleanupPage()
{
	cleanupMonitors();
	cleanupGrabAreas();
}

void GlobalColorCoefPage::updateDevice()
{
	QList<WBAdjustment> wblist;
	wblist.reserve(_transSettings->ledCount);

	constexpr const WBAdjustment defaultWB;
	for (int i = 0; i < _transSettings->ledCount; ++i) {
		if (_grabAreas.contains(i))
			wblist.append(_grabAreas[i]->getCoefs());
		else
			wblist.append(defaultWB);
	}

	device()->updateWBAdjustments(wblist, true);
}

void GlobalColorCoefPage::onCoefValueChanged(int value)
{
	Q_UNUSED(value);

	WBAdjustment wba;
	wba.red = _ui->sbRed->value() / (double)_ui->sbRed->maximum();
	wba.green = _ui->sbGreen->value() / (double)_ui->sbGreen->maximum();
	wba.blue = _ui->sbBlue->value() / (double)_ui->sbBlue->maximum();

	MonitorSettings& settings = _screens[_ui->cbMonitorSelect->currentIndex()];
	for (GrabWidget * const widget : _grabAreas) {
		if (settings.screen->geometry().contains(widget->geometry().center()))
			widget->setCoefs(wba);
	}

	settings.red = _ui->sbRed->value();
	settings.green = _ui->sbGreen->value();
	settings.blue = _ui->sbBlue->value();
	settings.colorTemp = _ui->hsColorTemperature->value();
}

void GlobalColorCoefPage::onColorTemperatureValueChanged(int value)
{
	const StructRgb whitePoint = PrismatikMath::whitePoint(value);
	_ui->sbRed->setValue(whitePoint.r / 2.55);
	_ui->sbGreen->setValue(whitePoint.g / 2.55);
	_ui->sbBlue->setValue(whitePoint.b / 2.55);

	MonitorSettings& settings = _screens[_ui->cbMonitorSelect->currentIndex()];
	settings.red = _ui->sbRed->value();
	settings.green = _ui->sbGreen->value();
	settings.blue = _ui->sbBlue->value();
	settings.colorTemp = value;
}

void GlobalColorCoefPage::cleanupMonitors()
{
	qDeleteAll(_monitorForms);
	_monitorForms.clear();
}

void GlobalColorCoefPage::addGrabArea(const int id)
{
	GrabWidget* const zone = new GrabWidget(id, AllowCoefConfig);

	zone->move(_transSettings->zonePositions[id]);
	zone->resize(_transSettings->zoneSizes[id]);
	if (_isInitFromSettings) {
		zone->setCoefRed(SettingsScope::Settings::getLedCoefRed(id));
		zone->setCoefGreen(SettingsScope::Settings::getLedCoefGreen(id));
		zone->setCoefBlue(SettingsScope::Settings::getLedCoefBlue(id));
	}
	zone->setAreaEnabled(_transSettings->zoneEnabled[id]);
	zone->fillBackgroundWhite();
	zone->show();
	_grabAreas.insert(id, zone);
}

void GlobalColorCoefPage::cleanupGrabAreas()
{
	qDeleteAll(_grabAreas);
	_grabAreas.clear();
}

void GlobalColorCoefPage::onMonitor_currentIndexChanged(int idx)
{
	int i = 0;
	for (MonitorIdForm* const monitorId : _monitorForms)
		monitorId->setActive(idx == i++);

	const MonitorSettings& settings = _screens[idx];
	_ui->sbRed->blockSignals(true);
	_ui->hsRed->setValue(settings.red);
	_ui->sbRed->blockSignals(false);

	_ui->sbGreen->blockSignals(true);
	_ui->hsGreen->setValue(settings.green);
	_ui->sbGreen->blockSignals(false);

	_ui->sbBlue->blockSignals(true);
	_ui->hsBlue->setValue(settings.blue);
	_ui->sbBlue->blockSignals(false);

	_ui->hsColorTemperature->blockSignals(true);
	_ui->hsColorTemperature->setValue(settings.colorTemp);
	_ui->hsColorTemperature->blockSignals(false);
}

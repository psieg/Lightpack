/*
 * ConfigureDevicePage.cpp
 *
 *	Created on: 11/1/2013
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

#include <QDesktopWidget>
#include <QMessageBox>

#include "ConfigureDevicePage.hpp"
#include "ui_ConfigureDevicePage.h"
#include "Settings.hpp"
#include "LedDeviceArdulight.hpp"
#include "LedDeviceAdalight.hpp"
#include "LedDeviceVirtual.hpp"
#include "Wizard.hpp"

using namespace SettingsScope;

ConfigureDevicePage::ConfigureDevicePage(bool isInitFromSettings, TransientSettings *ts, QWidget *parent):
	QWizardPage(parent),
	SettingsAwareTrait(isInitFromSettings, ts),
	ui(new Ui::ConfigureDevicePage)
{
	ui->setupUi(this);
}

void ConfigureDevicePage::initializePage()
{
	ui->cbBaudRate->clear();
	// NOTE: This line emit's signal currentIndex_Changed()
	ui->cbBaudRate->addItems(Settings::getSupportedSerialPortBaudRates());
	int currentBaudRate = 0;
	QString currentSerialPort = NULL;
	QString currentColorSequence = NULL;
	if (field("isAdalight").toBool()) {
		currentBaudRate = Settings::getAdalightSerialPortBaudRate();
		currentSerialPort = Settings::getAdalightSerialPortName();
		currentColorSequence = Settings::getColorSequence(SupportedDevices::DeviceTypeAdalight);
	}
	else if (field("isArdulight").toBool()) {
		currentBaudRate = Settings::getArdulightSerialPortBaudRate();
		currentSerialPort = Settings::getArdulightSerialPortName();
		currentColorSequence = Settings::getColorSequence(SupportedDevices::DeviceTypeArdulight);
	}
	else
		currentSerialPort = SERIAL_PORT_DEFAULT;

	if (currentBaudRate > 0)
		ui->cbBaudRate->setCurrentText(QString::number(currentBaudRate));
	if (currentSerialPort != NULL && currentSerialPort.isEmpty() == false)
		ui->leSerialPortName->setText(currentSerialPort);
	if (currentColorSequence != NULL && currentColorSequence.isEmpty() == false)
		ui->cbColorFormat->setCurrentText(currentColorSequence);

	registerField("serialPort", ui->leSerialPortName);
	registerField("baudRate", ui->cbBaudRate, "currentText");
	registerField("colorFormat", ui->cbColorFormat, "currentText");

}

void ConfigureDevicePage::cleanupPage()
{
	setField("serialPort", "");
	setField("baudRate", 0);
	setField("colorFormat", "RGB");
}

bool ConfigureDevicePage::validatePage()
{
	QString portName = field("serialPort").toString();
	int baudRate = field("baudRate").toInt();
	if (field("isAdalight").toBool()) {
		_transSettings->ledDevice.reset(new LedDeviceAdalight(portName, baudRate));
	} else if (field("isArdulight").toBool()){
		_transSettings->ledDevice.reset(new LedDeviceArdulight(portName, baudRate));
	} else {
		QMessageBox::information(NULL, "Wrong device", "Try to restart the wizard");
		qCritical() << "couldn't create LedDevice, unexpected state, device is not selected or device is not configurable";
		return false;
	}
	_transSettings->ledDevice->setColorSequence(field("colorFormat").toString());
	_transSettings->ledDevice->open();

	return true;
}

int ConfigureDevicePage::nextId() const {
	if (QApplication::desktop()->screenCount() == 1) {
		return reinterpret_cast<Wizard *>(wizard())->skipMonitorConfigurationPage();
	} else {
		return Page_MonitorConfiguration;
	}
}

ConfigureDevicePage::~ConfigureDevicePage()
{
	delete ui;
}

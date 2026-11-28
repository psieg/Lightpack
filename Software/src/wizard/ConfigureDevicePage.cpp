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

#include <QMessageBox>
#include <QCollator>
#include <QSerialPortInfo>
#include <algorithm>

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

	connect(ui->tbRefreshSerialPorts, &QToolButton::clicked, this, &ConfigureDevicePage::refreshSerialPorts);
	connect(ui->cbSerialPortName, &QComboBox::currentTextChanged, this, &ConfigureDevicePage::serialPortNameChanged);
}

QString ConfigureDevicePage::serialPortName() const
{
	const QComboBox *cb = ui->cbSerialPortName;
	const QString text = cb->currentText().trimmed();
	// A detected port is shown as "COM5 - USB-SERIAL CH340"; its port name is in the item data.
	// Anything else was typed by hand and is used as-is.
	const int idx = cb->findText(text);
	if (idx >= 0)
		return cb->itemData(idx).toString();
	return text;
}

void ConfigureDevicePage::setSerialPortName(const QString &portName)
{
	QComboBox *cb = ui->cbSerialPortName;
	const int idx = cb->findData(portName);
	if (idx >= 0) {
		cb->setCurrentIndex(idx);
	} else {
		cb->setCurrentIndex(-1);
		cb->setEditText(portName);
	}
}

void ConfigureDevicePage::refreshSerialPorts()
{
	populateSerialPorts(serialPortName(), false);
}

void ConfigureDevicePage::populateSerialPorts(const QString &preferredPort, const bool preferredIsDefault)
{
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

	// USB serial adapters (Arduino, CH340, FTDI, CP210x...) first, then by port name in natural order (COM2 < COM10)
	QCollator collator;
	collator.setNumericMode(true);
	collator.setCaseSensitivity(Qt::CaseInsensitive);
	std::sort(ports.begin(), ports.end(), [&collator](const QSerialPortInfo &a, const QSerialPortInfo &b) {
		if (a.hasVendorIdentifier() != b.hasVendorIdentifier())
			return a.hasVendorIdentifier();
		return collator.compare(a.portName(), b.portName()) < 0;
	});

	QComboBox *cb = ui->cbSerialPortName;
	const QSignalBlocker blocker(cb);
	cb->clear();

	int preferredIdx = -1;
	int firstUsbIdx = -1;
	for (const QSerialPortInfo &port : std::as_const(ports)) {
		const QString description = port.description().isEmpty() ? port.manufacturer() : port.description();
		const QString label = description.isEmpty() ? port.portName() : port.portName() + QStringLiteral(" - ") + description;

		QStringList tooltip;
		tooltip << port.systemLocation();
		if (!port.manufacturer().isEmpty())
			tooltip << port.manufacturer();
		if (port.hasVendorIdentifier() && port.hasProductIdentifier())
			tooltip << QStringLiteral("VID:PID %1:%2")
				.arg(port.vendorIdentifier(), 4, 16, QLatin1Char('0'))
				.arg(port.productIdentifier(), 4, 16, QLatin1Char('0')).toUpper();

		const int idx = cb->count();
		cb->addItem(label, port.portName());
		cb->setItemData(idx, tooltip.join(QLatin1Char('\n')), Qt::ToolTipRole);

		if (preferredIdx < 0 && port.portName().compare(preferredPort, Qt::CaseInsensitive) == 0)
			preferredIdx = idx;
		if (firstUsbIdx < 0 && port.hasVendorIdentifier())
			firstUsbIdx = idx;
	}

	// Keep the configured port if it is connected, unless it is just the built-in default
	// and a USB serial adapter is present. If the configured port is gone (e.g. the device
	// was re-plugged and got a new number), pick the first USB serial adapter instead.
	if (preferredIdx >= 0 && !(preferredIsDefault && firstUsbIdx >= 0))
		cb->setCurrentIndex(preferredIdx);
	else if (firstUsbIdx >= 0)
		cb->setCurrentIndex(firstUsbIdx);
	else if (!preferredPort.isEmpty())
		setSerialPortName(preferredPort);
	else if (cb->count() > 0)
		cb->setCurrentIndex(0);

	emit serialPortNameChanged();
}

void ConfigureDevicePage::initializePage()
{
	ui->cbBaudRate->clear();
	// NOTE: This line emit's signal currentIndex_Changed()
	ui->cbBaudRate->addItems(Settings::getSupportedSerialPortBaudRates());
	int currentBaudRate = 0;
	QString currentSerialPort;
	QString currentColorSequence;
	if (field(QStringLiteral("isAdalight")).toBool()) {
		currentBaudRate = Settings::getAdalightSerialPortBaudRate();
		currentSerialPort = Settings::getAdalightSerialPortName();
		currentColorSequence = Settings::getColorSequence(SupportedDevices::DeviceTypeAdalight);
	}
	else if (field(QStringLiteral("isArdulight")).toBool()) {
		currentBaudRate = Settings::getArdulightSerialPortBaudRate();
		currentSerialPort = Settings::getArdulightSerialPortName();
		currentColorSequence = Settings::getColorSequence(SupportedDevices::DeviceTypeArdulight);
	}
	else
		currentSerialPort = QStringLiteral(SERIAL_PORT_DEFAULT);

	if (currentBaudRate > 0)
		ui->cbBaudRate->setCurrentText(QString::number(currentBaudRate));
	populateSerialPorts(currentSerialPort, currentSerialPort == QStringLiteral(SERIAL_PORT_DEFAULT));
	if (!currentColorSequence.isEmpty())
		ui->cbColorFormat->setCurrentText(currentColorSequence);

	registerField(QStringLiteral("serialPort"), this, "serialPortName", SIGNAL(serialPortNameChanged()));
	registerField(QStringLiteral("baudRate"), ui->cbBaudRate, "currentText");
	registerField(QStringLiteral("colorFormat"), ui->cbColorFormat, "currentText");

}

void ConfigureDevicePage::cleanupPage()
{
	setField(QStringLiteral("serialPort"), "");
	setField(QStringLiteral("baudRate"), 0);
	setField(QStringLiteral("colorFormat"), "RGB");
}

bool ConfigureDevicePage::validatePage()
{
	QString portName = field(QStringLiteral("serialPort")).toString();
	int baudRate = field(QStringLiteral("baudRate")).toInt();
	if (field(QStringLiteral("isAdalight")).toBool()) {
		_transSettings->ledDevice.reset(new LedDeviceAdalight(portName, baudRate));
	} else if (field(QStringLiteral("isArdulight")).toBool()){
		_transSettings->ledDevice.reset(new LedDeviceArdulight(portName, baudRate));
	} else {
		QMessageBox::information(NULL, QStringLiteral("Wrong device"), QStringLiteral("Try to restart the wizard"));
		qCritical() << "couldn't create LedDevice, unexpected state, device is not selected or device is not configurable";
		return false;
	}
	_transSettings->ledDevice->setColorSequence(field(QStringLiteral("colorFormat")).toString());
	_transSettings->ledDevice->open();

	return true;
}

ConfigureDevicePage::~ConfigureDevicePage()
{
	delete ui;
}

int ConfigureDevicePage::nextId() const
{
	// skip UDP device page
	return Page_ChooseProfile;
}

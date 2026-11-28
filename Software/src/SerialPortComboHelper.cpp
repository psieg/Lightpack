/*
 * SerialPortComboHelper.cpp
 *
 *	Project: Prismatik
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

#include "SerialPortComboHelper.hpp"

#include <QCollator>
#include <QComboBox>
#include <QSerialPortInfo>
#include <QSignalBlocker>
#include <QStringList>
#include <algorithm>

namespace SerialPortComboHelper
{

void populate(QComboBox *comboBox, const QString &preferredPort, const bool preferredIsDefault)
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

	const QSignalBlocker blocker(comboBox);
	comboBox->clear();

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

		const int idx = comboBox->count();
		comboBox->addItem(label, port.portName());
		comboBox->setItemData(idx, tooltip.join(QLatin1Char('\n')), Qt::ToolTipRole);

		if (preferredIdx < 0 && port.portName().compare(preferredPort, Qt::CaseInsensitive) == 0)
			preferredIdx = idx;
		if (firstUsbIdx < 0 && port.hasVendorIdentifier())
			firstUsbIdx = idx;
	}

	// Keep the configured port if it is connected, unless it is just the built-in default
	// and a USB serial adapter is present. If the configured port is gone (e.g. the device
	// was re-plugged and got a new number), pick the first USB serial adapter instead.
	if (preferredIdx >= 0 && !(preferredIsDefault && firstUsbIdx >= 0))
		comboBox->setCurrentIndex(preferredIdx);
	else if (firstUsbIdx >= 0)
		comboBox->setCurrentIndex(firstUsbIdx);
	else if (!preferredPort.isEmpty())
		selectPortName(comboBox, preferredPort);
	else if (comboBox->count() > 0)
		comboBox->setCurrentIndex(0);
}

QString selectedPortName(const QComboBox *comboBox)
{
	const QString text = comboBox->currentText().trimmed();
	// A detected port is shown as "COM5 - USB-SERIAL CH340"; its port name is in the item data.
	// Anything else was typed by hand and is used as-is.
	const int idx = comboBox->findText(text);
	if (idx >= 0)
		return comboBox->itemData(idx).toString();
	return text;
}

void selectPortName(QComboBox *comboBox, const QString &portName)
{
	const int idx = comboBox->findData(portName);
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	} else {
		comboBox->setCurrentIndex(-1);
		comboBox->setEditText(portName);
	}
}

} // namespace SerialPortComboHelper

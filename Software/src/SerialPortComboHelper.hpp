/*
 * SerialPortComboHelper.hpp
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

#pragma once

#include <QString>

class QComboBox;

// Fills an (editable) QComboBox with the detected serial ports, labelled with
// their description (e.g. "COM5 - USB-SERIAL CH340"), keeping the bare port
// name in the item data. A port name typed by hand is also accepted.
namespace SerialPortComboHelper
{
	// Rescans the ports and selects one: the preferred port if it is connected
	// (unless it is only the built-in default and a USB serial adapter is present),
	// otherwise the first USB serial adapter, otherwise the preferred port as text.
	// Emits no signals from the combo box.
	void populate(QComboBox *comboBox, const QString &preferredPort, bool preferredIsDefault);

	// The port name for the current selection or typed text.
	QString selectedPortName(const QComboBox *comboBox);

	// Selects the item for portName, or shows it as typed text if not detected.
	void selectPortName(QComboBox *comboBox, const QString &portName);
}

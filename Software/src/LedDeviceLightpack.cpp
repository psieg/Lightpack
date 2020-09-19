/*
 * LightpackDevice.cpp
 *
 *	Created on: 26.07.2010
 *		Author: Mike Shatohin (brunql)
 *		Project: Lightpack
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
 *
 *	Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *	Lightpack is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Lightpack is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "LedDeviceLightpack.hpp"

#include <algorithm>
#include <QtDebug>
#include "debug.h"
#include "Settings.hpp"
#include <QApplication>

using namespace SettingsScope;

const int LedDeviceLightpack::kPingDeviceInterval = 1000;
const int LedDeviceLightpack::kLedsPerDevice = 10;

LedDeviceLightpack::LedDeviceLightpack(QObject *parent) :
	AbstractLedDevice(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "thread id: " << this->thread()->currentThreadId();

//	m_hidDevice = NULL;

	memset(m_writeBuffer, 0, sizeof(m_writeBuffer));
	memset(m_readBuffer, 0, sizeof(m_readBuffer));

	m_timerPingDevice = new QTimer(this);

	connect(m_timerPingDevice, SIGNAL(timeout()), this, SLOT(timerPingDeviceTimeout()));
	connect(this, SIGNAL(ioDeviceSuccess(bool)), this, SLOT(restartPingDevice(bool)));
	connect(this, SIGNAL(openDeviceSuccess(bool)), this, SLOT(restartPingDevice(bool)));

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "initialized";
}

LedDeviceLightpack::~LedDeviceLightpack()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "hid_close(...);";
	closeDevices();
}

void LedDeviceLightpack::setColors(const QList<QRgb> & colors)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	DEBUG_MID_LEVEL << Q_FUNC_INFO << Qt::hex << (colors.isEmpty() ? -1 : colors.first());
#else
	DEBUG_MID_LEVEL << Q_FUNC_INFO << hex << (colors.isEmpty() ? -1 : colors.first());
#endif

#if 0
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "thread id: " << this->thread()->currentThreadId();
#endif
	if (m_devices.size() == 0) {
		return;
	}

	if (static_cast<size_t>(colors.count()) > maxLedsCount()) {
		qWarning() << Q_FUNC_INFO << "data size is greater than max leds count";
		// skip command with wrong data size
		return;
	}

//	QMutexLocker locker(&getLightpackApp()->m_mutex);

	resizeColorsBuffer(colors.count());

	// Save colors for showing changes of the brightness
	m_colorsSaved = colors;

	applyColorModifications(colors, m_colorsBuffer);
	applyDithering(m_colorsBuffer, 12);

	// First write_buffer[0] == 0x00 - ReportID, i have problems with using it
	// Second byte of usb buffer is command (write_buffer[1] == CMD_UPDATE_LEDS, see below)
	int buffIndex = WRITE_BUFFER_INDEX_DATA_START;

	bool ok = true;
	const int kLedRemap[] = {4, 3, 0, 1, 2, 5, 6, 7, 8, 9};
	const size_t kSizeOfLedColor = 6;

	memset(m_writeBuffer, 0, sizeof(m_writeBuffer));
	for (int i = 0; i < m_colorsBuffer.count(); i++)
	{
		StructRgb color = m_colorsBuffer[i];

		buffIndex = WRITE_BUFFER_INDEX_DATA_START + kLedRemap[i % 10] * kSizeOfLedColor;

		// Send main 8 bits for compability with existing devices
		m_writeBuffer[buffIndex++] = (color.r & 0x0FF0) >> 4;
		m_writeBuffer[buffIndex++] = (color.g & 0x0FF0) >> 4;
		m_writeBuffer[buffIndex++] = (color.b & 0x0FF0) >> 4;

		// Send over 4 bits for devices revision >= 6
		// All existing devices ignore it
		m_writeBuffer[buffIndex++] = (color.r & 0x000F);
		m_writeBuffer[buffIndex++] = (color.g & 0x000F);
		m_writeBuffer[buffIndex++] = (color.b & 0x000F);

		if ((i+1) % kLedsPerDevice == 0 || i == m_colorsBuffer.size() - 1) {
			if (m_devices.empty() || !writeBufferToDeviceWithCheck(CMD_UPDATE_LEDS, m_devices[(i+kLedsPerDevice)/kLedsPerDevice - 1])) {
				ok = false;
				break;
			}
			memset(m_writeBuffer, 0, sizeof(m_writeBuffer));
			buffIndex = WRITE_BUFFER_INDEX_DATA_START;
		}
	}

//	locker.unlock();


	// WARNING: LedDeviceManager sends data only when the arrival of this signal
	emit commandCompleted(ok);
}

int LedDeviceLightpack::maxLedsCount()
{
	if (m_devices.size() == 0)
		tryToReopenDevice();
	return m_devices.size() * kLedsPerDevice;
}
void LedDeviceLightpack::switchOffLeds()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	if (m_colorsSaved.empty())
	{
		for (size_t i = 0; i < maxLedsCount(); ++i)
			m_colorsSaved << 0;
	} else {
		for (int i = 0; i < m_colorsSaved.count(); i++)
			m_colorsSaved[i] = 0;
	}

	m_timerPingDevice->stop();

	memset(m_writeBuffer, 0, sizeof(m_writeBuffer));

	bool ok = true;
	for(int i = 0; i < m_devices.size(); i++) {
		if (!writeBufferToDeviceWithCheck(CMD_UPDATE_LEDS, m_devices[i]))
			ok = false;
	}


	emit commandCompleted(ok);
	// Stop ping device if switchOffLeds() signal comes
}

void LedDeviceLightpack::setRefreshDelay(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	m_writeBuffer[WRITE_BUFFER_INDEX_DATA_START] = value & 0xff;
	m_writeBuffer[WRITE_BUFFER_INDEX_DATA_START+1] = (value >> 8);

	bool ok = true;
	for(int i = 0; i < m_devices.size(); i++) {
		if (!writeBufferToDeviceWithCheck(CMD_SET_TIMER_OPTIONS, m_devices[i]))
			ok = false;
	}
	emit commandCompleted(ok);
}

void LedDeviceLightpack::setUsbPowerLedDisabled(bool isDisabled) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << isDisabled;

	m_writeBuffer[WRITE_BUFFER_INDEX_DATA_START] = (unsigned char)!isDisabled;

	bool ok = true;
	for (int i = 0; i < m_devices.size(); ++i) {
		if (!writeBufferToDeviceWithCheck(CMD_UNOFFICIAL_SET_USBLED, m_devices[i]))
			ok = false;
	}
	emit commandCompleted(ok);
}

void LedDeviceLightpack::setColorDepth(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	m_writeBuffer[WRITE_BUFFER_INDEX_DATA_START] = (unsigned char)value;

	bool ok = true;
	for(int i = 0; i < m_devices.size(); i++) {
		if (!writeBufferToDeviceWithCheck(CMD_SET_PWM_LEVEL_MAX_VALUE, m_devices[i]))
			ok = false;
	}
	emit commandCompleted(ok);
}

void LedDeviceLightpack::setSmoothSlowdown(int value)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

	m_writeBuffer[WRITE_BUFFER_INDEX_DATA_START] = (unsigned char)value;

	bool ok = true;
	for(int i = 0; i < m_devices.size(); i++) {
		if (!writeBufferToDeviceWithCheck(CMD_SET_SMOOTH_SLOWDOWN, m_devices[i]))
			ok = false;
	}
	emit commandCompleted(ok);
}

void LedDeviceLightpack::setColorSequence(QString /*value*/)
{
	emit commandCompleted(true);
}

void LedDeviceLightpack::requestFirmwareVersion()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	QString fwVersion;

	bool ok = readDataFromDeviceWithCheck();

	// TODO: write command CMD_GET_VERSION to device
	int fw_unofficial = 0;
	if (ok)
	{
		int fw_major = m_readBuffer[INDEX_FW_VER_MAJOR];
		int fw_minor = m_readBuffer[INDEX_FW_VER_MINOR];
		fw_unofficial = m_readBuffer[INDEX_FW_VER_UNOFFICIAL];
		fwVersion = QString::number(fw_major) + "." + QString::number(fw_minor) + (fw_unofficial > 0 ? "+" + QString::number(fw_unofficial) : "");
	} else {
		fwVersion = tr("read device fail");
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Version:" << fwVersion;

	emit firmwareVersion(fwVersion);
	emit firmwareVersionUnofficial(fw_unofficial);
	emit commandCompleted(ok);
}

void LedDeviceLightpack::updateDeviceSettings()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << sender();

	AbstractLedDevice::updateDeviceSettings();
	setUsbPowerLedDisabled(Settings::isDeviceUsbPowerLedDisabled());
	setRefreshDelay(Settings::getDeviceRefreshDelay());
	setColorDepth(Settings::getDeviceColorDepth());
	setSmoothSlowdown(Settings::getDeviceSmooth());
}


void LedDeviceLightpack::open()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	if (m_devices.size() > 0)
	{
		emit openDeviceSuccess(true);
		return;
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << QString("hid_open(0x%1, 0x%2)")
						.arg(USB_VENDOR_ID, 4, 16, QChar('0'))
						.arg(USB_PRODUCT_ID, 4, 16, QChar('0'));

	open(USB_VENDOR_ID, USB_PRODUCT_ID);
	open(USB_OLD_VENDOR_ID, USB_OLD_PRODUCT_ID);

	if (m_devices.size() == 0)
	{
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Lightpack devices not found";
		emit openDeviceSuccess(false);
		return;
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Lightpack opened";

	updateDeviceSettings();

	emit openDeviceSuccess(true);
}

void LedDeviceLightpack::open(unsigned short vid, unsigned short pid)
{
	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device * handle = NULL;
	QMap<QString, hid_device*> map;
	QList<hid_device*> list;

	devs = hid_enumerate(vid, pid);
	cur_dev = devs;
	while (cur_dev) {
		path_to_open = cur_dev->path;
		if (path_to_open) {
			/* Open the device */
			handle = hid_open_path(path_to_open);

			if(handle != NULL) {

				// Immediately return from hid_read() if no data available
				hid_set_nonblocking(handle, 1);
				if(cur_dev->serial_number != NULL && wcslen(cur_dev->serial_number) > 0) {
					QString serialNum = QString::fromWCharArray(cur_dev->serial_number);
					DEBUG_LOW_LEVEL << "found Lightpack, serial number: " << serialNum;
					map.insert(serialNum, handle);
				} else {
					DEBUG_LOW_LEVEL << "found Lightpack, without serial number";
					list.append(handle);
				}
			} else {
				qCritical() << Q_FUNC_INFO << "couldn't open dev by path";
			}
		}
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
	m_devices.append(map.values());
	m_devices.append(list);
}

void LedDeviceLightpack::close() {
	closeDevices();
}

bool LedDeviceLightpack::readDataFromDevice()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	int bytes_read = hid_read(m_devices[0], m_readBuffer, sizeof(m_readBuffer));

	if(bytes_read < 0){
		qWarning() << "Error reading data:" << bytes_read;
		emit ioDeviceSuccess(false);
		return false;
	}
	emit ioDeviceSuccess(true);
	return true;
}

bool LedDeviceLightpack::writeBufferToDevice(int command, hid_device *phid_device)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << command;
#if 0
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "thread id: " << this->thread()->currentThreadId();
#endif

	m_writeBuffer[WRITE_BUFFER_INDEX_REPORT_ID] = 0x00;
	m_writeBuffer[WRITE_BUFFER_INDEX_COMMAND] = command;

	int error = hid_write(phid_device, m_writeBuffer, sizeof(m_writeBuffer));
	if (error < 0)
	{
		// Trying to repeat sending data:
		error = hid_write(phid_device, m_writeBuffer, sizeof(m_writeBuffer));
		if(error < 0){
			qWarning() << "Error writing data:" << error;
			emit ioDeviceSuccess(false);
			return false;
		}
	}
	emit ioDeviceSuccess(true);
	return true;
}

bool LedDeviceLightpack::tryToReopenDevice()
{
	closeDevices();
//	QThread::sleep(100);
	open();

	if (m_devices.size() == 0)
	{
		return false;
	}

	DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Reopen success";
	return true;
}

bool LedDeviceLightpack::readDataFromDeviceWithCheck()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	if (m_devices.size() > 0)
	{
		if (!readDataFromDevice())
		{
			if (tryToReopenDevice())
				return readDataFromDevice();
			else
				return false;
		}
		return true;
	} else {
		if (tryToReopenDevice())
			return readDataFromDevice();
		else
			return false;
	}
}

bool LedDeviceLightpack::writeBufferToDeviceWithCheck(int command, hid_device *phid_device)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;

	if (phid_device != NULL)
	{
		if (!writeBufferToDevice(command, phid_device))
		{
			if (!writeBufferToDevice(command, phid_device))
			{
				if (tryToReopenDevice())
					return writeBufferToDevice(command, phid_device);
				else
					return false;
			}
		}
		return true;
	} else {
		if (tryToReopenDevice())
			return writeBufferToDevice(command, phid_device);
		else
			return false;
	}
}

void LedDeviceLightpack::resizeColorsBuffer(int buffSize)
{
	if (m_colorsBuffer.count() == buffSize || buffSize < 0)
		return;

	m_colorsBuffer.clear();

	size_t checkedBufferSize = buffSize;
	if (checkedBufferSize > maxLedsCount())
	{
		qCritical() << Q_FUNC_INFO << "buffSize > MaximumLedsCount" << checkedBufferSize << ">" << maxLedsCount();

		checkedBufferSize = maxLedsCount();
	}

	std::fill_n(std::back_inserter(m_colorsBuffer), checkedBufferSize, StructRgb());
}

void LedDeviceLightpack::closeDevices()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	m_timerPingDevice->stop();
	m_timerPingDevice->blockSignals(true);

	for(int i=0; i < m_devices.size(); i++) {
		hid_close(m_devices[i]);
	}
	m_devices.clear();
}

void LedDeviceLightpack::restartPingDevice(bool isSuccess)
{
	Q_UNUSED(isSuccess);

	if (Settings::isBacklightEnabled() && Settings::isPingDeviceEverySecond())
	{
		// Start ping device with PingDeviceInterval ms after last data transfer complete
		m_timerPingDevice->start(kPingDeviceInterval);
	} else {
		m_timerPingDevice->stop();
	}
}

void LedDeviceLightpack::timerPingDeviceTimeout()
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	if (m_devices.size() == 0)
	{
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "open devices";
		open();

		if (m_devices.size() == 0)
		{
			DEBUG_MID_LEVEL << Q_FUNC_INFO << "open devices fail";
			emit openDeviceSuccess(false);
			return;
		}
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "open devices ok";

		emit openDeviceSuccess(true);
		closeDevices(); // device should be opened by open() function
		return;
	}

	DEBUG_MID_LEVEL << Q_FUNC_INFO << "hid_write";

	m_writeBuffer[WRITE_BUFFER_INDEX_REPORT_ID] = 0x00;
	m_writeBuffer[WRITE_BUFFER_INDEX_COMMAND] = CMD_NOP;
	int bytes = hid_write(m_devices[0], m_writeBuffer, sizeof(m_writeBuffer));

	if (bytes < 0)
	{
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "hid_write fail";
		closeDevices();
		emit ioDeviceSuccess(false);
		return;
	}

	DEBUG_MID_LEVEL << Q_FUNC_INFO << "hid_write ok";
	emit ioDeviceSuccess(true);
}

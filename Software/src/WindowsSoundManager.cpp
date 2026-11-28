/*
 * WindowsSoundManager.cpp
 *
 *	Created on: 11.12.2011
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *	Lightpack a USB content-driving ambient lighting system
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


#include "WindowsSoundManager.hpp"
#include "debug.h"
#include "bass.h"
#include "basswasapi.h"

namespace {
// Only inputs (microphones, line-in...) and loopback devices (what an output is playing)
// can be captured. Plain output endpoints are listed by BASSWASAPI too but are useless here.
bool isCaptureDevice(const BASS_WASAPI_DEVICEINFO &inf)
{
	return (inf.flags & BASS_DEVICE_ENABLED) && (inf.flags & BASS_DEVICE_INPUT);
}

// The loopback of the default output device, or the first enabled loopback device, or -1.
// Note: BASSWASAPI also lists disabled/unplugged endpoints Windows remembers (e.g. every
// monitor ever connected), so there can be hundreds of devices; enumerate until it fails.
int findRecommendedLoopbackDevice()
{
	QByteArray defaultOutputId;
	int firstLoopback = -1;
	BASS_WASAPI_DEVICEINFO inf;
	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &inf); i++) {
		if (!(inf.flags & BASS_DEVICE_ENABLED))
			continue;
		if (!(inf.flags & BASS_DEVICE_INPUT) && (inf.flags & BASS_DEVICE_DEFAULT))
			defaultOutputId = QByteArray(inf.id);
		if (inf.flags & BASS_DEVICE_LOOPBACK) {
			// A loopback device has the same endpoint id as its output and is listed after it
			if (!defaultOutputId.isEmpty() && defaultOutputId == QByteArray(inf.id))
				return i;
			if (firstLoopback < 0)
				firstLoopback = i;
		}
	}
	return firstLoopback;
}
} // namespace

WindowsSoundManager::WindowsSoundManager(int hWnd, QObject *parent) : SoundManagerBase(parent)
{
	m_hWnd = hWnd;

	m_timer.setTimerType(Qt::PreciseTimer);
	connect(&m_timer, &QTimer::timeout, this, &WindowsSoundManager::updateColors);
}

WindowsSoundManager::~WindowsSoundManager()
{
	if (m_isInited) BASS_Free();
}

bool WindowsSoundManager::init() {
	BASS_SetConfig(BASS_CONFIG_UNICODE, true);
	// initialize "no sound" BASS device
	if (!BASS_Init(0, 44100, 0, (HWND)IntToPtr(m_hWnd), NULL)) {
		qCritical() << Q_FUNC_INFO << "Can't initialize BASS" << BASS_ErrorGetCode();
		m_isInited = false;
		return false;
	}

	m_isInited = true;
	return true;
}

// WASAPI callback - not doing anything with the data
DWORD CALLBACK DuffRecording(void *buffer, DWORD length, void *user)
{
	Q_UNUSED(buffer);
	Q_UNUSED(length);
	Q_UNUSED(user);
	return TRUE; // continue recording
}

void WindowsSoundManager::populateDeviceList(QList<SoundManagerDeviceInfo>& devices, int& recommended)
{
	BASS_WASAPI_DEVICEINFO inf;
	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &inf); i++) {
		if (!isCaptureDevice(inf))
			continue;
		devices.append(SoundManagerDeviceInfo(inf.name, i));
		if (inf.flags & BASS_DEVICE_LOOPBACK)
			devices.last().name.append(tr(" (loopback)"));
	}
	recommended = findRecommendedLoopbackDevice();
}

void WindowsSoundManager::start(bool isEnabled)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << isEnabled;

	if (m_isEnabled == isEnabled)
		return;

	m_isEnabled = isEnabled;

	if (m_isEnabled)
	{
		if (!m_isInited)
		{
			if (!init()) {
				m_isEnabled = false;
				return;
			}
		}

		int device = -1;
		if (m_device != -1) {
			BASS_WASAPI_DEVICEINFO inf;
			if (!BASS_WASAPI_GetDeviceInfo(m_device, &inf)) {
				qWarning() << Q_FUNC_INFO << "Unable to get device info for" << m_device << "- falling back";
			} else if (!(inf.flags & BASS_DEVICE_ENABLED)) {
				qWarning() << Q_FUNC_INFO << "Selected device is not enabled - falling back";
			} else if (!(inf.flags & BASS_DEVICE_INPUT)) {
				qWarning() << Q_FUNC_INFO << "Selected device is an output, which can't be captured - falling back";
			} else {
				device = m_device;
			}
		}
		if (device == -1) {
			// No usable saved device: the default output's loopback (recommended)
			device = findRecommendedLoopbackDevice();
			if (device == -1) {
				qWarning() << Q_FUNC_INFO << "No loopback device found, using the default input device";
				device = -2; // BASSWASAPI: default input device
			} else {
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Falling back to loopback device" << device;
			}
		}

		// initialize input WASAPI device
		if (!BASS_WASAPI_Init(device, 0, 0, BASS_WASAPI_BUFFER, 1, 0.1f, &DuffRecording, NULL)) {
			qCritical() << Q_FUNC_INFO << "Can't initialize WASAPI device" << BASS_ErrorGetCode();
			return;
		}

		BASS_WASAPI_Start();
		// setup update timer (40hz)
		//m_timer = timeSetEvent(25, 25, (LPTIMECALLBACK)&UpdateSpectrum, 0, TIME_PERIODIC);
		m_timer.start(25);
	}
	else
	{
		m_timer.stop();
		BASS_WASAPI_Free();
	}

	if (m_visualizer == nullptr)
		return;
	if (m_isEnabled)
		m_visualizer->start();
	else
		m_visualizer->stop();
}

void WindowsSoundManager::updateFft()
{
	// get the FFT data
	if (fftSize() == 1024)
		BASS_WASAPI_GetData(m_fft, BASS_DATA_FFT2048); 
	else if (fftSize() == 2048)
		BASS_WASAPI_GetData(m_fft, BASS_DATA_FFT4096);
}

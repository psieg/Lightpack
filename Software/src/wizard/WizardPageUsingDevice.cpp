/*
* ZoneConfiguration.cpp
*
*	Created on: 16.2.2017
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

#include "WizardPageUsingDevice.hpp"
#include "AbstractLedDevice.hpp"
#include "SettingsDefaults.hpp"
#include "types.h"


WizardPageUsingDevice::WizardPageUsingDevice(bool isInitFromSettings, TransientSettings *ts, QWidget *parent) :
QWizardPage(parent),
SettingsAwareTrait(isInitFromSettings, ts)
{
}

WizardPageUsingDevice::~WizardPageUsingDevice()
{
}

void WizardPageUsingDevice::resetDeviceSettings()
{

	QList<WBAdjustment> adjustments;
	const int numOfLeds = device()->maxLedsCount();

	for (int led = 0; led < numOfLeds; ++led) {
		WBAdjustment wba;
		wba.red = 1.0;
		wba.green = 1.0;
		wba.blue = 1.0;
		adjustments.append(wba);
	}

	device()->updateWBAdjustments(adjustments);
	device()->setBrightness(SettingsScope::Profile::Device::BrightnessDefault);
	device()->setSmoothSlowdown(SettingsScope::Profile::Device::SmoothDefault);
	device()->setGamma(SettingsScope::Profile::Device::GammaDefault);
	device()->setMinimumLuminosityThresholdEnabled(false);


}

AbstractLedDevice * WizardPageUsingDevice::device()
{
	return _transSettings->ledDevice.data();
}

void WizardPageUsingDevice::turnLightOn(int id)
{
	QList<QRgba64> lights;
	for (int i = 0; i < _transSettings->ledCount; i++)
	{
		if (i == id)
			lights.append(qRgba64(65535, 65535, 65535, 0));
		else
			lights.append(qRgba64(0));
	}
	device()->setColors(lights);
}

void WizardPageUsingDevice::turnLightsOn(QRgba64 color)
{
	QList<QRgba64> lights;
	for (int i = 0; i < _transSettings->ledCount; i++)
	{
		lights.append(color);
	}
	device()->setColors(lights);
}

void WizardPageUsingDevice::turnLightsOff()
{
	QList<QRgba64> lights;
	for (int i = 0; i < _transSettings->ledCount; i++)
	{
		lights.append(qRgba64(0));
	}
	device()->setColors(lights);
}
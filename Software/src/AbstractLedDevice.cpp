/*
 * AbstractLedDevice.cpp
 *
 *	Created on: 05.02.2013
 *		Project: Prismatik
 *
 *	Copyright (c) 2013 Timur Sattarov, tim.helloworld [at] gmail.com
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

#include "AbstractLedDevice.hpp"
#include "colorspace_types.h"
#include "PrismatikMath.hpp"
#include "Settings.hpp"

void AbstractLedDevice::setGamma(double value, bool updateColors) {
	m_gamma = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::setBrightness(int value, bool updateColors) {
	m_brightness = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::setBrightnessCap(int value, bool updateColors) {
	m_brightnessCap = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::setLedMilliAmps(const int value, const bool updateColors) {
	m_ledMilliAmps = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::setPowerSupplyAmps(const double value, const bool updateColors) {
	m_powerSupplyAmps = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::setLuminosityThreshold(int value, bool updateColors) {
	m_luminosityThreshold = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::setMinimumLuminosityThresholdEnabled(bool value, bool updateColors) {
	m_isMinimumLuminosityEnabled = value;
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::updateWBAdjustments() {
	updateWBAdjustments(SettingsScope::Settings::getLedCoefs());
}

void AbstractLedDevice::updateWBAdjustments(const QList<WBAdjustment> &coefs, bool updateColors) {
	m_wbAdjustments.clear();
	m_wbAdjustments.append(coefs);
	if (updateColors)
		setColors(m_colorsSaved);
}

void AbstractLedDevice::updateDeviceSettings()
{
	using namespace SettingsScope;
	setGamma(Settings::getDeviceGamma(), false);
	setBrightness(Settings::getDeviceBrightness(), false);
	setBrightnessCap(Settings::getDeviceBrightnessCap(), false);
	setLuminosityThreshold(Settings::getLuminosityThreshold(), false);
	setMinimumLuminosityThresholdEnabled(Settings::isMinimumLuminosityEnabled(), false);
	updateWBAdjustments(Settings::getLedCoefs(), false);

	setColors(m_colorsSaved);
}

/*!
	Modifies colors according to gamma, luminosity threshold, white balance and brightness settings
	All modifications are made over extended 12bit RGB, so \code outColors \endcode will contain 12bit
	RGB instead of 8bit.
*/
void AbstractLedDevice::applyColorModifications(const QList<QRgb> &inColors, QList<StructRgb> &outColors) {

	const bool isApplyWBAdjustments = m_wbAdjustments.count() == inColors.count();

	for(int i = 0; i < inColors.count(); i++) {

		//renormalize to 12bit
		const constexpr double k = 4095/255.0;
		outColors[i].r = qRed(inColors[i]) * k;
		outColors[i].g = qGreen(inColors[i]) * k;
		outColors[i].b = qBlue(inColors[i]) * k;

		PrismatikMath::gammaCorrection(m_gamma, outColors[i]);
	}

	const StructLab avgColor = PrismatikMath::toLab(PrismatikMath::avgColor(outColors));

	const double ampCoef = m_ledMilliAmps / (4095.0 * 3.0) / 1000.0;
	double estimatedTotalAmps = 0.0;

	for (int i = 0; i < outColors.count(); ++i) {
		StructLab lab = PrismatikMath::toLab(outColors[i]);
		const int dl = m_luminosityThreshold - lab.l;
		if (dl > 0) {
			if (m_isMinimumLuminosityEnabled) { // apply minimum luminosity or dead-zone
				// Cross-fade a and b channels to avarage value within kFadingRange, fadingFactor = (dL - fadingRange)^2 / (fadingRange^2)
				constexpr int kFadingRange = 5;
				const double fadingCoeff = dl < kFadingRange ? (dl - kFadingRange)*(dl - kFadingRange)/(kFadingRange*kFadingRange): 1;
				const char da = avgColor.a - lab.a;
				const char db = avgColor.b - lab.b;
				lab.l = m_luminosityThreshold;
				lab.a += PrismatikMath::round(da * fadingCoeff);
				lab.b += PrismatikMath::round(db * fadingCoeff);
				const StructRgb rgb = PrismatikMath::toRgb(lab);
				outColors[i] = rgb;
			} else {
				outColors[i].r = 0;
				outColors[i].g = 0;
				outColors[i].b = 0;
			}
		}

		PrismatikMath::brightnessCorrection(m_brightness, outColors[i]);

		if (isApplyWBAdjustments) {
			outColors[i].r *= m_wbAdjustments[i].red;
			outColors[i].g *= m_wbAdjustments[i].green;
			outColors[i].b *= m_wbAdjustments[i].blue;
		}
		if (m_brightnessCap < SettingsScope::Profile::Device::BrightnessCapMax) {
			const double bcapFactor = (m_brightnessCap / 100.0 * 4095 * 3) / (outColors[i].r + outColors[i].g + outColors[i].b);
			if (bcapFactor < 1.0) {
				outColors[i].r *= bcapFactor;
				outColors[i].g *= bcapFactor;
				outColors[i].b *= bcapFactor;
			}
		}

		estimatedTotalAmps += ((double)outColors[i].r + (double)outColors[i].g + (double)outColors[i].b) * ampCoef;
	}

	if (m_powerSupplyAmps > 0.0 && m_powerSupplyAmps < estimatedTotalAmps) {
		const double powerRatio = m_powerSupplyAmps / estimatedTotalAmps;
		for (StructRgb& color : outColors) {
			color.r *= powerRatio;
			color.g *= powerRatio;
			color.b *= powerRatio;
		}
	}
}

void AbstractLedDevice::applyDithering(QList<StructRgb>& colors, int colorDepth)
{
	unsigned int maxColorValueIn = 4095;
	unsigned int maxColorValueOut = (1 << colorDepth) - 1;

	double k = (double)maxColorValueIn / (double)maxColorValueOut;

	double carryR = 0;
	double carryG = 0;
	double carryB = 0;

	double meanIndR = 0;
	double meanIndG = 0;
	double meanIndB = 0;

	for (double i = 0; i < colors.count(); ++i) {
		double tempR = colors[i].r;
		double tempG = colors[i].g;
		double tempB = colors[i].b;

		colors[i].r = (double)colors[i].r / k;
		colors[i].g = (double)colors[i].g / k;
		colors[i].b = (double)colors[i].b / k;

		carryR += tempR - (double)colors[i].r * k;
		carryG += tempG - (double)colors[i].g * k;
		carryB += tempB - (double)colors[i].b * k;

		meanIndR += (tempR - (double)colors[i].r * k) * i;
		meanIndG += (tempG - (double)colors[i].g * k) * i;
		meanIndB += (tempB - (double)colors[i].b * k) * i;

		if (carryR >= k) {
			int ind = (meanIndR - (carryR - k) * i) / k;
			colors[ind].r++;
			carryR -= k;
			meanIndR = carryR * i;
		}
		if (carryG >= k) {
			int ind = (meanIndG - (carryG - k) * i) / k;
			colors[ind].g++;
			carryG -= k;
			meanIndG = carryG * i;
		}
		if (carryB >= k) {
			int ind = (meanIndB - (carryB - k) * i) / k;
			colors[ind].b++;
			carryB -= k;
			meanIndB = carryB * i;
		}
	}
}

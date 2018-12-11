/*
 * calculations.cpp
 *
 *	Created on: 23.07.2012
 *		Project: Lightpack
 *
 *	Copyright (c) 2012 Timur Sattarov
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

#include "calculations.hpp"

#define PIXEL_FORMAT_ARGB 2,1,0 // channel positions in a 4 byte color
#define PIXEL_FORMAT_ABGR 0,1,2
#define PIXEL_FORMAT_RGBA 3,2,1
#define PIXEL_FORMAT_BGRA 1,2,3

#define PIXEL_INDEX(_channelOffset_,_position_) (index + _channelOffset_ + (bytesPerPixel * _position_))
#define PIXEL_CHANNEL(_channel_,_position_) (buffer[PIXEL_INDEX(offset##_channel_,_position_)])
#define PIXEL_R(_position_) (PIXEL_CHANNEL(R,_position_))
#define PIXEL_G(_position_) (PIXEL_CHANNEL(G,_position_))
#define PIXEL_B(_position_) (PIXEL_CHANNEL(B,_position_))

namespace {
	const uint8_t bytesPerPixel = 4;
	const uint8_t pixelsPerStep = 4;

	struct ColorValue {
		int r, g, b;
	};

	template<uint8_t offsetR, uint8_t offsetG, uint8_t offsetB>
	static const unsigned int accumulateBuffer(
		const unsigned char *buffer,
		const unsigned int pitch,
		const QRect &rect,
		ColorValue *resultColor) {
		register unsigned int r = 0, g = 0, b = 0;
		unsigned int count = 0; // count the amount of pixels taken into account
		for (int currentY = 0; currentY < rect.height(); currentY++) {
			int index = pitch * (rect.y() + currentY) + rect.x()*bytesPerPixel;
			for (int currentX = 0; currentX < rect.width(); currentX += pixelsPerStep) {
				r += PIXEL_R(0) + PIXEL_R(1) + PIXEL_R(2) + PIXEL_R(3);
				g += PIXEL_G(0) + PIXEL_G(1) + PIXEL_G(2) + PIXEL_G(3);
				b += PIXEL_B(0) + PIXEL_B(1) + PIXEL_B(2) + PIXEL_B(3);
				count += pixelsPerStep;
				index += bytesPerPixel * pixelsPerStep;
			}
		}

		resultColor->r = r;
		resultColor->g = g;
		resultColor->b = b;
		return count;
	}
} // namespace

namespace Grab {
	namespace Calculations {
		QRgb calculateAvgColor(QRgb *result, const unsigned char *buffer, BufferFormat bufferFormat, unsigned int pitch, const QRect &rect) {

			Q_ASSERT_X(rect.width() % pixelsPerStep == 0, "average color calculation", "rect width should be aligned by 4 bytes");

			unsigned int count = 0; // count the amount of pixels taken into account
			ColorValue color = {0, 0, 0};

			switch(bufferFormat) {
			case BufferFormatArgb:
				count = accumulateBuffer<PIXEL_FORMAT_ARGB>(buffer, pitch, rect, &color);
				break;

			case BufferFormatAbgr:
				count = accumulateBuffer<PIXEL_FORMAT_ABGR>(buffer, pitch, rect, &color);
				break;

			case BufferFormatRgba:
				count = accumulateBuffer<PIXEL_FORMAT_RGBA>(buffer, pitch, rect, &color);
				break;

			case BufferFormatBgra:
				count = accumulateBuffer<PIXEL_FORMAT_BGRA>(buffer, pitch, rect, &color);
				break;
			default:
				return -1;
				break;
			}

			if ( count > 1 ) {
				color.r = (color.r / count) & 0xff;
				color.g = (color.g / count) & 0xff;
				color.b = (color.b / count) & 0xff;
			}

			*result = qRgb(color.r, color.g, color.b);
			return *result;
		}

		QRgb calculateAvgColor(QList<QRgb> *colors) {
			int r=0, g=0, b=0;
			const int size = colors->size();
			for(int i=0; i < size; i++) {
				const QRgb rgb = colors->at(i);
				r += qRed(rgb);
				g += qGreen(rgb);
				b += qBlue(rgb);
			}
			r = r / size;
			g = g / size;
			b = b / size;
			return qRgb(r, g, b);
		}
	}
}

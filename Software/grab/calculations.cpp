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
#include <stdint.h>
#include <immintrin.h>

#define PIXEL_FORMAT_ARGB 2,1,0 // channel positions in a 4 byte color
#define PIXEL_FORMAT_ABGR 0,1,2
#define PIXEL_FORMAT_RGBA 3,2,1
#define PIXEL_FORMAT_BGRA 1,2,3


namespace {
	constexpr const uint8_t bytesPerPixel = 4;
	constexpr const uint8_t pixelsPerStep = 4;

	struct ColorValue {
		int r, g, b;
	};


#define PIXEL_INDEX(_channelOffset_,_position_) (index + _channelOffset_ + (bytesPerPixel * _position_))
#define PIXEL_CHANNEL(_channel_,_position_) (buffer[PIXEL_INDEX(offset##_channel_,_position_)])
#define PIXEL_R(_position_) (PIXEL_CHANNEL(R,_position_))
#define PIXEL_G(_position_) (PIXEL_CHANNEL(G,_position_))
#define PIXEL_B(_position_) (PIXEL_CHANNEL(B,_position_))

	template<uint8_t offsetR, uint8_t offsetG, uint8_t offsetB>
	static ColorValue accumulateBuffer(
		const int* const buff,
		const size_t pitch,
		const QRect& rect) {
		const unsigned char* const buffer = (const unsigned char* const)buff;

		ColorValue color{0,0,0};

		const int delta = rect.width() % pixelsPerStep;
		if (rect.width() >= pixelsPerStep)
			for (int currentY = 0; currentY < rect.height(); currentY++) {
				for (int currentX = 0; currentX < rect.width() - delta; currentX += pixelsPerStep) {
					const size_t index = pitch * bytesPerPixel * (rect.y() + currentY) + (rect.x() + currentX) * bytesPerPixel;
					color.r += PIXEL_R(0) + PIXEL_R(1) + PIXEL_R(2) + PIXEL_R(3);
					color.g += PIXEL_G(0) + PIXEL_G(1) + PIXEL_G(2) + PIXEL_G(3);
					color.b += PIXEL_B(0) + PIXEL_B(1) + PIXEL_B(2) + PIXEL_B(3);
				}
			}
		for (int currentX = rect.width() - delta; currentX < rect.width(); ++currentX) {
			for (int currentY = 0; currentY < rect.height(); ++currentY) {
				const size_t index = pitch * bytesPerPixel * (rect.y() + currentY) + (rect.x() + currentX) * bytesPerPixel;
				color.r += PIXEL_R(0);
				color.g += PIXEL_G(0);
				color.b += PIXEL_B(0);
			}
		}
		const size_t count = rect.height() * rect.width();
		color.r = (color.r / count) & 0xff;
		color.g = (color.g / count) & 0xff;
		color.b = (color.b / count) & 0xff;
		return color;
	};

	template<uint8_t offsetR, uint8_t offsetG, uint8_t offsetB>
	static ColorValue accumulateBuffer128(
		const int * const buffer,
		const size_t pitch,
		const QRect& rect) {

		__m128i sum[bytesPerPixel] = {
			_mm_setzero_si128(),
			_mm_setzero_si128(),
			_mm_setzero_si128(),
			_mm_setzero_si128()
		};

		// masks to re-arrange ARGB into 000A, 000R...
		// without doing right shift (ARGB >> 2*8 => 00AR) and applying AND mask (00AR & 000F => 000R)
		// to isolate color components
		constexpr const char zero = (char)(1<<7);
		const __m128i shuffleR = _mm_set_epi8(
			zero,zero,zero,3*4+offsetR,
			zero,zero,zero,2*4+offsetR,
			zero,zero,zero,1*4+offsetR,
			zero,zero,zero,0*4+offsetR
		);
		const __m128i shuffleG = _mm_set_epi8(
			zero,zero,zero,3*4+offsetG,
			zero,zero,zero,2*4+offsetG,
			zero,zero,zero,1*4+offsetG,
			zero,zero,zero,0*4+offsetG
		);
		const __m128i shuffleB = _mm_set_epi8(
			zero,zero,zero,3*4+offsetB,
			zero,zero,zero,2*4+offsetB,
			zero,zero,zero,1*4+offsetB,
			zero,zero,zero,0*4+offsetB
		);
		const size_t softlimit = rect.width() / pixelsPerStep;
		if (softlimit > 0)
			for (size_t currentY = 0; currentY < (size_t)rect.height(); ++currentY) {
				for (size_t currentX = 0; currentX < softlimit; ++currentX) {
					const size_t index = pitch * (rect.y() + currentY) + rect.x() + currentX * pixelsPerStep;
					// (AARRGGBB AARRGGBB AARRGGBB AARRGGBB)
					const __m128i vec4 = _mm_loadu_si128((const __m128i*)&buffer[index]);

					//   (AARRGGBB AARRGGBB AARRGGBB AARRGGBB) shuffleR
					// = (000000RR 000000RR 000000RR 000000RR)
					sum[offsetR] = _mm_add_epi32(sum[offsetR], _mm_shuffle_epi8(vec4, shuffleR));
					sum[offsetG] = _mm_add_epi32(sum[offsetG], _mm_shuffle_epi8(vec4, shuffleG));
					sum[offsetB] = _mm_add_epi32(sum[offsetB], _mm_shuffle_epi8(vec4, shuffleB));
				}
			}
		//   ((BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB) + (GGGGGGGG GGGGGGGG GGGGGGGG GGGGGGGG))
		// + ((RRRRRRRR RRRRRRRR RRRRRRRR RRRRRRRR) + (AAAAAAAA AAAAAAAA AAAAAAAA AAAAAAAA))
		// = ((GGGGGGGG GGGGGGGG BBBBBBBB BBBBBBBB) + (AAAAAAAA AAAAAAAA RRRRRRRR RRRRRRRR))
		// =  (AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB)
		const __m128i horizontalSum128 = _mm_hadd_epi32(_mm_hadd_epi32(sum[0], sum[1]), _mm_hadd_epi32(sum[2], sum[3]));
		const size_t count = rect.height() * rect.width();

		ColorValue color{0,0,0};

		const int delta = rect.width() % pixelsPerStep;
		for (int currentX = rect.width() - delta; currentX < rect.width(); ++currentX) {
			for (int currentY = 0; currentY < rect.height(); ++currentY) {
				const size_t index = pitch * (rect.y() + currentY) + (rect.x() + currentX);
				color.r += ((const unsigned char* const)&buffer[index])[offsetR];
				color.g += ((const unsigned char* const)&buffer[index])[offsetG];
				color.b += ((const unsigned char* const)&buffer[index])[offsetB];
			}
		}
		color.r = ((color.r + _mm_extract_epi32(horizontalSum128, offsetR)) / count) & 0xff;
		color.g = ((color.g + _mm_extract_epi32(horizontalSum128, offsetG)) / count) & 0xff;
		color.b = ((color.b + _mm_extract_epi32(horizontalSum128, offsetB)) / count) & 0xff;
		return color;
	};

	template<uint8_t offsetR, uint8_t offsetG, uint8_t offsetB>
	static ColorValue accumulateBuffer256(
		const int * const buffer,
		const size_t pitch,
		const QRect& rect) {

		__m256i sum[bytesPerPixel] = {
			_mm256_setzero_si256(),
			_mm256_setzero_si256(),
			_mm256_setzero_si256(),
			_mm256_setzero_si256()
		}; // A,R,G,B sums

		constexpr const char zero = (char)(1<<7);
		const __m256i shuffleR = _mm256_broadcastsi128_si256(_mm_set_epi8(
			zero,zero,zero,3*4+offsetR,
			zero,zero,zero,2*4+offsetR,
			zero,zero,zero,1*4+offsetR,
			zero,zero,zero,0*4+offsetR
		));
		const __m256i shuffleG = _mm256_broadcastsi128_si256(_mm_set_epi8(
			zero,zero,zero,3*4+offsetG,
			zero,zero,zero,2*4+offsetG,
			zero,zero,zero,1*4+offsetG,
			zero,zero,zero,0*4+offsetG
		));
		const __m256i shuffleB = _mm256_broadcastsi128_si256(_mm_set_epi8(
			zero,zero,zero,3*4+offsetB,
			zero,zero,zero,2*4+offsetB,
			zero,zero,zero,1*4+offsetB,
			zero,zero,zero,0*4+offsetB
		));
		// 2 part processing:
		//   1) inner rect with multiple-of-8 width so we can do full 8px loads all the way
		const size_t softlimit = rect.width() / pixelsPerStep / 2;
		if (softlimit > 0)
			for (size_t currentY = 0; currentY < (size_t)rect.height(); ++currentY) {
				for (size_t currentX = 0; currentX < softlimit; ++currentX) {
					const size_t index = pitch * (rect.y() + currentY) + rect.x() + currentX * pixelsPerStep * 2;
					const __m256i vec8 = _mm256_loadu_si256((const __m256i*)&buffer[index]);
					sum[offsetR] = _mm256_add_epi32(sum[offsetR], _mm256_shuffle_epi8(vec8, shuffleR));
					sum[offsetG] = _mm256_add_epi32(sum[offsetG], _mm256_shuffle_epi8(vec8, shuffleG));
					sum[offsetB] = _mm256_add_epi32(sum[offsetB], _mm256_shuffle_epi8(vec8, shuffleB));
				}
			}
		//   2) the remaining delta-px wide rect
		const size_t delta = rect.width() % (pixelsPerStep * 2);
		if (delta > 0) {
			// masks to load only delta number of pixels
			const __m256i loadmasks[7] = {
				_mm256_setr_epi32(0xFFFFFFFFU,0,0,0,0,0,0,0),
				_mm256_setr_epi32(0xFFFFFFFFU,0xFFFFFFFFU,0,0,0,0,0,0),
				_mm256_setr_epi32(0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0,0,0,0,0),
				_mm256_setr_epi32(0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0,0,0,0),
				_mm256_setr_epi32(0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0,0,0),
				_mm256_setr_epi32(0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0,0),
				_mm256_setr_epi32(0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0)
			};
			const __m256i loadmask = loadmasks[delta - 1];
			for (size_t currentY = 0; currentY < (size_t)rect.height(); ++currentY) {
				const size_t index = pitch * (rect.y() + currentY) + rect.x() + softlimit * pixelsPerStep * 2;
				const __m256i vec8 = _mm256_maskload_epi32(&buffer[index], loadmask);
				sum[offsetR] = _mm256_add_epi32(sum[offsetR], _mm256_shuffle_epi8(vec8, shuffleR));
				sum[offsetG] = _mm256_add_epi32(sum[offsetG], _mm256_shuffle_epi8(vec8, shuffleG));
				sum[offsetB] = _mm256_add_epi32(sum[offsetB], _mm256_shuffle_epi8(vec8, shuffleB));
			}
		}

		const __m256i horizontalSum256 = _mm256_hadd_epi32(_mm256_hadd_epi32(sum[0], sum[1]) , _mm256_hadd_epi32(sum[2], sum[3]));
		const __m128i horizontalSum128 = _mm_add_epi32(_mm256_extracti128_si256(horizontalSum256, 0), _mm256_extracti128_si256(horizontalSum256, 1));
		const size_t count = rect.height() * rect.width();
		ColorValue color;
		color.r = (_mm_extract_epi32(horizontalSum128, offsetR) / count) & 0xff;
		color.g = (_mm_extract_epi32(horizontalSum128, offsetG) / count) & 0xff;
		color.b = (_mm_extract_epi32(horizontalSum128, offsetB) / count) & 0xff;
		return color;
	};

	template<uint8_t offsetR, uint8_t offsetG, uint8_t offsetB>
	static ColorValue accumulateBufferGamma(
		const int* const buff,
		const size_t pitch,
		const QRect& rect,
		const quint16* gamma) {
		const unsigned char* const buffer = (const unsigned char* const)buff;

		int r = 0;
		int g = 0;
		int b = 0;

		const int delta = rect.width() % pixelsPerStep;
		if (rect.width() >= pixelsPerStep)
			for (int currentY = 0; currentY < rect.height(); currentY++) {
				for (int currentX = 0; currentX < rect.width() - delta; currentX += pixelsPerStep) {
					const size_t index = pitch * bytesPerPixel * (rect.y() + currentY) + (rect.x() + currentX) * bytesPerPixel;
					r += gamma[PIXEL_R(0)] + gamma[PIXEL_R(1)] + gamma[PIXEL_R(2)] + gamma[PIXEL_R(3)];
					g += gamma[PIXEL_G(0)] + gamma[PIXEL_G(1)] + gamma[PIXEL_G(2)] + gamma[PIXEL_G(3)];
					b += gamma[PIXEL_B(0)] + gamma[PIXEL_B(1)] + gamma[PIXEL_B(2)] + gamma[PIXEL_B(3)];
				}
			}
		for (int currentX = rect.width() - delta; currentX < rect.width(); ++currentX) {
			for (int currentY = 0; currentY < rect.height(); ++currentY) {
				const size_t index = pitch * bytesPerPixel * (rect.y() + currentY) + (rect.x() + currentX) * bytesPerPixel;
				r += gamma[PIXEL_R(0)];
				g += gamma[PIXEL_G(0)];
				b += gamma[PIXEL_B(0)];
			}
		}

		const size_t count = rect.height() * rect.width();
		ColorValue color;
		color.r = (r / count) & 0xffff;
		color.g = (g / count) & 0xffff;
		color.b = (b / count) & 0xffff;
		return color;
	};

enum SIMDLevel {
    None = 0,
    SSE4_1 = 1 << 0,
    AVX2 = 1 << 1
};
// https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
#if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1300)
static uint32_t available_simd() {
    uint32_t level = SIMDLevel::None;
    if (_may_i_use_cpu_feature(_FEATURE_AVX2))
        level |= SIMDLevel::AVX2;
    if (_may_i_use_cpu_feature(_FEATURE_SSE4_1))
        level |= SIMDLevel::SSE4_1;
    return level;
}
#else /* non-Intel compiler */
void run_cpuid(uint32_t eax, uint32_t ecx, uint32_t* abcd)
{
#if defined(_MSC_VER)
    __cpuidex((int*)abcd, eax, ecx);
#else
    uint32_t ebx = 0;
    uint32_t edx = 0;
# if defined( __i386__ ) && defined ( __PIC__ )
     /* in case of PIC under 32-bit EBX cannot be clobbered */
    __asm__ ( "movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
    __asm__ ( "cpuid" : "+b" (ebx),
# endif
              "+a" (eax), "+c" (ecx), "=d" (edx) );
    abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
#endif
}


#if defined(_MSC_VER)
# include <intrin.h>
#endif
static uint32_t available_simd() {
    uint32_t abcd[4] = {0,0,0,0};

    run_cpuid(1, 0, abcd);
    uint32_t eax = 0x07;
    uint32_t ecx = 0x00;
#if defined(_MSC_VER)
    __cpuidex((int*)abcd, eax, ecx);
#else
    uint32_t ebx = 0;
    uint32_t edx = 0;
# if defined( __i386__ ) && defined ( __PIC__ )
     /* in case of PIC under 32-bit EBX cannot be clobbered */
    __asm__ ( "movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
    __asm__ ( "cpuid" : "+b" (ebx),
# endif
              "+a" (eax), "+c" (ecx), "=d" (edx) );
    abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
#endif
    uint32_t level = SIMDLevel::None;
    // CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]==1
    run_cpuid(7, 0, abcd);
    if ((abcd[1] & (1 << 5)))
        level |= SIMDLevel::AVX2;

    // CPUID.(EAX=01H, ECX=0H):ECX.SSE4_1[bit 19]==1
    run_cpuid(1, 0, abcd);
    if ((abcd[2] & (1 << 19)))
        level |= SIMDLevel::SSE4_1;

    return level;
}
#endif

/*
	accumulateBuffer128 requires SSE4.1
	accumulateBuffer256 requires AVX2

	instruction availability:
	Steam Hardware & Software Survey (March 2020)
	SSE4.1   97.88% / +0.69%
	AVX2     74.19% / +2.73%

	by default set functions to non-SIMD and upgrade to AVX2 or SSE4.1 when available
*/
auto accumulateARGB = accumulateBuffer<PIXEL_FORMAT_ARGB>;
auto accumulateABGR = accumulateBuffer<PIXEL_FORMAT_ABGR>;
auto accumulateRGBA = accumulateBuffer<PIXEL_FORMAT_RGBA>;
auto accumulateBGRA = accumulateBuffer<PIXEL_FORMAT_BGRA>;

struct simdupgrade {
	simdupgrade() {
		uint32_t level = available_simd();
		if (level & SIMDLevel::AVX2) {
			accumulateARGB = accumulateBuffer256<PIXEL_FORMAT_ARGB>;
			accumulateABGR = accumulateBuffer256<PIXEL_FORMAT_ABGR>;
			accumulateRGBA = accumulateBuffer256<PIXEL_FORMAT_RGBA>;
			accumulateBGRA = accumulateBuffer256<PIXEL_FORMAT_BGRA>;
		}
		else if (level & SIMDLevel::SSE4_1) {
			accumulateARGB = accumulateBuffer128<PIXEL_FORMAT_ARGB>;
			accumulateABGR = accumulateBuffer128<PIXEL_FORMAT_ABGR>;
			accumulateRGBA = accumulateBuffer128<PIXEL_FORMAT_RGBA>;
			accumulateBGRA = accumulateBuffer128<PIXEL_FORMAT_BGRA>;
		}
	}
};
simdupgrade avxup;
} // namespace

namespace Grab {
	namespace Calculations {
		QRgb calculateAvgColor(const unsigned char * const buffer, BufferFormat bufferFormat, const size_t pitch, const QRect &rect) {

			ColorValue color;
			switch(bufferFormat) {
			case BufferFormatArgb:
				color = accumulateARGB((int*)buffer, pitch / bytesPerPixel, rect);
				break;

			case BufferFormatAbgr:
				color = accumulateABGR((int*)buffer, pitch / bytesPerPixel, rect);
				break;

			case BufferFormatRgba:
				color = accumulateRGBA((int*)buffer, pitch / bytesPerPixel, rect);
				break;

			case BufferFormatBgra:
				color = accumulateBGRA((int*)buffer, pitch / bytesPerPixel, rect);
				break;
			default:
				return -1;
				break;
			}

			return qRgb(color.r, color.g, color.b);
		}

		QRgba64 calculateAvgColor(const unsigned char* const buffer, BufferFormat bufferFormat, const size_t pitch, const QRect& rect, const quint16* gamma) {
			
			ColorValue color;
			switch (bufferFormat) {
				case BufferFormatArgb:
					color = accumulateBufferGamma<PIXEL_FORMAT_ARGB>((int*)buffer, pitch / bytesPerPixel, rect, gamma);
					break;

				case BufferFormatAbgr:
					color = accumulateBufferGamma<PIXEL_FORMAT_ABGR>((int*)buffer, pitch / bytesPerPixel, rect, gamma);
					break;

				case BufferFormatRgba:
					color = accumulateBufferGamma<PIXEL_FORMAT_RGBA>((int*)buffer, pitch / bytesPerPixel, rect, gamma);
					break;

				case BufferFormatBgra:
					color = accumulateBufferGamma<PIXEL_FORMAT_BGRA>((int*)buffer, pitch / bytesPerPixel, rect, gamma);
					break;

				default:
					return qRgba64(0);
					break;
			}
			
			return qRgba64(color.r, color.g, color.b, 0);
		}
	}
}

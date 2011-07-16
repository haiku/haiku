/*
 * Copyright 2010 Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence.
 */


#include "Interpolate.h"

#include <cmath>


/*! Resampling class doing linear interpolation.
*/


Interpolate::Interpolate(uint32 src_format, uint32 dst_format)
	:
	Resampler(src_format, dst_format)
{
}

Interpolate::~Interpolate()
{
}


void
Interpolate::float_to_float(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dest = *(const float *)src * gain;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	#define SRC(n) *(const float*)(src + n * srcSampleOffset)

	while (count--) {
		*(float*)dest = gain * (SRC(0) + (SRC(1) - SRC(0)) * current) ;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::int32_to_float(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain / 2147483647.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dest = *(const int32 *)src * gain;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	#undef SRC
	#define SRC(n) *(const int32*)(src + n * srcSampleOffset)

	while (count--) {
		*(float*)dest = gain * (SRC(0) + (SRC(1) - SRC(0)) * current) ;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::int16_to_float(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain / 32767.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dest = *(const int16 *)src * gain;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	#undef SRC
	#define SRC(n) *(const int16*)(src + n * srcSampleOffset)

	while (count--) {
		*(float*)dest = gain * (SRC(0) + (SRC(1) - SRC(0)) * current);
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::int8_to_float(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain / 127.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dest = *(const int8 *)src * gain;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	#undef SRC
	#define SRC(n) *(const int8*)(src + n * srcSampleOffset)

	while (count--) {
		*(float*)dest = gain * (SRC(0) + (SRC(1) - SRC(0)) * current) ;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::uint8_to_float(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain / 127.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dest = (((int32) *(const uint8 *)src) - 128) * gain;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	#undef SRC
	#define SRC(n) ( *(const uint8*)(src + n * srcSampleOffset) - 128)

	while (count--) {
		*(float*)dest = gain * (SRC(0) + (SRC(1) - SRC(0)) * current);
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::float_to_int32(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain * 2147483647.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 2147483647.0f)
				*(int32 *)dest = 2147483647L;
			else if (sample < -2147483647.0f)
				*(int32 *)dest = -2147483647L;
			else
				*(int32 *)dest = (int32)sample;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	#undef SRC
	#define SRC(n) *(const float*)(src + n * srcSampleOffset)

	while (count--) {
		register float sample = gain * (SRC(0) + (SRC(1) - SRC(0))
			* current);
		if (sample > 2147483647.0f)
			*(int32 *)dest = 2147483647L;
		else if (sample < -2147483647.0f)
			*(int32 *)dest = -2147483647L;
		else
			*(int32 *)dest = (int32)sample;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::float_to_int16(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain * 32767.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 32767.0f)
				*(int16 *)dest = 32767;
			else if (sample < -32767.0f)
				*(int16 *)dest = -32767;
			else
				*(int16 *)dest = (int16)sample;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	while (count--) {
		register float sample = gain * (SRC(0) + (SRC(1) - SRC(0))
			* current);
		if (sample > 32767.0f)
			*(int16 *)dest = 32767;
		else if (sample < -32767.0f)
			*(int16 *)dest = -32767;
		else
			*(int16 *)dest = (int16)sample;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::float_to_int8(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain * 127.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 127.0f)
				*(int8 *)dest = 127;
			else if (sample < -127.0f)
				*(int8 *)dest = -127;
			else
				*(int8 *)dest = (int8)sample;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	while (count--) {
		register float sample = gain * (SRC(0) + (SRC(1) - SRC(0))
			* current);
		if (sample > 127.0f)
			*(int8 *)dest = 127;
		else if (sample < -127.0f)
			*(int8 *)dest = -127;
		else
			*(int8 *)dest = (int8)sample;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


void
Interpolate::float_to_uint8(const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain * 127.0;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			register float sample = 128.0f + *(const float *)src * gain;
			if (sample > 255.0f)
				*(uint8 *)dest = 255;
			else if (sample < 1.0f)
				*(uint8 *)dest = 1;
			else
				*(uint8 *)dest = (uint8)sample;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount - 1) / float(destSampleCount - 1);
	register float current = 0.0f;

	while (count--) {
		register float sample = gain * (SRC(0) + (SRC(1) - SRC(0))
			* current);
		if (sample > 255.0f)
			*(uint8 *)dest = 255;
		else if (sample < 1.0f)
			*(uint8 *)dest = 1;
		else
			*(uint8 *)dest = (uint8)sample;
		dest += destSampleOffset;
		current += delta;
		if (current >= 1.0f) {
			double ipart;
			current = modf(current, &ipart);
			src += srcSampleOffset * (int)ipart;
		}
	}
}


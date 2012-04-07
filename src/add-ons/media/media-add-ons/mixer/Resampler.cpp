/*
 * Copyright 2003 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 */


#include "Resampler.h"

#include <MediaDefs.h>

#include "MixerDebug.h"


/*!	A simple resampling class for the audio mixer.
	You pick the conversion function on object creation,
	and then call the Resample() function, specifying data pointer,
	offset (in bytes) to the next sample, and count of samples for
	both source and destination.
*/


Resampler::Resampler(uint32 src_format, uint32 dst_format)
	:
	fFunc(&Resampler::no_conversion)
{
	if (dst_format == media_raw_audio_format::B_AUDIO_FLOAT) {
		switch (src_format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				fFunc = &Resampler::float_to_float;
				return;
			case media_raw_audio_format::B_AUDIO_INT:
				fFunc = &Resampler::int32_to_float;
				return;
			case media_raw_audio_format::B_AUDIO_SHORT:
				fFunc = &Resampler::int16_to_float;
				return;
			case media_raw_audio_format::B_AUDIO_CHAR:
				fFunc = &Resampler::int8_to_float;
				return;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				fFunc = &Resampler::uint8_to_float;
				return;
			default:
				ERROR("Resampler::Resampler: unknown source format 0x%x\n",
					src_format);
				return;
		}
	}

	if (src_format == media_raw_audio_format::B_AUDIO_FLOAT) {
		switch (dst_format) {
			// float=>float already handled above
			case media_raw_audio_format::B_AUDIO_INT:
				fFunc = &Resampler::float_to_int32;
				return;
			case media_raw_audio_format::B_AUDIO_SHORT:
				fFunc = &Resampler::float_to_int16;
				return;
			case media_raw_audio_format::B_AUDIO_CHAR:
				fFunc = &Resampler::float_to_int8;
				return;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				fFunc = &Resampler::float_to_uint8;
				return;
			default:
				ERROR("Resampler::Resampler: unknown destination format 0x%x\n", dst_format);
				return;
		}
	}

	ERROR("Resampler::Resampler: source or destination format must be "
		"B_AUDIO_FLOAT\n");
}


Resampler::~Resampler()
{
}


status_t
Resampler::InitCheck() const
{
	return fFunc != &Resampler::no_conversion ? B_OK : B_ERROR;
}


void
Resampler::float_to_float(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dest = *(const float *)src * gain;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dest = *(const float *)src * gain;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::int32_to_float(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dest = *(const int32 *)src * gain;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dest = *(const int32 *)src * gain;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::int16_to_float(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dest = *(const int16 *)src * gain;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dest = *(const int16 *)src * gain;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::int8_to_float(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dest = *(const int8 *)src * gain;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dest = *(const int8 *)src * gain;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::uint8_to_float(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dest = (((int32) *(const uint8 *)src) - 128) * gain;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dest = (((int32) *(const uint8 *)src) - 128) * gain;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::float_to_int32(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 2147483647.0f)
				*(int32 *)dest = 2147483647L;
			else if (sample < -2147483647.0f)
				*(int32 *)dest = -2147483647L;
			else
				*(int32 *)dest = (int32)sample;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 2147483647.0f)
				*(int32 *)dest = 2147483647L;
			else if (sample < -2147483647.0f)
				*(int32 *)dest = -2147483647L;
			else
				*(int32 *)dest = (int32)sample;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::float_to_int16(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 32767.0f)
				*(int16 *)dest = 32767;
			else if (sample < -32767.0f)
				*(int16 *)dest = -32767;
			else
				*(int16 *)dest = (int16)sample;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 32767.0f)
				*(int16 *)dest = 32767;
			else if (sample < -32767.0f)
				*(int16 *)dest = -32767;
			else
				*(int16 *)dest = (int16)sample;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::float_to_int8(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 127.0f)
				*(int8 *)dest = 127;
			else if (sample < -127.0f)
				*(int8 *)dest = -127;
			else
				*(int8 *)dest = (int8)sample;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 127.0f)
				*(int8 *)dest = 127;
			else if (sample < -127.0f)
				*(int8 *)dest = -127;
			else
				*(int8 *)dest = (int8)sample;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}


void
Resampler::float_to_uint8(const void *_src, int32 srcSampleOffset,
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

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = 128.0f + *(const float *)src * gain;
			if (sample > 255.0f)
				*(uint8 *)dest = 255;
			else if (sample < 1.0f)
				*(uint8 *)dest = 1;
			else
				*(uint8 *)dest = (uint8)sample;
			dest += destSampleOffset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += srcSampleOffset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = 128.0f + *(const float *)src * gain;
			if (sample > 255.0f)
				*(uint8 *)dest = 255;
			else if (sample < 1.0f)
				*(uint8 *)dest = 1;
			else
				*(uint8 *)dest = (uint8)sample;
			dest += destSampleOffset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * srcSampleOffset;
		}
	}
}

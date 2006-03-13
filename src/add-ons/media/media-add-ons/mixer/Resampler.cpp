/* Copyright (C) 2003 Marcus Overhagen
 * Released under terms of the MIT license.
 *
 * A simple resampling class for the audio mixer.
 * You pick the conversion function on object creation,
 * and then call the Resample() function, specifying data pointer,
 * offset (in bytes) to the next sample, and count of samples for
 * both source and destination.
 *
 */

#include <MediaDefs.h>
#include "Resampler.h"
#include "MixerDebug.h"

Resampler::Resampler(uint32 src_format, uint32 dst_format, int16 dst_valid_bits)
 :	fFunc(0)
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
				ERROR("Resampler::Resampler: unknown source format 0x%x\n", src_format);
				return;
		}
	}

	if (src_format == media_raw_audio_format::B_AUDIO_FLOAT) {
		switch (dst_format) {
			// float=>float already handled above
			case media_raw_audio_format::B_AUDIO_INT:
				fFunc = &Resampler::float_to_int32_32;
				if (dst_valid_bits == 24)
					fFunc = &Resampler::float_to_int32_24;
				else if (dst_valid_bits == 20)
					fFunc = &Resampler::float_to_int32_20;
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

	ERROR("Resampler::Resampler: source or destination format must be B_AUDIO_FLOAT\n");
}

Resampler::~Resampler()
{
}

status_t
Resampler::InitCheck()
{
	return (fFunc != 0) ? B_OK : B_ERROR;
}

void
Resampler::float_to_float(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dst = *(const float *)src * gain;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dst = *(const float *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dst = *(const float *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::int32_to_float(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain / 2147483647.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dst = *(const int32 *)src * gain;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dst = *(const int32 *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dst = *(const int32 *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::int16_to_float(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain / 32767.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dst = *(const int16 *)src * gain;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dst = *(const int16 *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dst = *(const int16 *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::int8_to_float(const void *_src, int32 src_sample_offset, int32 src_sample_count,
				  		 void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain / 127.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dst = *(const int8 *)src * gain;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dst = *(const int8 *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dst = *(const int8 *)src * gain;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::uint8_to_float(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain / 127.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			*(float *)dst = (((int32) *(const uint8 *)src) - 128) * gain;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			*(float *)dst = (((int32) *(const uint8 *)src) - 128) * gain;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			*(float *)dst = (((int32) *(const uint8 *)src) - 128) * gain;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::float_to_int32_32(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain * 2147483647.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 2147483647.0f)
				*(int32 *)dst = 2147483647L;
			else if (sample < -2147483647.0f)
				*(int32 *)dst = -2147483647L;
			else
				*(int32 *)dst = (int32)sample;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 2147483647.0f)
				*(int32 *)dst = 2147483647L;
			else if (sample < -2147483647.0f)
				*(int32 *)dst = -2147483647L;
			else
				*(int32 *)dst = (int32)sample;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 2147483647.0f)
				*(int32 *)dst = 2147483647L;
			else if (sample < -2147483647.0f)
				*(int32 *)dst = -2147483647L;
			else
				*(int32 *)dst = (int32)sample;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}


void
Resampler::float_to_int32_24(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain * 8388607.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 8388607.0f)
				*(int32 *)dst = 8388607L;
			else if (sample < -8388607.0f)
				*(int32 *)dst = -8388607L;
			else
				*(int32 *)dst = (int32)sample;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 8388607.0f)
				*(int32 *)dst = 8388607L;
			else if (sample < -8388607.0f)
				*(int32 *)dst = -8388607L;
			else
				*(int32 *)dst = (int32)sample;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 8388607.0f)
				*(int32 *)dst = 8388607L;
			else if (sample < -8388607.0f)
				*(int32 *)dst = -8388607L;
			else
				*(int32 *)dst = (int32)sample;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}


void
Resampler::float_to_int32_20(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain * 524287.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 524287.0f)
				*(int32 *)dst = 524287L;
			else if (sample < -524287.0f)
				*(int32 *)dst = -524287L;
			else
				*(int32 *)dst = (int32)sample;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 524287.0f)
				*(int32 *)dst = 524287L;
			else if (sample < -524287.0f)
				*(int32 *)dst = -524287L;
			else
				*(int32 *)dst = (int32)sample;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 524287.0f)
				*(int32 *)dst = 524287L;
			else if (sample < -524287.0f)
				*(int32 *)dst = -524287L;
			else
				*(int32 *)dst = (int32)sample;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}


void
Resampler::float_to_int16(const void *_src, int32 src_sample_offset, int32 src_sample_count,
				  		  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain * 32767.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 32767.0f)
				*(int16 *)dst = 32767;
			else if (sample < -32767.0f)
				*(int16 *)dst = -32767;
			else
				*(int16 *)dst = (int16)sample;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 32767.0f)
				*(int16 *)dst = 32767;
			else if (sample < -32767.0f)
				*(int16 *)dst = -32767;
			else
				*(int16 *)dst = (int16)sample;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 32767.0f)
				*(int16 *)dst = 32767;
			else if (sample < -32767.0f)
				*(int16 *)dst = -32767;
			else
				*(int16 *)dst = (int16)sample;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::float_to_int8(const void *_src, int32 src_sample_offset, int32 src_sample_count,
			 			 void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain * 127.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 127.0f)
				*(int8 *)dst = 127;
			else if (sample < -127.0f)
				*(int8 *)dst = -127;
			else
				*(int8 *)dst = (int8)sample;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 127.0f)
				*(int8 *)dst = 127;
			else if (sample < -127.0f)
				*(int8 *)dst = -127;
			else
				*(int8 *)dst = (int8)sample;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = *(const float *)src * gain;
			if (sample > 127.0f)
				*(int8 *)dst = 127;
			else if (sample < -127.0f)
				*(int8 *)dst = -127;
			else
				*(int8 *)dst = (int8)sample;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

void
Resampler::float_to_uint8(const void *_src, int32 src_sample_offset, int32 src_sample_count,
						  void *_dst, int32 dst_sample_offset, int32 dst_sample_count, float _gain)
{
	register const char * src = (const char *) _src;
	register char * dst = (char *) _dst;
	register int32 count = dst_sample_count;
	register float gain = _gain * 127.0;
	
	if (src_sample_count == dst_sample_count) {
		// optimized case for no resampling
		while (count--) {
			register float sample = 128.0f + *(const float *)src * gain;
			if (sample > 255.0f)
				*(uint8 *)dst = 255;
			else if (sample < 1.0f)
				*(uint8 *)dst = 1;
			else
				*(uint8 *)dst = (uint8)sample;
			src += src_sample_offset;
			dst += dst_sample_offset;
		}
		return;
	}

	register float delta = float(src_sample_count) / float(dst_sample_count);
	register float current = 0.0f;

	if (delta < 1.0) {
		// upsample
		while (count--) {
			register float sample = 128.0f + *(const float *)src * gain;
			if (sample > 255.0f)
				*(uint8 *)dst = 255;
			else if (sample < 1.0f)
				*(uint8 *)dst = 1;
			else
				*(uint8 *)dst = (uint8)sample;
			dst += dst_sample_offset;
			current += delta;
			if (current >= 1.0f) {
				current -= 1.0f;
				src += src_sample_offset;
			}
		}
	} else {
		// downsample
		while (count--) {
			register float sample = 128.0f + *(const float *)src * gain;
			if (sample > 255.0f)
				*(uint8 *)dst = 255;
			else if (sample < 1.0f)
				*(uint8 *)dst = 1;
			else
				*(uint8 *)dst = (uint8)sample;
			dst += dst_sample_offset;
			current += delta;
			register int32 skipcount = (int32)current;
			current -= skipcount;
			src += skipcount * src_sample_offset;
		}
	}
}

/* Copyright (C) 2003 Marcus Overhagen
 * Released under terms of the MIT license.
 *
 * A simple resampling class for the audio mixer.
 * You pick the conversation function on object creation,
 * and then call the Resample() function, specifying data pointer,
 * offset (in bytes) to the next sample, and count of samples for
 * both source and destination.
 *
 */

#include "Resampler.h"
#include "debug.h"

Resampler::Resampler(uint32 src_format, uint32 dst_format)
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
				FATAL("Resampler::Resampler: unknown source format 0x%x\n", src_format);
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
				FATAL("Resampler::Resampler: unknown destination format 0x%x\n", dst_format);
				return;
		}
	}

	FATAL("Resampler::Resampler: source or destination format must be B_AUDIO_FLOAT\n");
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
Resampler::float_to_float(const void *src, int32 src_sample_offset, int32 src_sample_count,
						  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::int32_to_float(const void *src, int32 src_sample_offset, int32 src_sample_count,
						  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::int16_to_float(const void *src, int32 src_sample_offset, int32 src_sample_count,
						  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::int8_to_float(const void *src, int32 src_sample_offset, int32 src_sample_count,
				  		 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::uint8_to_float(const void *src, int32 src_sample_offset, int32 src_sample_count,
						  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::float_to_int32(const void *src, int32 src_sample_offset, int32 src_sample_count,
						  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::float_to_int16(const void *src, int32 src_sample_offset, int32 src_sample_count,
				  		  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::float_to_int8(const void *src, int32 src_sample_offset, int32 src_sample_count,
			 			 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}

void
Resampler::float_to_uint8(const void *src, int32 src_sample_offset, int32 src_sample_count,
						  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
}


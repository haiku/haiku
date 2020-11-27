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


template<typename inType, typename outType, int gnum, int gden,
	int inMiddle, int outMiddle, int32 min, int32 max> static void
kernel(Resampler* object, const void *_src, int32 srcSampleOffset,
	int32 srcSampleCount, void *_dest, int32 destSampleOffset,
	int32 destSampleCount, float _gain)
{
	register const char * src = (const char *)_src;
	register char * dest = (char *)_dest;
	register int32 count = destSampleCount;
	register float gain = _gain * gnum / gden;

	if (srcSampleCount == destSampleCount) {
		// optimized case for no resampling
		while (count--) {
			float tmp = ((*(const inType*)src) - inMiddle) * gain + outMiddle;
			if (tmp < min) tmp = min;
			if (tmp > max) tmp = max;
			*(outType *)dest = (outType)tmp;
			src += srcSampleOffset;
			dest += destSampleOffset;
		}
		return;
	}

	register float delta = float(srcSampleCount) / float(destSampleCount);
	register float current = 0.0f;

	// downsample
	while (count--) {
		float tmp = ((*(const inType*)src) - inMiddle) * gain + outMiddle;
		if (tmp < min) tmp = min;
		if (tmp > max) tmp = max;
		*(outType *)dest = (outType)tmp;

		dest += destSampleOffset;
		current += delta;
		register int32 skipcount = (int32)current;
		current -= skipcount;
		src += skipcount * srcSampleOffset;
	}
}


Resampler::Resampler(uint32 src_format, uint32 dst_format)
	:
	fFunc(0)
{
	if (dst_format == media_raw_audio_format::B_AUDIO_FLOAT) {
		switch (src_format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				fFunc = &kernel<float, float, 1, 1, 0, 0, -1, 1>;
				return;
			case media_raw_audio_format::B_AUDIO_INT:
				fFunc = &kernel<int32, float, 1, INT32_MAX, 0, 0, -1, 1>;
				return;
			case media_raw_audio_format::B_AUDIO_SHORT:
				fFunc = &kernel<int16, float, 1, INT16_MAX, 0, 0, -1, 1>;
				return;
			case media_raw_audio_format::B_AUDIO_CHAR:
				fFunc = &kernel<int8, float, 1, INT8_MAX, 0, 0, -1, 1>;
				return;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				fFunc = &kernel<uint8, float, 2, UINT8_MAX, 128, 0, -1, 1>;
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
				fFunc = &kernel<float, int32, INT32_MAX, 1, 0, 0,
					INT32_MIN, INT32_MAX>;
				return;
			case media_raw_audio_format::B_AUDIO_SHORT:
				fFunc = &kernel<float, int16, INT16_MAX, 1, 0, 0,
					INT16_MIN, INT16_MAX>;
				return;
			case media_raw_audio_format::B_AUDIO_CHAR:
				fFunc = &kernel<float, int8, INT8_MAX, 1, 0, 0,
					INT8_MIN, INT8_MAX>;
				return;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				fFunc = &kernel<float, uint8, UINT8_MAX, 2, 0, 128,
					0, UINT8_MAX>;
				return;
			default:
				ERROR("Resampler::Resampler: unknown destination format 0x%x\n",
					dst_format);
				return;
		}
	}

	ERROR("Resampler::Resampler: source or destination format must be "
		"B_AUDIO_FLOAT\n");
}


Resampler::Resampler()
	:
	fFunc(0)
{
}



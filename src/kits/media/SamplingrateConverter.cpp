/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SamplingrateConverter.cpp
 *  DESCR: Converts between different sampling rates
 *         This is really bad code, as there are much better algorithms
 *         than the simple duplicating or dropping of samples.
 *         Also, the implementation isn't very nice.
 *         Feel free to send me better versions to marcus@overhagen.de
 ***********************************************************************/

#include <SupportDefs.h>
#include <MediaDefs.h>
#include <stdio.h>

#include "SamplingrateConverter.h"


namespace MediaKitPrivate {

static void Upsample_stereo(const float *inbuffer, int inframecount, float *outbuffer, int outframecount);
static void Upsample_mono(const float *inbuffer, int inframecount, float *outbuffer, int outframecount);
static void Upsample_stereo(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount);
static void Upsample_mono(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount);
static void Upsample_stereo(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount);
static void Upsample_mono(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount);
static void Upsample_stereo(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount);
static void Upsample_mono(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount);
static void Upsample_stereo(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount);
static void Upsample_mono(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount);
static void Downsample_stereo(const float *inbuffer, int inframecount, float *outbuffer, int outframecount);
static void Downsample_mono(const float *inbuffer, int inframecount, float *outbuffer, int outframecount);
static void Downsample_stereo(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount);
static void Downsample_mono(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount);
static void Downsample_stereo(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount);
static void Downsample_mono(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount);
static void Downsample_stereo(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount);
static void Downsample_mono(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount);
static void Downsample_stereo(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount);
static void Downsample_mono(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount);


status_t 
SamplingrateConverter::convert(void *dest, int destframecount,
							   const void *src, int srcframecount,
					           uint32 format, 
					           int channel_count)
{
	if (channel_count != 1 && channel_count != 2) {
		fprintf(stderr,"SamplingrateConverter::convert() unsupported channel count %d\n",channel_count);
		return B_ERROR;
	}
	switch (format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			if (destframecount > srcframecount) {
				if (channel_count == 1)
					Upsample_mono((const float *)src,srcframecount,(float *)dest,destframecount);
				else
					Upsample_stereo((const float *)src,srcframecount,(float *)dest,destframecount);
			} else {
				if (channel_count == 1)
					Downsample_mono((const float *)src,srcframecount,(float *)dest,destframecount);
				else
					Downsample_stereo((const float *)src,srcframecount,(float *)dest,destframecount);
			}
			return B_OK;
		case media_raw_audio_format::B_AUDIO_INT:
			if (destframecount > srcframecount) {
				if (channel_count == 1)
					Upsample_mono((const int32 *)src,srcframecount,(int32 *)dest,destframecount);
				else
					Upsample_stereo((const int32 *)src,srcframecount,(int32 *)dest,destframecount);
			} else {
				if (channel_count == 1)
					Downsample_mono((const int32 *)src,srcframecount,(int32 *)dest,destframecount);
				else
					Downsample_stereo((const int32 *)src,srcframecount,(int32 *)dest,destframecount);
			}
			return B_OK;
		case media_raw_audio_format::B_AUDIO_SHORT:
			if (destframecount > srcframecount) {
				if (channel_count == 1)
					Upsample_mono((const int16 *)src,srcframecount,(int16 *)dest,destframecount);
				else
					Upsample_stereo((const int16 *)src,srcframecount,(int16 *)dest,destframecount);
			} else {
				if (channel_count == 1)
					Downsample_mono((const int16 *)src,srcframecount,(int16 *)dest,destframecount);
				else
					Downsample_stereo((const int16 *)src,srcframecount,(int16 *)dest,destframecount);
			}
			return B_OK;
		case media_raw_audio_format::B_AUDIO_CHAR:
			if (destframecount > srcframecount) {
				if (channel_count == 1)
					Upsample_mono((const int8 *)src,srcframecount,(int8 *)dest,destframecount);
				else
					Upsample_stereo((const int8 *)src,srcframecount,(int8 *)dest,destframecount);
			} else {
				if (channel_count == 1)
					Downsample_mono((const int8 *)src,srcframecount,(int8 *)dest,destframecount);
				else
					Downsample_stereo((const int8 *)src,srcframecount,(int8 *)dest,destframecount);
			}
			return B_OK;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			if (destframecount > srcframecount) {
				if (channel_count == 1)
					Upsample_mono((const uint8 *)src,srcframecount,(uint8 *)dest,destframecount);
				else
					Upsample_stereo((const uint8 *)src,srcframecount,(uint8 *)dest,destframecount);
			} else {
				if (channel_count == 1)
					Downsample_mono((const uint8 *)src,srcframecount,(uint8 *)dest,destframecount);
				else
					Downsample_stereo((const uint8 *)src,srcframecount,(uint8 *)dest,destframecount);
			}
			return B_OK;
		default:
			fprintf(stderr,"SamplingrateConverter::convert() unsupported sample format %d\n",(int)format);
			return B_ERROR;
	}
}

void Upsample_stereo(const float *inbuffer, int inframecount, float *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		*outbuffer++ = inbuffer[1];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer += 2;
		}
	}
}

void Upsample_mono(const float *inbuffer, int inframecount, float *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer++;
		}
	}
}

void Upsample_stereo(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		*outbuffer++ = inbuffer[1];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer += 2;
		}
	}
}

void Upsample_mono(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer++;
		}
	}
}

void Upsample_stereo(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		*outbuffer++ = inbuffer[1];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer += 2;
		}
	}
}

void Upsample_mono(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer++;
		}
	}
}

void Upsample_stereo(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		*outbuffer++ = inbuffer[1];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer += 2;
		}
	}
}

void Upsample_mono(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer++;
		}
	}
}

void Upsample_stereo(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		*outbuffer++ = inbuffer[1];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer += 2;
		}
	}
}

void Upsample_mono(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount)
{
	float delta = float(inframecount) / float(outframecount);
	float current = 0.0f;
		
	while (outframecount--) {
		*outbuffer++ = inbuffer[0];
		current += delta;
		if (current > 1.0f) {
			current -= 1.0f;
			inbuffer++;
		}
	}
}

void Downsample_stereo(const float *inbuffer, int inframecount, float *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		int p = int(pos);
		*outbuffer++ = inbuffer[p];
		*outbuffer++ = inbuffer[p+1];
		pos += delta;
	}
}

void Downsample_mono(const float *inbuffer, int inframecount, float *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		*outbuffer++ = inbuffer[int(pos)];
		pos += delta;
	}
}

void Downsample_stereo(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		int p = int(pos);
		*outbuffer++ = inbuffer[p];
		*outbuffer++ = inbuffer[p+1];
		pos += delta;
	}
}

void Downsample_mono(const int32 *inbuffer, int inframecount, int32 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		*outbuffer++ = inbuffer[int(pos)];
		pos += delta;
	}
}

void Downsample_stereo(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		int p = int(pos);
		*outbuffer++ = inbuffer[p];
		*outbuffer++ = inbuffer[p+1];
		pos += delta;
	}
}

void Downsample_mono(const int16 *inbuffer, int inframecount, int16 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		*outbuffer++ = inbuffer[int(pos)];
		pos += delta;
	}
}

void Downsample_stereo(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		int p = int(pos);
		*outbuffer++ = inbuffer[p];
		*outbuffer++ = inbuffer[p+1];
		pos += delta;
	}
}

void Downsample_mono(const int8 *inbuffer, int inframecount, int8 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		*outbuffer++ = inbuffer[int(pos)];
		pos += delta;
	}
}

void Downsample_stereo(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		int p = int(pos);
		*outbuffer++ = inbuffer[p];
		*outbuffer++ = inbuffer[p+1];
		pos += delta;
	}
}

void Downsample_mono(const uint8 *inbuffer, int inframecount, uint8 *outbuffer, int outframecount)
{
	double delta = double(inframecount) / double(outframecount);
	double pos = 0.0;
	while (outframecount--) {
		*outbuffer++ = inbuffer[int(pos)];
		pos += delta;
	}
}

} //namespace MediaKitPrivate

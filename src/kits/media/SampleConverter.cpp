/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SampleConverter.cpp
 *  DESCR: Converts between different sample formats
 ***********************************************************************/

#include <SupportDefs.h>
#include <MediaDefs.h>
#include <stdio.h>

#include "SampleConverter.h"

extern "C" {
// used by BeOS R5 legacy.media_addon and mixer.media_addon
void convertBufferFloatToShort(float *out, short *in, int32 count);
void convertBufferFloatToShort(float *out, short *in, int32 count)
{
	MediaKitPrivate::SampleConverter::convert(
		out,
		media_raw_audio_format::B_AUDIO_FLOAT,
		in,
		media_raw_audio_format::B_AUDIO_SHORT,
		count);
}
};

namespace MediaKitPrivate {

status_t 
SampleConverter::convert(void *dest, uint32 dest_format, 
        const void *source, uint32 source_format, int samplecount)
{
	switch (dest_format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			switch (source_format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					memcpy(dest,source,samplecount * sizeof(float));
					return B_OK;
				case media_raw_audio_format::B_AUDIO_INT:
					convert((float *)dest,(const int32 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert((float *)dest,(const int16 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert((float *)dest,(const int8 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert((float *)dest,(const uint8 *)source,samplecount);
					return B_OK;
				default:
					fprintf(stderr,"SampleConverter::convert() unsupported source sample format %d\n",(int)source_format);
					return B_ERROR;
			}
		case media_raw_audio_format::B_AUDIO_INT:
			switch (source_format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert((int32 *)dest,(const float *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_INT:
					memcpy(dest,source,samplecount * sizeof(int32));
					return B_OK;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert((int32 *)dest,(const int16 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert((int32 *)dest,(const int8 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert((int32 *)dest,(const uint8 *)source,samplecount);
					return B_OK;
				default:
					fprintf(stderr,"SampleConverter::convert() unsupported source sample format %d\n",(int)source_format);
					return B_ERROR;
			}
		case media_raw_audio_format::B_AUDIO_SHORT:
			switch (source_format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert((int16 *)dest,(const float *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_INT:
					convert((int16 *)dest,(const int32 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_SHORT:
					memcpy(dest,source,samplecount * sizeof(int16));
					return B_OK;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert((int16 *)dest,(const int8 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert((int16 *)dest,(const uint8 *)source,samplecount);
					return B_OK;
				default:
					fprintf(stderr,"SampleConverter::convert() unsupported source sample format %d\n",(int)source_format);
					return B_ERROR;
			}
		case media_raw_audio_format::B_AUDIO_CHAR:
			switch (source_format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert((int8 *)dest,(const float *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_INT:
					convert((int8 *)dest,(const int32 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert((int8 *)dest,(const int16 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_CHAR:
					memcpy(dest,source,samplecount * sizeof(int8));
					return B_OK;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert((int8 *)dest,(const uint8 *)source,samplecount);
					return B_OK;
				default:
					fprintf(stderr,"SampleConverter::convert() unsupported source sample format %d\n",(int)source_format);
					return B_ERROR;
			}
		case media_raw_audio_format::B_AUDIO_UCHAR:
			switch (source_format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert((uint8 *)dest,(const float *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_INT:
					convert((uint8 *)dest,(const int32 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert((uint8 *)dest,(const int16 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert((uint8 *)dest,(const int8 *)source,samplecount);
					return B_OK;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					memcpy(dest,source,samplecount * sizeof(uint8));
					return B_OK;
				default:
					fprintf(stderr,"SampleConverter::convert() unsupported source sample format %d\n",(int)source_format);
					return B_ERROR;
			}
		default: 
			fprintf(stderr,"SampleConverter::convert() unsupported destination sample format %d\n",(int)source_format);
			return B_ERROR;
	}
}

void SampleConverter::convert(uint8 *dest, const float *source, int samplecount)
{
	while (samplecount--) {
		register int32 sample = int32(*(source++) * 127.0f);
		if (sample > 127)
			sample = 255;
		else if (sample < -127)
			sample = 1;
		*dest++ = uint8(sample + 128);
	}
}

void SampleConverter::convert(uint8 *dest, const int32 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = uint8((*source++ / (256 * 256 * 256)) + 128);
}

void SampleConverter::convert(uint8 *dest, const int16 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = uint8((int32(*source++) / 256) + 128);
}

void SampleConverter::convert(uint8 *dest, const int8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = uint8(int32(*source++) + 128);
}

void SampleConverter::convert(int8 *dest, const float *source, int samplecount)
{
	while (samplecount--) {
		register int32 sample = (int32) (*(source++) * 127.0f);
		if (sample > 127)
			*dest++ = 127;
		else if (sample < -127)
			*dest++ = -127;
		else
			*dest++ = int8(sample);
	}
}

void SampleConverter::convert(int8 *dest, const int32 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int8(*source++ / (256 * 256 * 256));
}

void SampleConverter::convert(int8 *dest, const int16 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int8(*source++ / 256);
}

void SampleConverter::convert(int8 *dest, const uint8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int8(*source++ - 128);
}

void SampleConverter::convert(int16 *dest, const float *source, int samplecount)
{
	while (samplecount--) {
		register int32 sample = (int32) (*source++ * 32767.0f);
		if (sample > 32767)
			*dest++ = 32767;
		else if (sample < -32767)
			*dest++ = -32767;
		else
			*dest++ = int16(sample);
	}
}

void SampleConverter::convert(int16 *dest, const int32 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int16(*source++ / (256 * 256));
}

void SampleConverter::convert(int16 *dest, const int8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int16(*source++) * 256;
}

void SampleConverter::convert(int16 *dest, const uint8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = (int16(*source++) - 128) * 256;
}

void SampleConverter::convert(int32 *dest, const float *source, int samplecount)
{
	while (samplecount--) {
		register double sample = double(*source++) * 2147483647.0;
		if (sample > 2147483647.0)
			*dest++ = 2147483647;
		else if (sample < -2147483647.0)
			*dest++ = -2147483647;
		else
			*dest++ = int32(sample);
	}
}

void SampleConverter::convert(int32 *dest, const int16 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int32(*source++) * 256 * 256;
}

void SampleConverter::convert(int32 *dest, const int8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = int32(*source++) * 256 * 256 * 256;
}

void SampleConverter::convert(int32 *dest, const uint8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = (int32(*source++) - 128) * 256 * 256 * 256;
}

void SampleConverter::convert(float *dest, const int32 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = float(*source++) / 2147483647.0f;
}

void SampleConverter::convert(float *dest, const int16 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = float(*source++) / 32767.0f;
}

void SampleConverter::convert(float *dest, const int8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = float(*source++) / 127.0f;
}

void SampleConverter::convert(float *dest, const uint8 *source, int samplecount)
{
	while (samplecount--)
		*dest++ = (float(*source++) - 128.0f) / 127.0f;
}

} //namespace MediaKitPrivate

/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: ChannelMixer.cpp
 *  DESCR: Converts mono into stereo, stereo into mono
 *         (multiple channel support should be added in the future)
 ***********************************************************************/

#include <SupportDefs.h>
#include <MediaDefs.h>
#include <stdio.h>

#include "ChannelMixer.h"

namespace MediaKitPrivate {

status_t 
ChannelMixer::mix(void *dest, int dest_chan_count, 
                  const void *source, int source_chan_count,
                  int framecount, uint32 format)
{
	if (dest_chan_count == 1 && source_chan_count == 2)
		return mix_2_to_1(dest, source, framecount, format);
	else if (dest_chan_count == 2 && source_chan_count == 1)
		return mix_1_to_2(dest, source, framecount, format);
	else {
		fprintf(stderr,"ChannelMixer::mix() dest_chan_count = %d, source_chan_count = %d not supported\n",
				dest_chan_count, 
				source_chan_count);
		return B_ERROR;
	}
}

status_t
ChannelMixer::mix_2_to_1(void *dest, const void *source, int framecount, uint32 format)
{
	switch (format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
		{
			register float *d = (float *)dest;
			register float *s = (float *)source;
			while (framecount--) {
				*(d++) = (s[0] + s[1]) / 2.0f;
				s += 2;
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_INT:
		{
			register int32 *d = (int32 *)dest;
			register int32 *s = (int32 *)source;
			while (framecount--) {
				*(d++) = (s[0] >> 1) + (s[1] >> 1);
				s += 2;
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_SHORT:
		{
			register int16 *d = (int16 *)dest;
			register int16 *s = (int16 *)source;
			while (framecount--) {
				register int32 temp = s[0] + s[1];
				*(d++) = int16(temp >> 1);
				s += 2;
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_CHAR:
		{
			register int8 *d = (int8 *)dest;
			register int8 *s = (int8 *)source;
			while (framecount--) {
				register int32 temp = s[0] + s[1];
				*(d++) = int8(temp >> 1);
				s += 2;
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_UCHAR:
		{
			register uint8 *d = (uint8 *)dest;
			register uint8 *s = (uint8 *)source;
			while (framecount--) {
				register int32 temp = (int32(s[0]) - 128) + (int32(s[1]) - 128);
				*(d++) = int8((temp >> 1) + 128);
				s += 2;
			}
			return B_OK;
		}
		default:
		{
			fprintf(stderr,"ChannelMixer::mix_2_to_1() unsupported sample format %d\n",(int)format);
			return B_ERROR;
		}
	}
}

status_t
ChannelMixer::mix_1_to_2(void *dest, const void *source, int framecount, uint32 format)
{
	switch (format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
		{
			register float *d = (float *)dest;
			register float *s = (float *)source;
			while (framecount--) {
				*(d++) = *s;
				*(d++) = *(s++);
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_INT:
		{
			register int32 *d = (int32 *)dest;
			register int32 *s = (int32 *)source;
			while (framecount--) {
				*(d++) = *s;
				*(d++) = *(s++);
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_SHORT:
		{
			register int16 *d = (int16 *)dest;
			register int16 *s = (int16 *)source;
			while (framecount--) {
				*(d++) = *s;
				*(d++) = *(s++);
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_CHAR:
		{
			register int8 *d = (int8 *)dest;
			register int8 *s = (int8 *)source;
			while (framecount--) {
				*(d++) = *s;
				*(d++) = *(s++);
			}
			return B_OK;
		}
		case media_raw_audio_format::B_AUDIO_UCHAR:
		{
			register uint8 *d = (uint8 *)dest;
			register uint8 *s = (uint8 *)source;
			while (framecount--) {
				*(d++) = *s;
				*(d++) = *(s++);
			}
			return B_OK;
		}
		default:
		{
			fprintf(stderr,"ChannelMixer::mix_1_to_2() unsupported sample format %d\n",(int)format);
			return B_ERROR;
		}
	}
}

} //namespace MediaKitPrivate

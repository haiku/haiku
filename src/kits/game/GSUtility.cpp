/*
 * Copyright 2001-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 */


#include "GSUtility.h"

#include <GameSoundDefs.h>
#include <MediaDefs.h>

#include <new>


_gs_ramp*
InitRamp(float* value, float set, float frames, bigtime_t duration)
{
	float diff = (set > *value) ? set - *value : *value - set;
	bigtime_t sec = bigtime_t(duration / 1000000.0);
	float inc = diff * 200;

	_gs_ramp* ramp = new (std::nothrow) _gs_ramp;
	if (ramp != NULL) {
		ramp->value = value;

		ramp->frame_total = frames * sec;
		ramp->frame_inc = int(ramp->frame_total / inc);

		ramp->inc = (set - *value) / inc;

		ramp->frame_count = 0;
		ramp->frame_inc_count = 0;

		ramp->duration = duration;
	}
	return ramp;
}


bool
ChangeRamp(_gs_ramp* ramp)
{
	if (ramp->frame_count > ramp->frame_total)
		return true;

	if (ramp->frame_inc_count >= ramp->frame_inc) {
		ramp->frame_inc_count = 0;
		*ramp->value += ramp->inc;
	} else
		ramp->frame_inc_count++;

	ramp->frame_count++;
	return false;
}


size_t
get_sample_size(int32 format)
{
	size_t sample;

	switch(format) {
		case media_raw_audio_format::B_AUDIO_CHAR:
			sample  = sizeof(char);
			break;

		case gs_audio_format::B_GS_U8:
			sample = sizeof(uint8);
			break;

		case gs_audio_format::B_GS_S16:
			sample = sizeof(int16);
			break;

		case gs_audio_format::B_GS_S32:
			sample = sizeof(int32);
			break;

		case gs_audio_format::B_GS_F:
			sample = sizeof(float);
			break;

		default:
			sample = 0;
			break;
	}

	return sample;
}


void
media_to_gs_format(gs_audio_format* dest, media_raw_audio_format* source)
{
	dest->format = source->format;
	dest->frame_rate = source->frame_rate;
	dest->channel_count = source->channel_count;
	dest->byte_order = source->byte_order;
	dest->buffer_size = source->buffer_size;
}

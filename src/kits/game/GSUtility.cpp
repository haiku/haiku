//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		GameSoundDevice.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	Utility functions used by sound system
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <MediaDefs.h>

// Project Includes ------------------------------------------------------------
#include <GameSoundDefs.h>

// Local Includes --------------------------------------------------------------
#include "GSUtility.h"

// Local Defines ---------------------------------------------------------------

// Utility functions -----------------------------------------------------------
_gs_ramp* InitRamp(float* value, float set, float frames, bigtime_t duration)
{
	float diff = (set > *value) ? set - *value : *value - set;
	bigtime_t sec = bigtime_t(duration / 1000000.0);
	float inc = diff * 200;
	
	_gs_ramp* ramp = new _gs_ramp;
	ramp->value = value;
	
	ramp->frame_total = frames * sec;
	ramp->frame_inc = int(ramp->frame_total / inc);
	
	ramp->inc = (set - *value) / inc;
	
	ramp->frame_count = 0;
	ramp->frame_inc_count = 0;
	
	ramp->duration = duration;
	
	return ramp;
}
	
bool ChangeRamp(_gs_ramp* ramp)
{
	if (ramp->frame_count > ramp->frame_total) return true;
	
	if (ramp->frame_inc_count >= ramp->frame_inc)
	{
		ramp->frame_inc_count = 0;	
		*ramp->value += ramp->inc;
	}
	else ramp->frame_inc_count++;
		
	ramp->frame_count++;
	return false;
}

size_t get_sample_size(int32 format)
{
	size_t sample;

	switch(format)
	{
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
			
		default: sample = 0;
	}
	
	return sample;
}

void media_to_gs_format(gs_audio_format* dest, media_raw_audio_format* source)
{
	dest->format = source->format;
	dest->frame_rate = source->frame_rate;
	dest->channel_count = source->channel_count;
	dest->byte_order = source->byte_order;
	dest->buffer_size = source->buffer_size;	
}

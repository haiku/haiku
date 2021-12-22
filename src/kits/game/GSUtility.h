//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
#ifndef _GAMESOUND_UTILITY_H
#define _GAMESOUND_UTILITY_H


#include <GameSoundDefs.h>
#include <MediaDefs.h>


struct _gs_ramp
{
	float inc;
	float orignal;
	float* value;

	float frame_total;
	float frame_inc;

	float frame_count;
	float frame_inc_count;

	bigtime_t duration;
};


_gs_ramp* InitRamp(float* value, float set, float frames, bigtime_t duration);
bool ChangeRamp(_gs_ramp* ramp);
size_t get_sample_size(int32 format);
void media_to_gs_format(gs_audio_format* dest,
	media_raw_audio_format* source);


template<typename T, int32 min, int32 max>
static inline T clamp(float value)
{
	if (value <= min)
		return min;
	if (value >= max)
		return max;
	return T(value);
}
#endif

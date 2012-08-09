/******************************************************************************
/
/	File:			SoundUtils.cpp
/
/   Description:	Utility functions for handling audio data.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
******************************************************************************/

#include <math.h>
#include "SoundUtils.h"

// These two conversions seem to pop up all the time in media code.
// I guess it's the curse of microsecond resolution... ;-)
double
us_to_s(bigtime_t usecs)
{
	return (usecs / 1000000.0);
}

bigtime_t
s_to_us(double secs)
{
	return (bigtime_t) (secs * 1000000.0);
}

int
bytes_per_frame(
	const media_raw_audio_format & format)
{
	//	The media_raw_audio_format format constants encode the
	//	bytes-per-sample value in the low nybble. Having a fixed
	//	number of bytes-per-sample, and no inter-sample relationships,
	//	is what makes a format "raw".
	int bytesPerSample = format.format & 0xf;
	return bytesPerSample * format.channel_count;
}

int
frames_per_buffer(
	const media_raw_audio_format & format)
{
	// This will give us the number of full-sized frames that will fit
	// in a buffer. (Remember, integer division automatically rounds
	// down.)
	int frames = 0;
	if (bytes_per_frame(format) > 0) {
		frames = format.buffer_size / bytes_per_frame(format);
	}
	return frames;
}

bigtime_t
buffer_duration(
	const media_raw_audio_format & format)
{
	//	Figuring out duration is easy. We take extra precaution to
	//	not divide by zero or return irrelevant results.
	bigtime_t duration = 0;
	if (format.buffer_size > 0 && format.frame_rate > 0 
		&& bytes_per_frame(format) > 0) {
		//	In these kinds of calculations, it's always useful to double-check
		//	the unit conversions. (Anyone remember high school physics?)
		//	bytes/(bytes/frame) / frames/sec
		//	= frames * sec/frames
		//	= secs                            which is what we want.
		duration = s_to_us((format.buffer_size / bytes_per_frame(format)) 
			/ format.frame_rate);
	}
	return duration;
}

bigtime_t
frames_duration(
	const media_raw_audio_format & format, int64 num_frames)
{
	//	Tells us how long in us it will take to produce num_frames,
	//	with the given format.
	bigtime_t duration = 0;
	if (format.frame_rate > 0) {
		duration = s_to_us(num_frames/format.frame_rate);
	}
	return duration;
}

int
buffers_for_duration(
	const media_raw_audio_format & format, bigtime_t duration)
{
	// Double-checking those unit conversions again:
	// secs * ( (frames/sec) / (frames/buffer) ) = secs * (buffers/sec) 
	// = buffers
	int buffers = 0;
	if (frames_per_buffer(format) > 0) {
		buffers = (int) ceil(us_to_s(duration)
			*(format.frame_rate/frames_per_buffer(format)));
	}
	return buffers;
}

int64
frames_for_duration(
	const media_raw_audio_format & format, bigtime_t duration)
{
	return (int64) ceil(format.frame_rate*us_to_s(duration));
}

/*
 * Copyright 2008 Stephan Aßmus, <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SUPPORT_FUNCTIONS_H
#define SUPPORT_FUNCTIONS_H

#include <MediaDefs.h>

static inline int32
bytes_per_frame(const media_format& format)
{
	int32 channelCount = format.u.raw_audio.channel_count;
	size_t sampleSize = format.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	return sampleSize * channelCount;
}


static inline bigtime_t
time_for_buffer(size_t size, const media_format& format)
{
	int32 frameSize = bytes_per_frame(format);
	float frameRate = format.u.raw_audio.frame_rate;

	return (bigtime_t)((double)size * 1000000 / frameSize / frameRate);
}


#endif // SUPPORT_FUNCTIONS_H

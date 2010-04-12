/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#ifndef AUDIO_CHANNEL_CONVERTER_H
#define AUDIO_CHANNEL_CONVERTER_H


/*! This AudioReader just converts the source channel count
	into another one, e.g. 1 -> 2. Frame rate and sample format
	remain unchanged.
*/


#include "AudioReader.h"


class AudioChannelConverter : public AudioReader {
public:
								AudioChannelConverter(AudioReader* source,
									const media_format& format);
	virtual						~AudioChannelConverter();

	virtual bigtime_t			InitialLatency() const;
	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			AudioReader*		Source() const;

protected:
			AudioReader*		fSource;
};

#endif	// AUDIO_CHANNEL_CONVERTER_H

/*
 * Copyright 2000-2006 Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#ifndef AUDIO_ADAPTER_H
#define AUDIO_ADAPTER_H


/*! This AudioReader slaves an AudioConverter and an AudioResampler
	to convert the source data to a given format.
	At this time the number of channels cannot be changed and the output
	format byte order is set to the one of the host.
	If input and output format are the same, the overhead is quit small.
*/


#include "AudioReader.h"


class AudioChannelConverter;
class AudioFormatConverter;
class AudioResampler;


class AudioAdapter : public AudioReader {
public:
								AudioAdapter(AudioReader* source,
									const media_format& format);
	virtual						~AudioAdapter();

	virtual bigtime_t			InitialLatency() const;
	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			AudioReader*		Source() const;

protected:
			void				_ConvertChannels(void* buffer,
									int64 frames) const;

			AudioReader*		fSource;
			AudioReader*		fFinalConverter;
			AudioFormatConverter* fFormatConverter;
			AudioChannelConverter* fChannelConverter;
			AudioResampler*		fResampler;
};

#endif	// AUDIO_ADAPTER_H

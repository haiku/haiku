/*
 * Copyright © 2000-2006 Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#include "AudioAdapter.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>

#include "AudioChannelConverter.h"
#include "AudioFormatConverter.h"
#include "AudioResampler.h"

using std::nothrow;

//#define TRACE_AUDIO_ADAPTER
#ifdef TRACE_AUDIO_ADAPTER
# define TRACE(x...)	printf(x)
#else
# define TRACE(x...)
#endif


AudioAdapter::AudioAdapter(AudioReader* source, const media_format& format)
	: AudioReader(format),
	  fSource(source),
	  fFinalConverter(NULL),
	  fFormatConverter(NULL),
	  fChannelConverter(NULL),
	  fResampler(NULL)
{
	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	fFormat.u.raw_audio.byte_order = hostByteOrder;
	if (source && source->Format().type == B_MEDIA_RAW_AUDIO) {

		if (fFormat.u.raw_audio.format != 0
			&& (fFormat.u.raw_audio.format
					!= source->Format().u.raw_audio.format
				|| source->Format().u.raw_audio.byte_order != hostByteOrder)) {
			TRACE("AudioAdapter() - using format converter\n");
			fFormatConverter = new (nothrow) AudioFormatConverter(source,
				fFormat.u.raw_audio.format, hostByteOrder);
			source = fFormatConverter;
		}

		if (fFormat.u.raw_audio.frame_rate != 0
			&& fFormat.u.raw_audio.frame_rate
				!= source->Format().u.raw_audio.frame_rate) {
			TRACE("AudioAdapter() - using resampler (%.1f -> %.1f)\n",
				source->Format().u.raw_audio.frame_rate,
				fFormat.u.raw_audio.frame_rate);
			fResampler = new (nothrow) AudioResampler(source,
				fFormat.u.raw_audio.frame_rate);
			source = fResampler;
		}

		if (fFormat.u.raw_audio.channel_count != 0
			&& fFormat.u.raw_audio.channel_count
				!= source->Format().u.raw_audio.channel_count) {
			TRACE("AudioAdapter() - using channel converter (%ld -> %ld)\n",
				source->Format().u.raw_audio.channel_count,
				fFormat.u.raw_audio.channel_count);
			fChannelConverter = new (nothrow) AudioChannelConverter(source,
				fFormat);
			source = fChannelConverter;
		}

		fFinalConverter = source;
	} else
		fSource = NULL;
}


AudioAdapter::~AudioAdapter()
{
	delete fFormatConverter;
	delete fChannelConverter;
	delete fResampler;
}


bigtime_t
AudioAdapter::InitialLatency() const
{
	return fSource->InitialLatency();
}


status_t
AudioAdapter::Read(void* buffer, int64 pos, int64 frames)
{
	TRACE("AudioAdapter::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	pos += fOutOffset;
	status_t ret = fFinalConverter->Read(buffer, pos, frames);
	TRACE("AudioAdapter::Read() done: %s\n", strerror(ret));
	return ret;
}


status_t
AudioAdapter::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && !fFinalConverter)
		error = B_NO_INIT;
	if (error == B_OK && fFinalConverter)
		error = fFinalConverter->InitCheck();
	return error;
}


AudioReader*
AudioAdapter::Source() const
{
	return fSource;
}


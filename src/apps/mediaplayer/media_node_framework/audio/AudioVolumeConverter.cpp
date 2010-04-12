/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */


#include "AudioVolumeConverter.h"

#include <stdio.h>
#include <string.h>

#include <MediaDefs.h>


//#define TRACE_AUDIO_CONVERTER
#ifdef TRACE_AUDIO_CONVERTER
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif


template<typename SampleType>
static void
convert(SampleType* buffer, const int32 samples, const float volume,
	const float rounding)
{
	for (int32 i = 0; i < samples; i++) {
		*buffer = (SampleType)(*buffer * volume + rounding);
		buffer++;
	}
}


template<typename SampleType>
static void
convert(SampleType* buffer, const int32 frames, const int32 channels,
	const float volume1, const float volume2, const float rounding)
{
	float volumeDiff = volume2 - volume1;
	for (int32 i = 0; i < frames; i++) {
		float volume = volume1 + volumeDiff * (i / (frames - 1));
		for (int32 k = 0; k < channels; k++) {
			*buffer = (SampleType)(*buffer * volume + rounding);
			buffer++;
		}
	}
}


// #pragma mark -


AudioVolumeConverter::AudioVolumeConverter(AudioReader* source, float volume)
	:
	AudioReader(),
	fSource(NULL),
	fVolume(volume),
	fPreviousVolume(volume)
{
	if (source && source->Format().type == B_MEDIA_RAW_AUDIO)
		fFormat = source->Format();
	else
		source = NULL;
	fSource = source;
}


AudioVolumeConverter::~AudioVolumeConverter()
{
}


bigtime_t
AudioVolumeConverter::InitialLatency() const
{
	return fSource->InitialLatency();
}


status_t
AudioVolumeConverter::Read(void* buffer, int64 pos, int64 frames)
{
	TRACE("AudioVolumeConverter::Read(%p, %lld, %lld)\n", buffer, pos, frames);
	status_t error = InitCheck();
	if (error != B_OK) {
		TRACE("AudioVolumeConverter::Read() done 1\n");
		return error;
	}
	pos += fOutOffset;

	status_t ret = fSource->Read(buffer, pos, frames);
	if (fPreviousVolume == 1.0 && fVolume == 1.0) {
		TRACE("AudioVolumeConverter::Read() done 2\n");
		return ret;
	}

	int32 channelCount = fFormat.u.raw_audio.channel_count;
	int32 samples = frames * channelCount;

	// apply volume
	switch (fSource->Format().u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			if (fVolume != fPreviousVolume) {
				convert((float*)buffer, frames, channelCount,
					fPreviousVolume, fVolume, 0.0);
			} else
				convert((float*)buffer, samples, fVolume, 0.0);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			if (fVolume != fPreviousVolume) {
				convert((int32*)buffer, frames, channelCount,
					fPreviousVolume, fVolume, 0.5);
			} else
				convert((int32*)buffer, samples, fVolume, 0.5);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			if (fVolume != fPreviousVolume) {
				convert((int16*)buffer, frames, channelCount,
					fPreviousVolume, fVolume, 0.5);
			} else
				convert((int16*)buffer, samples, fVolume, 0.5);
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR: {
			// handle this extra, because center != 0
			// (also ignores ramping the volume)
			uchar* b = (uchar*)buffer;
			for (int32 i = 0; i < samples; i++) {
				*b = (uchar)(((float)*b - 128) * fVolume + 128.5);
				b++;
			}
			break;
		}
		case media_raw_audio_format::B_AUDIO_CHAR:
			if (fVolume != fPreviousVolume) {
				convert((int8*)buffer, frames, channelCount,
					fPreviousVolume, fVolume, 0.0);
			} else
				convert((int8*)buffer, samples, fVolume, 0.5);
			break;
	}

	fPreviousVolume = fVolume;

	TRACE("AudioVolumeConverter::Read() done\n");
	return B_OK;
}


status_t
AudioVolumeConverter::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && !fSource)
		error = B_NO_INIT;
	if (error == B_OK)
		error = fSource->InitCheck();
	return error;
}


AudioReader*
AudioVolumeConverter::Source() const
{
	return fSource;
}


void
AudioVolumeConverter::SetVolume(float volume)
{
	fVolume = volume;
}


float
AudioVolumeConverter::Volume() const
{
	return fVolume;
}


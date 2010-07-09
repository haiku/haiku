/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */


#include "AudioChannelConverter.h"

#include <new>
#include <stdio.h>
#include <string.h>

using std::nothrow;


//#define TRACE_AUDIO_CONVERTER
#ifdef TRACE_AUDIO_CONVERTER
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif


template<typename Type, typename BigType>
static void
convert(Type* inBuffer, Type* outBuffer, int32 inChannels, int32 outChannels,
	int32 frames)
{
	// TODO: more conversions!
	switch (inChannels) {
		case 0:
			break;
		case 1:
			switch (outChannels) {
				case 2:
					for (int32 i = 0; i < frames; i++) {
						*outBuffer++ = *inBuffer;
						*outBuffer++ = *inBuffer++;
					}
					break;
			}
			break;
		case 2:
			switch (outChannels) {
				case 1:
					for (int32 i = 0; i < frames; i++) {
						*outBuffer++
							= (Type)((BigType)inBuffer[0] + inBuffer[1]) / 2;
						inBuffer += 2;
					}
					break;
			}
			break;
		default:
			switch (outChannels) {
				case 2:
					for (int32 i = 0; i < frames; i++) {
						outBuffer[0] = inBuffer[0];
						outBuffer[1] = inBuffer[1];
						inBuffer += outChannels;
						outBuffer += 2;
					}
					break;
			}
			break;
	}
}


// #pragma mark -


AudioChannelConverter::AudioChannelConverter(AudioReader* source,
		const media_format& format)
	: AudioReader(format),
	  fSource(source)
{
	// TODO: check the format and make sure everything matches
	// except for channel count
}


AudioChannelConverter::~AudioChannelConverter()
{
}


bigtime_t
AudioChannelConverter::InitialLatency() const
{
	return fSource->InitialLatency();
}


status_t
AudioChannelConverter::Read(void* outBuffer, int64 pos, int64 frames)
{
	TRACE("AudioChannelConverter::Read(%p, %Ld, %Ld)\n", outBuffer, pos, frames);
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	pos += fOutOffset;

	int32 inChannels = fSource->Format().u.raw_audio.channel_count;
	int32 outChannels = fFormat.u.raw_audio.channel_count;
	TRACE("   convert %ld -> %ld channels\n", inChannels, outChannels);

	int32 inSampleSize = fSource->Format().u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 inFrameSize = inSampleSize * inChannels;
	uint8* inBuffer = new (nothrow) uint8[inFrameSize * frames];

	TRACE("   fSource->Read()\n");
	status_t ret = fSource->Read(inBuffer, pos, frames);
	if (ret != B_OK) {
		delete[] inBuffer;
		return ret;
	}

	// We know that both formats are the same except for channel count
	switch (fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			convert<float, float>((float*)inBuffer, (float*)outBuffer,
				inChannels, outChannels, frames);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			convert<int32, int64>((int32*)inBuffer, (int32*)outBuffer,
				inChannels, outChannels, frames);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			convert<int16, int32>((int16*)inBuffer, (int16*)outBuffer,
				inChannels, outChannels, frames);
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			convert<uint8, uint16>((uint8*)inBuffer, (uint8*)outBuffer,
				inChannels, outChannels, frames);
			break;
		case media_raw_audio_format::B_AUDIO_CHAR:
			convert<int8, int16>((int8*)inBuffer, (int8*)outBuffer,
				inChannels, outChannels, frames);
			break;
	}

	delete[] inBuffer;

	TRACE("AudioChannelConverter::Read() done: %s\n", strerror(ret));
	return ret;
}


status_t
AudioChannelConverter::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && !fSource)
		error = B_NO_INIT;
	return error;
}


AudioReader*
AudioChannelConverter::Source() const
{
	return fSource;
}


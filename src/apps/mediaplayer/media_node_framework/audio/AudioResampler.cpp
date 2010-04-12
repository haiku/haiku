/*
 * Copyright 2000-2006 Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */


#include "AudioResampler.h"

#include <stdio.h>

#include "SampleBuffer.h"


//#define TRACE_AUDIO_RESAMPLER
#ifdef TRACE_AUDIO_RESAMPLER
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif


//! Calculates the greatest common divider of /a/ and /b/.
template<typename T> inline
T
gcd(T a, T b)
{
	while (b != 0) {
		T r = a % b;
		a = b;
		b = r;
	}
	return a;
}


template<typename Buffer>
static void
resample_linear(void* _inBuffer, void* _outBuffer, uint32 channelCount,
	double inFrameRate, double outFrameRate, int32 frames)
{
	typedef double sample_t;
	Buffer inBuffer(_inBuffer);
	Buffer outFrameBuf(_outBuffer);
	for (sample_t outFrame = 0; outFrame < frames; outFrame++) {
		// time of the out sample
		sample_t outTime = outFrame / outFrameRate;
		// first in frame
		int64 inFrame = int64(outTime * inFrameRate);
		// time of the first and the second in frame
		sample_t inTime1 = (sample_t)inFrame / inFrameRate;
		sample_t inTime2 = (sample_t)(inFrame + 1) / inFrameRate;
		// differences between the out frame time and the in frame times
		sample_t timeDiff1 = outTime - inTime1;
		sample_t timeDiff2 = inTime2 - outTime;
		sample_t timeDiff = timeDiff1 + timeDiff2;
		// pointer to the first and second in frame
		Buffer inFrameBuf1 = inBuffer + inFrame * channelCount;
		Buffer inFrameBuf2 = inFrameBuf1 + channelCount;
		for (uint32 c = 0; c < channelCount;
			 c++, inFrameBuf1++, inFrameBuf2++, outFrameBuf++) {
			// sum weighted according to the distance to the respective other
			// in frame
			outFrameBuf.WriteSample((timeDiff2 * inFrameBuf1.ReadSample()
				+ timeDiff1 * inFrameBuf2.ReadSample()) / timeDiff);
		}
	}
}


// #pragma mark -


AudioResampler::AudioResampler()
	:
	AudioReader(),
	fSource(NULL),
	fTimeScale(1.0),
	fInOffset(0)
{
}


AudioResampler::AudioResampler(AudioReader* source, float frameRate,
		float timeScale)
	:
	AudioReader(),
	fSource(NULL),
	fTimeScale(timeScale),
	fInOffset(0)
{
	SetSource(source);
	if (fSource)
		fFormat.u.raw_audio.frame_rate = frameRate;
}


AudioResampler::~AudioResampler()
{
}


bigtime_t
AudioResampler::InitialLatency() const
{
	return fSource->InitialLatency();
}


status_t
AudioResampler::Read(void* buffer, int64 pos, int64 frames)
{
	TRACE("AudioResampler::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);

	status_t error = InitCheck();
	if (error != B_OK) {
		TRACE("AudioResampler::Read() done1\n");
		return error;
	}
	// calculate position and frames in the source data
	int64 sourcePos = ConvertToSource(pos);
	int64 sourceFrames = ConvertToSource(pos + frames) - sourcePos;
	// check the frame counts
	if (sourceFrames == frames) {
		TRACE("AudioResampler::Read() done2\n");
		return fSource->Read(buffer, sourcePos, sourceFrames);
	}
	if (sourceFrames == 0) {
		ReadSilence(buffer, frames);
		TRACE("AudioResampler::Read() done3\n");
		return B_OK;
	}
	// check, if playing backwards
	bool backward = false;
	if (sourceFrames < 0) {
		sourceFrames = -sourceFrames;
		sourcePos -= sourceFrames;
		backward = true;
	}

	// we need at least two frames to interpolate
	sourceFrames += 2;
	int32 sampleSize = media_raw_audio_format::B_AUDIO_SIZE_MASK;
	uint32 channelCount = fFormat.u.raw_audio.channel_count;
	char* inBuffer = new char[sourceFrames * channelCount * sampleSize];
	error = fSource->Read(inBuffer, sourcePos, sourceFrames);
	if (error != B_OK) {
		TRACE("AudioResampler::_ReadLinear() done4\n");
		return error;
	}
	double inFrameRate = fSource->Format().u.raw_audio.frame_rate;
	double outFrameRate = (double)fFormat.u.raw_audio.frame_rate
		/ (double)fTimeScale;
	// choose the sample buffer to be used
	switch (fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			resample_linear< FloatSampleBuffer<double> >(inBuffer, buffer,
				channelCount, inFrameRate, outFrameRate, (int32)frames);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			resample_linear< IntSampleBuffer<double> >(inBuffer, buffer,
				channelCount, inFrameRate, outFrameRate, (int32)frames);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			resample_linear< ShortSampleBuffer<double> >(inBuffer, buffer,
				channelCount, inFrameRate, outFrameRate, (int32)frames);
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			resample_linear< UCharSampleBuffer<double> >(inBuffer, buffer,
				channelCount, inFrameRate, outFrameRate, (int32)frames);
			break;
		case media_raw_audio_format::B_AUDIO_CHAR:
			resample_linear< CharSampleBuffer<double> >(inBuffer, buffer,
				channelCount, inFrameRate, outFrameRate, (int32)frames);
			break;
	}
	// reverse the frame order if reading backwards
	if (backward)
		ReverseFrames(buffer, frames);
	delete[] inBuffer;
	TRACE("AudioResampler::Read() done\n");
	return B_OK;
}


status_t
AudioResampler::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && !fSource)
		error = B_NO_INIT;
	return error;
}


void
AudioResampler::SetSource(AudioReader* source)
{
	if (!source) {
		TRACE("AudioResampler::SetSource() - NULL source\n");
		return;
	}

	if (source->Format().type != B_MEDIA_RAW_AUDIO) {
		TRACE("AudioResampler::SetSource() - not B_MEDIA_RAW_AUDIO\n");
		return;
	}

	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	if (source->Format().u.raw_audio.byte_order != hostByteOrder) {
		TRACE("AudioResampler::SetSource() - not host byte order\n");
		return;
	}

	float frameRate = FrameRate();
		// don't overwrite previous audio frame rate
	fSource = source;
	fFormat = source->Format();
	fFormat.u.raw_audio.frame_rate = frameRate;
}


void
AudioResampler::SetFrameRate(float frameRate)
{
	fFormat.u.raw_audio.frame_rate = frameRate;
}


void
AudioResampler::SetTimeScale(float timeScale)
{
	fTimeScale = timeScale;
}


AudioReader*
AudioResampler::Source() const
{
	return fSource;
}


float
AudioResampler::FrameRate() const
{
	return fFormat.u.raw_audio.frame_rate;
}


float
AudioResampler::TimeScale() const
{
	return fTimeScale;
}


void
AudioResampler::SetInOffset(int64 offset)
{
	fInOffset = offset;
}


int64
AudioResampler::InOffset() const
{
	return fInOffset;
}


int64
AudioResampler::ConvertFromSource(int64 pos) const
{
	double inFrameRate = fSource->Format().u.raw_audio.frame_rate;
	double outFrameRate = fFormat.u.raw_audio.frame_rate;
	return (int64)((double)(pos - fInOffset) * outFrameRate / inFrameRate
		/ (double)fTimeScale) - fOutOffset;
}


int64
AudioResampler::ConvertToSource(int64 pos) const
{
	double inFrameRate = fSource->Format().u.raw_audio.frame_rate;
	double outFrameRate = fFormat.u.raw_audio.frame_rate;
	return (int64)((double)(pos + fOutOffset) * inFrameRate / outFrameRate
		* (double)fTimeScale) + fInOffset;
}


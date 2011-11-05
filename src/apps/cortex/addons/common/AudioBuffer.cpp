/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// AudioBuffer.cpp
// e.moon 31mar99
//

#include <Buffer.h>
#include <Debug.h>
#include <RealtimeAlloc.h>
#include "AudioBuffer.h"

#include <cmath>
#include <cstring>

#include "audio_buffer_tools.h"

const media_raw_audio_format AudioBuffer::defaultFormat = {
	44100.0,
	2,
	media_raw_audio_format::B_AUDIO_FLOAT,
	media_raw_audio_format::wildcard.byte_order,
	media_raw_audio_format::wildcard.buffer_size
};

// -------------------------------------------------------- //
// ctor/dtor/accessors
// -------------------------------------------------------- //

AudioBuffer::AudioBuffer(
	const media_raw_audio_format& format,
	rtm_pool* pFromPool) :
	
	RawBuffer(
		(format.format & 0x0f) * format.channel_count,
		0,
		true,
		pFromPool),		
	m_format(format) {}

AudioBuffer::AudioBuffer(
	const media_raw_audio_format& format,
	uint32 frames,
	bool bCircular,
	rtm_pool* pFromPool) :

	RawBuffer(
		(format.format & 0x0f) * format.channel_count,
		0,
		bCircular,
		pFromPool),
	m_format(format) {
	
	resize(frames);
}

AudioBuffer::AudioBuffer(
	const media_raw_audio_format& format,
	void* pData,
	uint32 frames,
	bool bCircular,
	rtm_pool* pFromPool) :
	
	RawBuffer(
		pData,
		(format.format & 0x0f) * format.channel_count,
		frames,
		bCircular,
		pFromPool) {}

AudioBuffer::AudioBuffer(
	const media_raw_audio_format& format,
	BBuffer* pBuffer,
	bool bCircular) :
	
	RawBuffer(),
	m_format(format)
	
	{

	if(pBuffer->Header()->type != B_MEDIA_RAW_AUDIO)
		return;
	
	// reference it:
	m_pData = pBuffer->Data();
	m_frameSize = (m_format.format & 0x0f) * m_format.channel_count;
	m_frames = pBuffer->Header()->size_used / m_frameSize;
	m_allocatedSize = 0;
	m_bOwnData = false;
	m_bCircular = bCircular;
}

// generate a reference (point) to the target's buffer
AudioBuffer::AudioBuffer(const AudioBuffer& clone) :
	RawBuffer(clone),
	m_format(clone.m_format) {}

AudioBuffer& AudioBuffer::operator=(const AudioBuffer& clone) {
	RawBuffer::operator=(clone);
	m_format = clone.m_format;
	return *this;
}

AudioBuffer::~AudioBuffer() {}

// format access

void AudioBuffer::setFormat(const media_raw_audio_format& format) {
	m_format = format;
}

const media_raw_audio_format& AudioBuffer::format() const {
	return m_format;
}

// extra adoption support

void AudioBuffer::adopt(
	const media_raw_audio_format& format,
	void* pData,
	uint32 frames,
	bool bCircular,
	rtm_pool* pFromPool) {
	
	// clean up myself first
	free();
	
	// reference
	operator=(AudioBuffer(format, pData, frames, bCircular, pFromPool));

	// mark ownership
	m_bOwnData = true;
}

// as with RawBuffer::adopt(), returns false if the target
// doesn't own its buffer, but references it anyway

bool AudioBuffer::adopt(AudioBuffer& target) {
	m_format = target.m_format;
	return RawBuffer::adopt(target);
}

// -------------------------------------------------------- //
// operations
// -------------------------------------------------------- //

// test for format equivalence against target buffer
// (ie. determine whether any conversion would be necessary
//  for copy/mix operations)

bool AudioBuffer::formatSameAs(const AudioBuffer& target) const {
	return
		m_format.format == target.m_format.format &&
		m_format.channel_count == target.m_format.channel_count;
}

// copy to target audio buffer, applying any necessary
// format conversions.  behaves like rawCopyTo().
	
uint32 AudioBuffer::copyTo(
	AudioBuffer& target,
	uint32* pioFromFrame,
	uint32* pioTargetFrame,
	uint32 frames) const {

	// simplest case:	
	if(formatSameAs(target))
		return rawCopyTo(target, pioFromFrame, pioTargetFrame, frames);

	// sanity checks
	ASSERT(m_pData);
	ASSERT(m_frames);
	ASSERT(target.m_pData);

	// figure byte offsets & sizes
	uint32 fromOffset = *pioFromFrame * m_frameSize;
	uint32 targetOffset = *pioTargetFrame * m_frameSize;
	
	uint32 size = m_frames * m_frameSize;	
	uint32 targetSize = target.m_frames * target.m_frameSize;

	// figure number of samples to convert
	uint32 toCopy = frames * m_format.channel_count;	
	if(target.m_bCircular) {
		if(toCopy > targetSize)
			toCopy = targetSize;
	} else {
		if(toCopy > (targetSize-targetOffset))
			toCopy = (targetSize-targetOffset);
	}
	uint32 remaining = toCopy;
	
	uint32 sampleSize = m_frameSize / m_format.channel_count;

	// convert and copy a sample at a time
	for(; remaining; remaining -= sampleSize) {
		convert_sample(
			(void*) *((int8*)m_pData + fromOffset),
			(void*) *((int8*)target.m_pData + targetOffset),
			m_format.format,
			target.m_format.format);
		
		fromOffset += sampleSize;
		if(fromOffset == size)
			fromOffset = 0;
		
		targetOffset += sampleSize;
		if(targetOffset == targetSize)
			targetOffset = 0;
	}
	
	// write new offsets
	*pioFromFrame = fromOffset / m_frameSize;
	*pioTargetFrame = targetOffset / m_frameSize;
	
	return toCopy;
}

uint32 AudioBuffer::copyTo(
	AudioBuffer& target,
	uint32* pioFromFrame,
	uint32* pioTargetFrame) const {
	
	return copyTo(target, pioFromFrame, pioTargetFrame, m_frames);
}

// mix to target audio buffer, applying any necessary
// format conversions.  behaves like rawCopyTo().
	
uint32 AudioBuffer::mixTo(
	AudioBuffer& target,
	uint32* pioFromFrame,
	uint32* pioTargetFrame,
	uint32 frames,
	float fGain /*=1.0*/) const { return 0; } //nyi		
	
// calculate minimum & maximum peak levels
// (converted/scaled to given type if necessary)
// pMax and pMin must point to arrays with enough room
// for one value per channel.  existing array values aren't
// cleared first.
//
// (if pMin isn't provided, the maximum absolute levels will
// be written to pMax)

void AudioBuffer::findMin(float* pMin, uint32* pAt /*=0*/) const {
	findMin(0, m_frames, pMin, pAt);
}

uint32 AudioBuffer::findMin(uint32 fromFrame, uint32 frameCount,
	float* pMin, uint32* pAt /*=0*/) const {

	size_t channels = m_format.channel_count;
	size_t samples = m_frames * channels;
	size_t bytesPerSample = m_format.format & 0x0f;

	size_t firstSample = fromFrame * channels;
	size_t remaining = frameCount * channels;

	if(!m_pData)
		return fromFrame;
		
	int8* pCur = (int8*)m_pData + (firstSample * bytesPerSample);

	uint32 n;

	if(pAt) {
		// reset pAt
		for(n = 0; n < channels; n++)
			pAt[n] = ULONG_MAX;
	}
	
	// find minimum for each channel
	for(
		n = firstSample;
		remaining;
		remaining--, n++, pCur += bytesPerSample) {
		
		// wrap around to start of buffer?
		if(n == samples) {
			pCur = (int8*)m_pData;
			n = 0;
		}
				
		float fCur = 0;
		convert_sample(pCur, fCur, m_format.format);
		
		if(fCur < pMin[n % channels]) {
			pMin[n % channels] = fCur;
			if(pAt)
				pAt[n % channels] = n / channels;
		}
	}
	
	// return current frame
	return n / channels;
}

void AudioBuffer::findMax(float* pMax, uint32* pAt /*=0*/) const {
	findMax(0, m_frames, pMax, pAt);
}

uint32 AudioBuffer::findMax(uint32 fromFrame, uint32 frameCount,
	float* pMax, uint32* pAt /*=0*/) const {

	size_t channels = m_format.channel_count;
	size_t samples = m_frames * channels;
	size_t bytesPerSample = m_format.format & 0x0f;

	size_t firstSample = fromFrame * channels;
	size_t remaining = frameCount * channels;

	if(!m_pData)
		return fromFrame;
		
	int8* pCur = (int8*)m_pData + (firstSample * bytesPerSample);

	uint32 n;

	if(pAt) {
		// reset pAt
		for(n = 0; n < channels; n++)
			pAt[n] = ULONG_MAX;
	}
	
	// find minimum for each channel
	for(
		n = firstSample;
		remaining;
		remaining--, n++, pCur += bytesPerSample) {
		
		// wrap around to start of buffer?
		if(n == samples) {
			pCur = (int8*)m_pData;
			n = 0;
		}
				
		float fCur = 0;
		convert_sample(pCur, fCur, m_format.format);
		
		if(fCur > pMax[n % channels]) {
			pMax[n % channels] = fCur;
			if(pAt)
				pAt[n % channels] = n / channels;
		}
	}
	
	// return current frame
	return n / channels;
}

void AudioBuffer::findPeaks(float* pPeaks, uint32* pAt /*=0*/) const {
	findPeaks(0, m_frames, pPeaks, pAt);
}

uint32 AudioBuffer::findPeaks(uint32 fromFrame, uint32 frameCount,
	float* pPeaks, uint32* pAt) const {
	
	size_t channels = m_format.channel_count;
	size_t samples = m_frames * channels;
	size_t bytesPerSample = m_format.format & 0x0f;

	size_t firstSample = fromFrame * channels;
	size_t remaining = frameCount * channels;

	if(!m_pData)
		return fromFrame;
	int8* pCur = (int8*)m_pData + (firstSample * bytesPerSample);

	uint32 n;

	if(pAt) {
		// reset pAt
		for(n = 0; n < channels; n++)
			pAt[n] = ULONG_MAX;
	}

	// find peaks in both directions
	for(
		n = firstSample;
		remaining;
		remaining--, n++, pCur += bytesPerSample) {
		
		// wrap around to start of buffer?
		if(n == samples) {
			pCur = (int8*)m_pData;
			n = 0;
		}
				
		float fCur = 0;
		convert_sample(pCur, fCur, m_format.format);
		
		if(fabs(fCur) > pPeaks[n % channels]) {
			pPeaks[n % channels] = fCur;
			if(pAt)
				pAt[n % channels] = n / channels;
		}
	}
	
	// return current frame
	return n / channels;
}
	
// find average level
// (converted/scaled as necessary)
// pAverage must point to an array with enough room
// for one value per channel.
	
void AudioBuffer::average(float* pAverage) const {
	average(0, m_frames, pAverage);
}

uint32 AudioBuffer::average(uint32 fromFrame, uint32 frameCount,
	float* pAverage) const {
	
	size_t channels = m_format.channel_count;
	size_t samples = m_frames * channels;
	size_t bytesPerSample = m_format.format & 0x0f;

	size_t firstSample = fromFrame * channels;
	size_t remaining = frameCount * channels;

	if(!m_pData)
		return fromFrame;
	int8* pCur = (int8*)m_pData + (firstSample * bytesPerSample);

	// clear averages
	memset(pAverage, 0, sizeof(float)*channels);
	
	// calculate
	uint32 n;
	for(
		n = firstSample;
		remaining;
		remaining--, n++, pCur += bytesPerSample) {
			
		// wrap around to start of buffer
		if(n == samples) {
			pCur = (int8*)m_pData;
			n = 0;
		}
				
		float fCur = 0;
		convert_sample(pCur, fCur, m_format.format);

		pAverage[n%channels] += fCur;
	}

	for(uint32 n = 0; n < channels; n++)
		pAverage[n] /= frameCount;	
	
	// return current frame
	return n / channels;
}

// -------------------------------------------------------- //


// END -- AudioBuffer.h --

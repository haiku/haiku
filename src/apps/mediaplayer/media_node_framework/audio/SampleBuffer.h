/*
 * Copyright © 2000-2003 Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */

// This file contains the following classes:
// * SampleBuffer
// * FloatSampleBuffer
// * IntSampleBuffer
// * ShortSampleBuffer
// * UCharSampleBuffer
// * CharSampleBuffer

#ifndef SAMPLE_BUFFER_H
#define SAMPLE_BUFFER_H

#include <SupportDefs.h>

// SampleBuffer

template<int BYTES_PER_SAMPLE>
class SampleBuffer {
 protected:
	typedef	uint8				sample_block_t[BYTES_PER_SAMPLE];

 public:
	inline						SampleBuffer(void* buffer)
									: fBuffer((sample_block_t*)buffer) { }

	inline	void				operator+=(int samples) {
		fBuffer += samples;
	}
	inline	void				operator-=(int samples) {
		fBuffer -= samples;
	}

	inline	void				operator++(int) {
		fBuffer++;
	}
	inline	void				operator--(int) {
		fBuffer--;
	}

	inline	void*				operator+(int samples) {
		return fBuffer + samples;
	}
	inline	void*				operator-(int samples) {
		return fBuffer + samples;
	}

 protected:
			sample_block_t*		fBuffer;
};


// FloatSampleBuffer

template<typename sample_t>
class FloatSampleBuffer : public SampleBuffer<sizeof(float)> {
 public:
	inline						FloatSampleBuffer(void* buffer)
									: SampleBuffer<sizeof(float)>(buffer) {
	}

	inline	sample_t			ReadSample() const {
		return *((float*)fBuffer);
	}

	inline	void				WriteSample(sample_t value) {
		*((float*)fBuffer) = value;
	}
};


// IntSampleBuffer

template<typename sample_t>
class IntSampleBuffer : public SampleBuffer<sizeof(int)> {
 public:
	inline						IntSampleBuffer(void* buffer)
									: SampleBuffer<sizeof(int)>(buffer) {
	}

	inline	sample_t			ReadSample() const {
		return (sample_t)*((int*)fBuffer) / (sample_t)0x7fffffff;
	}

	inline	void				WriteSample(sample_t value) {
		*((int*)fBuffer) = int(value * (sample_t)0x7fffffff);
	}
};


// ShortSampleBuffer

template<typename sample_t>
class ShortSampleBuffer : public SampleBuffer<sizeof(short)> {
 public:
	inline						ShortSampleBuffer(void* buffer)
									: SampleBuffer<sizeof(short)>(buffer) {
	}

	inline	sample_t			ReadSample() const {
		return (sample_t)*((short*)fBuffer) / (sample_t)32767;
	}

	inline	void				WriteSample(sample_t value) {
		*((short*)fBuffer) = short(value * (sample_t)32767);
	}
};


// UCharSampleBuffer

template<typename sample_t>
class UCharSampleBuffer : public SampleBuffer<sizeof(uchar)> {
 public:
	inline						UCharSampleBuffer(void* buffer)
									: SampleBuffer<sizeof(uchar)>(buffer) {
	}

	inline	sample_t			ReadSample() const {
		return ((sample_t)*((uchar*)fBuffer) - 128) / (sample_t)127;
	}

	inline	void				WriteSample(sample_t value) {
		*((uchar*)fBuffer) = uchar(value * (sample_t)127 + 128);
	}
};


// CharSampleBuffer

template<typename sample_t>
class CharSampleBuffer : public SampleBuffer<sizeof(char)> {
 public:
	inline						CharSampleBuffer(void* buffer)
									: SampleBuffer<sizeof(char)>(buffer) {
	}

	inline	sample_t			ReadSample() const {
		return (sample_t)*((char*)fBuffer) / (sample_t)127;
	}

	inline	void				WriteSample(sample_t value) {
		*((uchar*)fBuffer) = uchar(value * (sample_t)127);
	}
};


#endif	// SAMPLE_BUFFER_H

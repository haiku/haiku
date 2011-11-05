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


// audio_buffer_tools.h
// eamoon@meadgroup.com
//
// Some straightforward audio buffer-handling routines

#ifndef __AUDIO_BUFFER_TOOLS_H__
#define __AUDIO_BUFFER_TOOLS_H__

#include <ByteOrder.h>
#include <Debug.h>

// ---------------------------------------------------------------- //
// sample conversion +++++
// 31mar99: providing conversion to and from float, and defining
// other conversions based on that.
// ---------------------------------------------------------------- //

/*
template<class from_sample_t, class to_sample_t>
void convert_sample(from_sample_t in, to_sample_t& out) {
	out = (to_sample_t)in; // +++++ arbitrary conversion stub
}
*/

inline void convert_sample(float& in, float& out) {
	out = in;
}

inline void convert_sample(uchar& in, float& out) {
	out = (float)(in - 128) / 127.0;
}

inline void convert_sample(short& in, float& out) {
	out = (float)in / 32767.0;
}

inline void convert_sample(int32& in, float& out) {
	out = (float)in / (float)0x7fffffff;
}

inline void convert_sample(float& in, uchar& out) {
	out = (uchar)(in * 127.0);
}

inline void convert_sample(float& in, short& out) {
	out = (short)(in * 32767.0);
}

inline void convert_sample(float& in, int32& out) {
	out = (int32)(in * 0x7fffffff);
}

inline void swap_convert_sample(float& in, float& out) {
	out = B_SWAP_FLOAT(in);
}

inline void swap_convert_sample(uchar& in, float& out) {
	// no swap needed for char
	out = (float)(in - 128) / 127.0;
}

inline void swap_convert_sample(short& in, float& out) {
	out = (float)(int16)(B_SWAP_INT16(in)) / 32767.0;
}

inline void swap_convert_sample(int32& in, float& out) {
	out = (float)(int32)(B_SWAP_INT32(in)) / (float)0x7fffffff;
}

inline void swap_convert_sample(float& in, uchar& out) {
	out = (uchar)((B_SWAP_FLOAT(in)) * 127.0);
}

inline void swap_convert_sample(float& in, short& out) {
	out = (short)((B_SWAP_FLOAT(in)) * 32767.0);
}

inline void swap_convert_sample(float& in, int32& out) {
	out = (int32)((B_SWAP_FLOAT(in)) * 0x7fffffff);
}


template<class to_sample_t>
inline void convert_sample(void* pIn, to_sample_t& out, int32 in_audio_format) {
	switch(in_audio_format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			convert_sample(*(uchar*)pIn, out);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			convert_sample(*(short*)pIn, out);
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			convert_sample(*(float*)pIn, out);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			convert_sample(*(int32*)pIn, out);
			break;
		default:
			ASSERT(!"convert_sample(): bad raw_audio_format value");
	}
}

template<class from_sample_t>
inline void convert_sample(from_sample_t in, void* pOut, int32 out_audio_format) {
	switch(out_audio_format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			convert_sample(in, *(uchar*)pOut);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			convert_sample(in, *(short*)pOut);
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			convert_sample(in, *(float*)pOut);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			convert_sample(in, *(int32*)pOut);
			break;
		default:
			ASSERT(!"convert_sample(): bad raw_audio_format value");
	}
}

inline void convert_sample(void* pIn, void* pOut,
	int32 in_audio_format, int32 out_audio_format) {
	
	// simplest case
	if(in_audio_format == out_audio_format)
		return;
	
	// one-step cases
	if(in_audio_format == media_raw_audio_format::B_AUDIO_FLOAT)
		convert_sample(*(float*)pIn, pOut, out_audio_format);
		
	else if(out_audio_format == media_raw_audio_format::B_AUDIO_FLOAT)
		convert_sample(pOut, *(float*)pIn, in_audio_format);

	else {	
		// two-step cases
		float fTemp = 0;
		convert_sample(pIn, fTemp, in_audio_format);
		convert_sample(fTemp, pOut, out_audio_format);
	}
}

// ---------------------------------------------------------------- //
// data-copying helper templates
// ---------------------------------------------------------------- //

// copy from linear buffer to circular buffer; no rescaling
// returns new offset into destination buffer

template<class from_sample_t, class to_sample_t>
inline size_t copy_linear_to_circular(
	from_sample_t* pFrom,
	to_sample_t* pTo,
	size_t samples, size_t toOffset, size_t toLength) {

	ASSERT(pFrom != 0);
	ASSERT(pTo != 0);
	ASSERT(samples > 0);
	ASSERT(toLength > 0);
	ASSERT(toOffset < toLength);

	size_t n = toOffset;
	for(; samples; samples--) {
		pTo[n] = (to_sample_t)*pFrom++;
		if(++n >= toLength)
			n = 0;
	}
	
	return n;
}

// copy from a linear buffer in one sample format to a circular buffer
// in another, delegating rescaling duties to convert_sample().
// returns new offset into destination buffer

template<class from_sample_t, class to_sample_t>
inline size_t copy_linear_to_circular_convert(
	from_sample_t* pFrom,
	to_sample_t* pTo,
	size_t samples, size_t toOffset, size_t toLength) {

	ASSERT(pFrom != 0);
	ASSERT(pTo != 0);
	ASSERT(samples > 0);
	ASSERT(toLength > 0);
	ASSERT(toOffset < toLength);

	size_t n = toOffset;
	for(; samples; samples--) {
		convert_sample(*pFrom++, pTo[n]);
		if(++n >= toLength)
			n = 0;
	}
	
	return n;
}

// copy from linear buffer to circular buffer; no rescaling
// returns new offset into source buffer

template<class from_sample_t, class to_sample_t>
inline size_t copy_circular_to_linear(
	from_sample_t* pFrom,
	to_sample_t* pTo,
	size_t samples, size_t fromOffset, size_t fromLength) {

	ASSERT(pFrom != 0);
	ASSERT(pTo != 0);
	ASSERT(samples > 0);
	ASSERT(fromLength > 0);
	ASSERT(fromOffset < fromLength);

	size_t n = fromOffset;
	for(; samples; samples--) {
		*pTo++ = (to_sample_t)pFrom[n];
		if(++n >= fromLength)
			n = 0;
	}
	
	return n;
}

// copy from a circular buffer in one sample format to a linear buffer
// in another, delegating rescaling duties to convert_sample().
// returns new offset into source buffer

template<class from_sample_t, class to_sample_t>
inline size_t copy_circular_to_linear_convert(
	from_sample_t* pFrom,
	to_sample_t* pTo,
	size_t samples, size_t fromOffset, size_t fromLength) {
	
	ASSERT(pFrom != 0);
	ASSERT(pTo != 0);
	ASSERT(samples > 0);
	ASSERT(fromLength > 0);
	ASSERT(fromOffset < fromLength);

	size_t n = fromOffset;
	for(; samples; samples--) {
		convert_sample(pFrom[n], *pTo++);
		if(++n >= fromLength)
			n = 0;
	}
	
	return n;
}

//// copy between circular buffers in the same format
//// +++++ re-templatize
//
///*template<class samp_t>
//inline*/ void copy_circular_to_circular(
//	sample_t* pFrom,
//	sample_t* pTo,
//	size_t samples,
//	size_t& ioFromOffset, size_t fromLength,
//	size_t& ioToOffset, size_t toLength);

// mix from a linear buffer to a circular buffer (no rescaling)
// input samples are multiplied by fSourceGain; destination samples
// are multiplied by fFeedback
// returns new offset within destination buffer

template<class from_sample_t, class to_sample_t>
inline size_t mix_linear_to_circular(
	from_sample_t* pFrom,
	to_sample_t* pTo,
	uint32 samples,
	uint32 toOffset,
	uint32 toLength,
	float fSourceGain,
	float fFeedback) {

	// feedback
	size_t n, nLength;
	if(fFeedback == 0.0) // +++++ memset?
		for(n = toOffset, nLength = samples;
			nLength;
			n = (n+1 == toLength) ? 0 : n+1, nLength--) { pTo[n] = 0.0; }
	else if(fFeedback != 1.0)
		for(n = toOffset, nLength = samples;
			nLength;
			n = (n+1 == toLength) ? 0 : n+1, nLength--) { pTo[n] *= fFeedback; }
	// else nothing to do
		
	// mix source, unless muted or not provided
	if(pFrom && fSourceGain != 0.0) {
		if(fSourceGain == 1.0)
			for(n = toOffset, nLength = samples;
				nLength;
				n = (n+1 == toLength) ? 0 : n+1, nLength--) {
				pTo[n] += (to_sample_t)*pFrom++;
			}
		else
			for(n = toOffset, nLength = samples;
				nLength;
				n = (n+1 == toLength) ? 0 : n+1, nLength--) {
				pTo[n] += (to_sample_t)*pFrom++ * fSourceGain; // +++++ re-cast to dest format?
			}
	}
	
	// increment loop position w/ rollover
	toOffset += samples;
	if(toOffset >= toLength)
		toOffset -= toLength;
	
	return toOffset;
}

// mix from a linear buffer in one sample format to a
// circular buffer in another, delegating to convert_sample() for rescaling
// input samples are multiplied by fSourceGain; destination samples
// are multiplied by fFeedback
// returns new offset within destination buffer

template<class from_sample_t, class to_sample_t>
inline size_t mix_linear_to_circular_convert(
	from_sample_t* pFrom,
	to_sample_t* pTo,
	size_t samples,
	size_t toOffset,
	size_t toLength,
	float fSourceGain,
	float fFeedback) {

	// feedback
	size_t n, nLength;
	if(fFeedback == 0.0) // +++++ memset?
		for(n = toOffset, nLength = samples;
			nLength;
			n = (n+1 == toLength) ? 0 : n+1, nLength--) { pTo[n] = 0.0; }
	else if(fFeedback != 1.0)
		for(n = toOffset, nLength = samples;
			nLength;
			n = (n+1 == toLength) ? 0 : n+1, nLength--) { pTo[n] *= fFeedback; }
	// else nothing to do
		
	// mix source, unless muted or not provided
	if(pFrom && fSourceGain != 0.0) {
		if(fSourceGain == 1.0)
			for(n = toOffset, nLength = samples;
				nLength;
				n = (n+1 == toLength) ? 0 : n+1, nLength--) {
				to_sample_t from;
				convert_sample(*pFrom++, from);
				pTo[n] += from;
			}
		else
			for(n = toOffset, nLength = samples;
				nLength;
				n = (n+1 == toLength) ? 0 : n+1, nLength--) {
				to_sample_t from;
				convert_sample(*pFrom++, from);
				pTo[n] += from * fSourceGain; // +++++ re-cast to dest format?
			}
	}
	
	// increment loop position w/ rollover
	toOffset += samples;
	if(toOffset >= toLength)
		toOffset -= toLength;
	
	return toOffset;
}

#endif /* __AUDIO_BUFFER_TOOLS_H__ */

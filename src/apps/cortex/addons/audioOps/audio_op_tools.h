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


// audio_op_tools.h
// * PURPOSE
//   General-purpose audio processing functions.
// * HISTORY
//   e.moon		26aug99		Begun

#ifndef __audio_op_tools_H__
#define __audio_op_tools_H__

#include "AudioBuffer.h"
#include <ByteOrder.h>

// the tools

inline void byteswap_buffer_subset(
	AudioBuffer&				buffer,
	type_code						type,
	uint32							fromFrame,
	uint32							count); //nyi

inline void byteswap_sample(int8& sample) {}
inline void byteswap_sample(int16& sample) {
	sample = B_SWAP_INT16(sample);
}
inline void byteswap_sample(int32& sample) {
	sample = B_SWAP_INT32(sample);
}
inline void byteswap_sample(float& sample) {
	sample = B_SWAP_FLOAT(sample);
}

inline double fast_floor(double val) {
	return (double)(int64)val;
}
inline double fast_ceil(double val) {
	return fast_floor(val+1.0);
}

#endif /*__audio_op_tools_H__*/
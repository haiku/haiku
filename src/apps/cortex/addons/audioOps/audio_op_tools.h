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
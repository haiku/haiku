/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _INTERPOLATE_H
#define _INTERPOLATE_H


#include "Resampler.h"


class Interpolate: public Resampler {
public:
							Interpolate(uint32 sourceFormat,
								uint32 destFormat);
	virtual					~Interpolate();
private:
	virtual	void				float_to_float(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				int32_to_float(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				int16_to_float(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				int8_to_float(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				uint8_to_float(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				float_to_int32(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				float_to_int16(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				float_to_int8(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
	virtual void				float_to_uint8(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);

};


#endif

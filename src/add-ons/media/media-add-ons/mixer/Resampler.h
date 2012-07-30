/*
 * Copyright 2003 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 */
#ifndef _RESAMPLER_H
#define _RESAMPLER_H


#include <SupportDefs.h>


class Resampler {
public:
								Resampler(uint32 sourceFormat,
									uint32 destFormat);
	virtual						~Resampler();

			status_t			InitCheck() const;

			void				Resample(const void* src, int32 srcSampleOffset,
									int32 srcSampleCount, void* dest,
									int32 destSampleOffset,
									int32 destSampleCount, float gain);

protected:
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

private:
			void				(Resampler::*fFunc)(const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
};


inline void
Resampler::Resample(const void *src, int32 srcSampleOffset,
	int32 srcSampleCount, void *dest, int32 destSampleOffset,
	int32 destSampleCount, float gain)
{
	(this->*fFunc)(src, srcSampleOffset, srcSampleCount, dest, destSampleOffset,
		destSampleCount, gain);
}

#endif // _RESAMPLER_H

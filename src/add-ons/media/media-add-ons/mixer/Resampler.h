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

			status_t			InitCheck() const;

			void				Resample(const void* src, int32 srcSampleOffset,
									int32 srcSampleCount, void* dest,
									int32 destSampleOffset,
									int32 destSampleCount, float gain);

protected:
								Resampler();
			void				(*fFunc)(Resampler* object, const void* src,
									int32 srcSampleOffset, int32 srcSampleCount,
									void* dest, int32 destSampleOffset,
									int32 destSampleCount, float gain);
};


inline void
Resampler::Resample(const void *src, int32 srcSampleOffset,
	int32 srcSampleCount, void *dest, int32 destSampleOffset,
	int32 destSampleCount, float gain)
{
	(*fFunc)(this, src, srcSampleOffset, srcSampleCount, dest, destSampleOffset,
		destSampleCount, gain);
}

#endif // _RESAMPLER_H

#ifndef _RESAMPLER_H
#define _RESAMPLER_H
/* Copyright (C) 2003 Marcus Overhagen
 * Released under terms of the MIT license.
 *
 * A simple resampling class for the audio mixer.
 * You pick the conversion function on object creation,
 * and then call the Resample() function, specifying data pointer,
 * offset (in bytes) to the next sample, and count of samples for
 * both source and destination.
 *
 */

class Resampler
{
public:
	Resampler(uint32 sourceformat, uint32 destformat);
	virtual ~Resampler();
	
	status_t InitCheck();
	
	void Resample(const void *src, int32 src_sample_offset, int32 src_sample_count,
				  void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);

protected:
	virtual void float_to_float	(const void *src, int32 src_sample_offset, int32 src_sample_count,
								 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void int32_to_float	(const void *src, int32 src_sample_offset, int32 src_sample_count,
								 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void int16_to_float	(const void *src, int32 src_sample_offset, int32 src_sample_count,
							     void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void int8_to_float	(const void *src, int32 src_sample_offset, int32 src_sample_count,
				  				 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void uint8_to_float	(const void *src, int32 src_sample_offset, int32 src_sample_count,
								 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void float_to_int32	(const void *src, int32 src_sample_offset, int32 src_sample_count,
								 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void float_to_int16	(const void *src, int32 src_sample_offset, int32 src_sample_count,
				  				 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void float_to_int8	(const void *src, int32 src_sample_offset, int32 src_sample_count,
			 				  	 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
	virtual void float_to_uint8	(const void *src, int32 src_sample_offset, int32 src_sample_count,
								 void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain);
private:
	void (Resampler::*fFunc)	(const void *, int32, int32, void *, int32, int32, float);
};


inline void
Resampler::Resample(const void *src, int32 src_sample_offset, int32 src_sample_count,
					void *dst, int32 dst_sample_offset, int32 dst_sample_count, float gain)
{
	(this->*fFunc)(src, src_sample_offset, src_sample_count, dst, dst_sample_offset, dst_sample_count, gain);
}

#endif

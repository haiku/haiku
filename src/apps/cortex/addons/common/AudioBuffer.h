// AudioBuffer.h
// eamoon@meadgroup.com
// 31mar99
//
// Represents a buffer of audio data.  Derived from RawBuffer;
// adds simple audio-specific operations.

#ifndef __AudioBuffer_H__
#define __AudioBuffer_H__

#include "RawBuffer.h"
#include <MediaDefs.h>

class AudioBuffer :
	public		RawBuffer {
	typedef	RawBuffer _inherited;

public:						// constants
	static const media_raw_audio_format 		defaultFormat;
	
public:						// ctor/dtor/accessors
	virtual ~AudioBuffer();
	
	// initialize empty buffer
	AudioBuffer(
		const media_raw_audio_format& format=defaultFormat,
		rtm_pool* pFromPool=0);
	
	// allocate buffer of given format & size
	AudioBuffer(
		const media_raw_audio_format& format,
		uint32 frames,
		bool bCircular=true,
		rtm_pool* pFromPool=0);
		
	// point to given buffer
	AudioBuffer(
		const media_raw_audio_format& format,
		void* pData,
		uint32 frames,
		bool bCircular=true,
		rtm_pool* pFromPool=0);
	
	// +++++ add a flag to disallow adopt()
	AudioBuffer(
		const media_raw_audio_format& format,
		class BBuffer* pBuffer,
		bool bCircular=true);

	// generate a reference (point) to the target's buffer
	AudioBuffer(const AudioBuffer& clone);
	AudioBuffer& operator=(const AudioBuffer& clone);

	void setFormat(const media_raw_audio_format& format);
	const media_raw_audio_format& format() const;

	// extra adoption support
	void adopt(
		const media_raw_audio_format& format,
		void* pData,
		uint32 frames,
		bool bCircular=true,
		rtm_pool* pFromPool=0);
	
	// as with RawBuffer::adopt(), returns false if the target
	// doesn't own its buffer, but references it anyway
	bool adopt(
		AudioBuffer& target);
	
public:						// operations

	// test for format equivalence against target buffer
	// (ie. determine whether any conversion would be necessary
	//  for copy/mix operations)
	
	bool formatSameAs(
		const AudioBuffer& target) const;

	// copy to target audio buffer, applying any necessary
	// format conversions.  behaves like rawCopyTo().
	
	uint32 copyTo(
		AudioBuffer& target,
		uint32* pioFromFrame,
		uint32* pioTargetFrame,
		uint32 frames) const; //nyi

	// as above; copies all frames
	
	uint32 copyTo(
		AudioBuffer& target,
		uint32* pioFromFrame,
		uint32* pioTargetFrame) const;
	
	// mix to target audio buffer, applying any necessary
	// format conversions.  behaves like rawCopyTo().
	
	uint32 mixTo(
		AudioBuffer& target,
		uint32* pioFromFrame,
		uint32* pioTargetFrame,
		uint32 frames,
		float fGain=1.0) const; //nyi		
	
	// calculate minimum & maximum peak levels
	// (converted/scaled to given type if necessary)
	// pMin and pMax must point to arrays with enough room
	// for one value per channel.  existing array values aren't
	// cleared first.
	//
	// (if pMin isn't provided, the maximum _absolute_ levels will
	// be written to pMax)
	//
	// if fromFrame and frameCount are given, scans that range.
	// if pAt is given, it's expected to point to an array with
	// room for one frame value per channel: this stores the
	// frame indices at which the peak levels were found, or
	// ULONG_MAX if no peak was found for the given channel.

	void findMin(float* pMin, uint32* pAt=0) const;
	uint32 findMin(uint32 fromFrame, uint32 frameCount,
		float* pMin, uint32* pAt=0) const;

	void findMax(float* pMax, uint32* pAt=0) const;
	uint32 findMax(uint32 fromFrame, uint32 frameCount,
		float* pMax, uint32* pAt=0) const;

	void findPeaks(float* pPeaks, uint32* pAt=0) const;
	uint32 findPeaks(uint32 fromFrame, uint32 frameCount,
		float* pPeaks, uint32* pAt=0) const;
	
	// find average level
	// (converted/scaled as necessary)
	// pAverage must point to an array with enough room
	// for one value per channel.
	
	void average(float* pAverage) const; //nyi
	uint32 average(uint32 fromFrame, uint32 frameCount,
		float* pAverage) const; //nyi

protected:					// members
	media_raw_audio_format			m_format;
};

#endif /* __AudioBuffer_H__ */
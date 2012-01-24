/*
 * Copyright 2012 Jérôme Leveque
 * Copyright 2003 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 */
#ifndef _MULTI_AUDIO_RESAMPLER_H
#define _MULTI_AUDIO_RESAMPLER_H


#include <MediaDefs.h>


class Resampler {
public:
								Resampler(uint32 sourceFormat,
									uint32 destFormat);

	virtual						~Resampler();

			status_t			InitCheck() const;

			void				Resample(const void *inputData,
									uint32 inputStride, void *outputData,
									uint32 outputStride, uint32 sampleCount);

protected:
	//static functions for converting datas
	template<typename T> void _CopyType2Type(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyFloat2Double(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyFloat2Short(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyFloat2Int(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyFloat2UChar(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyFloat2Char(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);

	virtual  void			_CopyDouble2Float(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyDouble2Short(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyDouble2Int(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyDouble2UChar(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyDouble2Char(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);

	virtual  void			_CopyInt2Float(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyInt2Double(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyInt2Short(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyInt2UChar(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyInt2Char(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);

	virtual  void			_CopyShort2Float(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyShort2Double(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyShort2Int(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyShort2UChar(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyShort2Char(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);

	virtual  void			_CopyUChar2Float(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyUChar2Double(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyUChar2Short(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyUChar2Int(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyUChar2Char(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);

	virtual  void			_CopyChar2Float(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyChar2Double(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyChar2Short(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyChar2Int(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
	virtual  void			_CopyChar2UChar(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);

private:
			void			(Resampler::*fFunc)(const void *inputData,
								uint32 inputStride, void *outputData,
								uint32 outputStride, uint32 sampleCount);
};


inline void
Resampler::Resample(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	if (this->InitCheck() != B_OK)
		return;
	(this->*fFunc)(inputData, inputStride, outputData, outputStride,
		sampleCount);
}


#endif // _MULTI_AUDIO_RESAMPLER_H

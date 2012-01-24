/*
 * Copyright 2012 Jérôme Leveque
 * Copyright 2012 Jérôme Duval
 * Copyright 2003 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 */


#include "Resampler.h"
#include "MultiAudioUtility.h"
#include "debug.h"


/*!	A simple resampling class for the multi_audio add-ons.
	You pick the conversion function on object creation,
	and then call the Resample() function, specifying data pointer,
	offset (in bytes) to the next sample, and count of samples.
*/


Resampler::Resampler(uint32 sourceFormat, uint32 destFormat)
	:
	fFunc(NULL)
{
	PRINT(("Resampler() in 0x%x, out 0x%x\n", (unsigned int)sourceFormat,
		(unsigned int)destFormat));

	switch (sourceFormat) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			switch (destFormat) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					fFunc = &Resampler::_CopyType2Type<float>;
					break;
				case media_raw_audio_format::B_AUDIO_DOUBLE:
					fFunc = &Resampler::_CopyFloat2Double;
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					fFunc = &Resampler::_CopyFloat2Int;
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					fFunc = &Resampler::_CopyFloat2Short;
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					fFunc = &Resampler::_CopyFloat2UChar;
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					fFunc = &Resampler::_CopyFloat2Char;
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_DOUBLE:
			switch (destFormat) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					fFunc = &Resampler::_CopyDouble2Float;
					break;
				case media_raw_audio_format::B_AUDIO_DOUBLE:
					fFunc = &Resampler::_CopyType2Type<double>;
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					fFunc = &Resampler::_CopyDouble2Int;
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					fFunc = &Resampler::_CopyDouble2Short;
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					fFunc = &Resampler::_CopyDouble2UChar;
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					fFunc = &Resampler::_CopyDouble2Char;
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_INT:
			switch (destFormat) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					fFunc = &Resampler::_CopyInt2Float;
					break;
				case media_raw_audio_format::B_AUDIO_DOUBLE:
					fFunc = &Resampler::_CopyInt2Double;
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					fFunc = &Resampler::_CopyType2Type<int32>;
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					fFunc = &Resampler::_CopyInt2Short;
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					fFunc = &Resampler::_CopyInt2UChar;
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					fFunc = &Resampler::_CopyInt2Char;
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_SHORT:
			switch (destFormat) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					fFunc = &Resampler::_CopyShort2Float;
					break;
				case media_raw_audio_format::B_AUDIO_DOUBLE:
					fFunc = &Resampler::_CopyShort2Double;
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					fFunc = &Resampler::_CopyShort2Int;
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					fFunc = &Resampler::_CopyType2Type<int16>;
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					fFunc = &Resampler::_CopyShort2UChar;
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					fFunc = &Resampler::_CopyShort2Char;
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_UCHAR:
			switch (destFormat) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					fFunc = &Resampler::_CopyUChar2Float;
					break;
				case media_raw_audio_format::B_AUDIO_DOUBLE:
					fFunc = &Resampler::_CopyUChar2Double;
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					fFunc = &Resampler::_CopyUChar2Int;
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					fFunc = &Resampler::_CopyUChar2Short;
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					fFunc = &Resampler::_CopyType2Type<uint8>;
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					fFunc = &Resampler::_CopyUChar2Char;
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_CHAR:
			switch (destFormat) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					fFunc = &Resampler::_CopyChar2Float;
					break;
				case media_raw_audio_format::B_AUDIO_DOUBLE:
					fFunc = &Resampler::_CopyChar2Double;
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					fFunc = &Resampler::_CopyChar2Int;
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					fFunc = &Resampler::_CopyChar2Short;
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					fFunc = &Resampler::_CopyChar2UChar;
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					fFunc = &Resampler::_CopyType2Type<int8>;
					break;
			}
			break;
	}
}


Resampler::~Resampler()
{
}


status_t
Resampler::InitCheck() const
{
	return fFunc != NULL ? B_OK : B_ERROR;
}


template<typename T>
void
Resampler::_CopyType2Type(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(T*)outputData = *(const T*)inputData;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyFloat2Double(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(double*)outputData = *(const float*)inputData;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyFloat2Int(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int32*)outputData = (int32)(*(const float*)inputData * 2147483647.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}

void
Resampler::_CopyFloat2Short(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int16*)outputData = (int16)(*(const float*)inputData * 32767.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyFloat2UChar(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(uint8*)outputData = (uint8)(128.0f + *(const float*)inputData * 127.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyFloat2Char(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int8*)outputData = (int8)(*(const float*)inputData * 127.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyDouble2Float(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(float*)outputData = *(const double*)inputData;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyDouble2Int(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int32*)outputData = (int32)(*(const double*)inputData * 2147483647.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyDouble2Short(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int16*)outputData = (int16)(*(const double*)inputData * 32767.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyDouble2UChar(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(uint8*)outputData = (uint8)(128.0f + *(const double*)inputData * 127.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyDouble2Char(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int8*)outputData = (int8)(*(const double*)inputData * 127.0f);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyShort2Float(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(float*)outputData = *(const int16*)inputData / 32767.0f;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyShort2Double(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(double*)outputData = *(const int16*)inputData / 32767.0;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyShort2Int(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int32*)outputData = (int32)*(const int16*)inputData << 16;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyShort2UChar(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(uint8*)outputData = 128 + (*(const int16*)inputData >> 8);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyShort2Char(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int8*)outputData = *(const int16*)inputData >> 8;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyInt2Float(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(float*)outputData = *(const int32*)inputData / 2147483647.0f;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyInt2Double(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(double*)outputData = *(const int32*)inputData / 2147483647.0;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyInt2Short(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int16*)outputData = *(const int32*)inputData >> 16;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyInt2UChar(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(uint8*)outputData = 128 + (*(const int32*)inputData >> 24);

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyInt2Char(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(uint8*)outputData = *(const int32*)inputData >> 24;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyUChar2Float(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(float*)outputData = (*(const uint8*)inputData - 128) / 127.0f;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyUChar2Double(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(double*)outputData = (*(const uint8*)inputData - 128) / 127.0;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyUChar2Short(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int16*)outputData = (int16)(*(const uint8*)inputData - 128) << 8;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyUChar2Int(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int32*)outputData = (int32)(*(const uint8*)inputData - 128) << 24;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyUChar2Char(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int8*)outputData = *(const uint8*)inputData - 128;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyChar2Float(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(float*)outputData = *(const int8*)inputData / 127.0f;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyChar2Double(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(double*)outputData = *(const int8*)inputData / 127.0;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyChar2Short(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int16*)outputData = ((int16)*(const int8*)inputData) << 8;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyChar2Int(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(int32*)outputData = ((int16)*(const int8*)inputData) << 24;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


void
Resampler::_CopyChar2UChar(const void *inputData, uint32 inputStride,
	void *outputData, uint32 outputStride, uint32 sampleCount)
{
	while (sampleCount > 0) {
		*(uint8*)outputData = *(const int8*)inputData + 128;

		outputData = (void*)((uint8*)outputData + outputStride);
		inputData = (void*)((uint8*)inputData + inputStride);

		sampleCount--;
	}
}


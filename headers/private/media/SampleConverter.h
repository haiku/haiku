#ifndef _SAMPLE_CONVERTER_
#define _SAMPLE_CONVERTER_

/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SampleConverter.h
 *  DESCR: Converts between different sample formats:
 *           media_raw_audio_format::B_AUDIO_FLOAT
 *           media_raw_audio_format::B_AUDIO_INT
 *           media_raw_audio_format::B_AUDIO_SHORT
 *           media_raw_audio_format::B_AUDIO_CHAR
 *           media_raw_audio_format::B_AUDIO_UCHAR
 ***********************************************************************/

namespace MediaKitPrivate {

class SampleConverter
{
public:
	static status_t 
	convert(void *dest, uint32 dest_format, 
            const void *source, uint32 source_format, 
            int samplecount);

	static void convert(uint8 *dest, const float *source, int samplecount);
	static void convert(uint8 *dest, const int32 *source, int samplecount);
	static void convert(uint8 *dest, const int16 *source, int samplecount);
	static void convert(uint8 *dest, const int8 *source, int samplecount);

	static void convert(int8 *dest, const float *source, int samplecount);
	static void convert(int8 *dest, const int32 *source, int samplecount);
	static void convert(int8 *dest, const int16 *source, int samplecount);
	static void convert(int8 *dest, const uint8 *source, int samplecount);

	static void convert(int16 *dest, const float *source, int samplecount);
	static void convert(int16 *dest, const int32 *source, int samplecount);
	static void convert(int16 *dest, const int8 *source, int samplecount);
	static void convert(int16 *dest, const uint8 *source, int samplecount);

	static void convert(int32 *dest, const float *source, int samplecount);
	static void convert(int32 *dest, const int16 *source, int samplecount);
	static void convert(int32 *dest, const int8 *source, int samplecount);
	static void convert(int32 *dest, const uint8 *source, int samplecount);

	static void convert(float *dest, const int32 *source, int samplecount);
	static void convert(float *dest, const int16 *source, int samplecount);
	static void convert(float *dest, const int8 *source, int samplecount);
	static void convert(float *dest, const uint8 *source, int samplecount);
};

} //namespace MediaKitPrivate

#endif

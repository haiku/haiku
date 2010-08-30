/*
 * Copyright 2000-2006 Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */


#include "AudioFormatConverter.h"

#include <ByteOrder.h>
#include <MediaDefs.h>


//#define TRACE_AUDIO_CONVERTER
#ifdef TRACE_AUDIO_CONVERTER
#	include <stdio.h>
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif


struct ReadFloat {
	inline int operator()(const void* buffer) const {
		// 0 == mid, -1.0 == bottom, 1.0 == top
		float b = *(float*)buffer;
		if (b < -1.0f)
			b = -1.0f;
		else if (b > 1.0f)
			b = 1.0f;
		return (int)((double)b * (double)0x7fffffff);
	}
};

struct ReadInt {
	inline int operator()(const void* buffer) const {
		// 0 == mid, 0x80000001 == bottom, 0x7fffffff == top
		int b = *(int*)buffer;
		if (b == INT_MIN)
			b++;
		return b;
	}
};

struct ReadShort {
	inline int operator()(const void* buffer) const {
		// 0 == mid, -32767 == bottom, +32767
		short b = *(short*)buffer;
		if (b == -32768)
			b++;
		return int(int64(b) * 0x7fffffff / 32767);
	}
};

struct ReadUChar {
	inline int operator()(const void* buffer) const {
		// 128 == mid, 1 == bottom, 255 == top
		uchar b = *(uchar*)buffer;
		if (b == 0)
			b++;
		return int((int64(b) - 0x80) * 0x7fffffff / 127);
	}
};

struct ReadChar {
	inline int operator()(const void* buffer) const {
		// 0 == mid, -127 == bottom, +127 == top
		char b = *(char*)buffer;
		if (b == 0)
			b++;
		return int(int64(b) * 0x7fffffff / 127);
	}
};

struct WriteFloat {
	inline void operator()(void* buffer, int value) const {
		*(float*)buffer = (double)value / (double)0x7fffffff;
	}

};

struct WriteInt {
	inline void operator()(void* buffer, int value) const {
		*(int*)buffer = value;
	}
};

struct WriteShort {
	inline void operator()(void* buffer, int value) const {
		*(short*)buffer = (short)(value / (int)0x10000);
	}
};

struct WriteUChar {
	inline void operator()(void* buffer, int value) const {
		*(uchar*)buffer = (uchar)(value / (int)0x1000000 + 128);
	}
};

struct WriteChar {
	inline void operator()(void* buffer, int value) const {
		*(char*)buffer = (char)(value / (int)0x1000000);
	}
};


template<typename ReadT, typename WriteT>
static void
convert(const ReadT& read, const WriteT& write,
		const char* inBuffer, char* outBuffer, int32 frames,
		int32 inSampleSize, int32 outSampleSize, int32 channelCount)
{
	for (int32 i = 0; i < frames; i++) {
		for (int32 c = 0; c < channelCount; c++) {
			write(outBuffer, read(inBuffer));
			inBuffer += inSampleSize;
			outBuffer += outSampleSize;
		}
	}
}


static void
swap_sample_byte_order(void* buffer, uint32 format, size_t length)
{
	type_code type = B_ANY_TYPE;
	switch (format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			type = B_FLOAT_TYPE;
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			type = B_INT32_TYPE;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			type = B_INT16_TYPE;
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			break;
		case media_raw_audio_format::B_AUDIO_CHAR:
			break;
	}
	if (type != B_ANY_TYPE)
		swap_data(type, buffer, length, B_SWAP_ALWAYS);
}


// #pragma mark -


AudioFormatConverter::AudioFormatConverter(AudioReader* source, uint32 format,
		uint32 byteOrder)
	:
	AudioReader(),
	fSource(NULL)
{
	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	if (source && source->Format().type == B_MEDIA_RAW_AUDIO
		&& source->Format().u.raw_audio.byte_order == hostByteOrder) {
		fFormat = source->Format();
		fFormat.u.raw_audio.format = format;
		fFormat.u.raw_audio.byte_order = byteOrder;
		int32 inSampleSize = source->Format().u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK;
		int32 outSampleSize = fFormat.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK;
		if (inSampleSize != outSampleSize) {
			fFormat.u.raw_audio.buffer_size
				= source->Format().u.raw_audio.buffer_size * outSampleSize
				  / inSampleSize;
		}
	} else
		source = NULL;
	fSource = source;
}


AudioFormatConverter::~AudioFormatConverter()
{
}


bigtime_t
AudioFormatConverter::InitialLatency() const
{
	return fSource->InitialLatency();
}

status_t
AudioFormatConverter::Read(void* buffer, int64 pos, int64 frames)
{
	TRACE("AudioFormatConverter::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);
	status_t error = InitCheck();
	if (error != B_OK) {
		TRACE("AudioFormatConverter::Read() done 1\n");
		return error;
	}
	pos += fOutOffset;

	if (fFormat.u.raw_audio.format == fSource->Format().u.raw_audio.format
		&& fFormat.u.raw_audio.byte_order
		   == fSource->Format().u.raw_audio.byte_order) {
		TRACE("AudioFormatConverter::Read() done 2\n");
		return fSource->Read(buffer, pos, frames);
	}

	int32 inSampleSize = fSource->Format().u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 outSampleSize = fFormat.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 channelCount = fFormat.u.raw_audio.channel_count;
	int32 inFrameSize = inSampleSize * channelCount;
	int32 outFrameSize = outSampleSize * channelCount;
	char* reformatBuffer = NULL;
	char* inBuffer = (char*)buffer;

	#ifdef TRACE_AUDIO_CONVERTER
		char formatString[256];
		string_for_format(fSource->Format(), formatString, 256);
		TRACE("  source format: %s\n", formatString);
		TRACE("  in format : format: %lx, sample size: %ld, channels: %ld, "
			"byte order: %lu\n", fSource->Format().u.raw_audio.format,
			inSampleSize, channelCount,
			fSource->Format().u.raw_audio.byte_order);
		TRACE("  out format: format: %lx, sample size: %ld, channels: %ld, "
			"byte order: %lu\n", fFormat.u.raw_audio.format, outSampleSize,
			channelCount, fFormat.u.raw_audio.byte_order);
	#endif // TRACE_AUDIO_CONVERTER

	if (inSampleSize != outSampleSize) {
		reformatBuffer = new char[frames * inFrameSize];
		inBuffer = reformatBuffer;
	}
	error = fSource->Read(inBuffer, pos, frames);
	// convert samples to host endianess
	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	if (fSource->Format().u.raw_audio.byte_order != hostByteOrder) {
		swap_sample_byte_order(inBuffer, fSource->Format().u.raw_audio.format,
							   frames * inFrameSize);
	}
	// convert the sample type
	switch (fSource->Format().u.raw_audio.format) {
		// float
		case media_raw_audio_format::B_AUDIO_FLOAT:
			switch (fFormat.u.raw_audio.format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					convert(ReadFloat(), WriteInt(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert(ReadFloat(), WriteShort(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert(ReadFloat(), WriteUChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert(ReadFloat(), WriteChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
			}
			break;
		// int
		case media_raw_audio_format::B_AUDIO_INT:
			switch (fFormat.u.raw_audio.format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert(ReadInt(), WriteFloat(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert(ReadInt(), WriteShort(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert(ReadInt(), WriteUChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert(ReadInt(), WriteChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
			}
			break;
		// short
		case media_raw_audio_format::B_AUDIO_SHORT:
			switch (fFormat.u.raw_audio.format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert(ReadShort(), WriteFloat(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					convert(ReadShort(), WriteInt(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert(ReadShort(), WriteUChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert(ReadShort(), WriteChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
			}
			break;
		// uchar
		case media_raw_audio_format::B_AUDIO_UCHAR:
			switch (fFormat.u.raw_audio.format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert(ReadUChar(), WriteFloat(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					convert(ReadUChar(), WriteInt(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert(ReadUChar(), WriteShort(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					convert(ReadUChar(), WriteChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
			}
			break;
		// char
		case media_raw_audio_format::B_AUDIO_CHAR:
			switch (fFormat.u.raw_audio.format) {
				case media_raw_audio_format::B_AUDIO_FLOAT:
					convert(ReadChar(), WriteFloat(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					convert(ReadChar(), WriteInt(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					convert(ReadChar(), WriteShort(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_UCHAR:
					convert(ReadChar(), WriteUChar(), inBuffer, (char*)buffer,
							frames, inSampleSize, outSampleSize, channelCount);
					break;
				case media_raw_audio_format::B_AUDIO_CHAR:
					break;
			}
			break;
	}
	// convert samples to output endianess
	if (fFormat.u.raw_audio.byte_order != hostByteOrder) {
		swap_sample_byte_order(buffer, fFormat.u.raw_audio.format,
							   frames * outFrameSize);
	}

	delete[] reformatBuffer;
	TRACE("AudioFormatConverter::Read() done\n");
	return B_OK;
}


status_t
AudioFormatConverter::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && !fSource)
		error = B_NO_INIT;
	if (error == B_OK)
		error = fSource->InitCheck();
	return error;
}


AudioReader*
AudioFormatConverter::Source() const
{
	return fSource;
}


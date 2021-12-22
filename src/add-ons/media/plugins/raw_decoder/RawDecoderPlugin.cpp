/*
 * Copyright (c) 2003-2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <DataIO.h>
#include <OS.h>
#include <MediaRoster.h>
#include <ReaderPlugin.h>

#include "RawFormats.h"
#include "RawDecoderPlugin.h"
#include "AudioConversion.h"

//#define TRACE_RAW_DECODER
#ifdef TRACE_RAW_DECODER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

inline size_t
AudioBufferSize(int32 channel_count, uint32 sample_format, float frame_rate, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (sample_format & 0xf) * channel_count * (size_t)((frame_rate * buffer_duration) / 1000000.0);
}


void
RawDecoder::GetCodecInfo(media_codec_info *info)
{
	strcpy(info->short_name, "raw");

	if (fInputFormat.IsAudio())
		strcpy(info->pretty_name, "Raw audio decoder");
	else
		strcpy(info->pretty_name, "Raw video decoder");
}


status_t
RawDecoder::Setup(media_format *ioEncodedFormat,
				  const void *infoBuffer, size_t infoSize)
{
	char s[200];
	string_for_format(*ioEncodedFormat, s, sizeof(s));
	TRACE("RawDecoder::Setup: %s\n", s);

	if (ioEncodedFormat->type != B_MEDIA_RAW_AUDIO && ioEncodedFormat->type != B_MEDIA_RAW_VIDEO)
		return B_ERROR;

	fInputFormat = *ioEncodedFormat;

	if (ioEncodedFormat->type == B_MEDIA_RAW_VIDEO) {
		fInputSampleSize = ioEncodedFormat->u.raw_video.display.line_count * ioEncodedFormat->u.raw_video.display.bytes_per_row;
		fInputFrameSize = fInputSampleSize;
	} else {
		fInputSampleSize = (ioEncodedFormat->u.raw_audio.format & B_AUDIO_FORMAT_SIZE_MASK);
		fInputFrameSize = fInputSampleSize * ioEncodedFormat->u.raw_audio.channel_count;
	}

	// since ioEncodedFormat is later passed to the application by BMediaTrack::EncodedFormat()
	// we need to remove non public format specifications

	// remove non format bits, like channel order
	ioEncodedFormat->u.raw_audio.format &= B_AUDIO_FORMAT_MASK;

	switch (ioEncodedFormat->u.raw_audio.format) {
		case B_AUDIO_FORMAT_UINT8:
		case B_AUDIO_FORMAT_INT8:
		case B_AUDIO_FORMAT_INT16:
		case B_AUDIO_FORMAT_INT32:
		case B_AUDIO_FORMAT_FLOAT32:
			break; // ok to pass through

		case B_AUDIO_FORMAT_INT24:
			ioEncodedFormat->u.raw_audio.format = B_AUDIO_FORMAT_INT32;
			break;

		case B_AUDIO_FORMAT_FLOAT64:
			ioEncodedFormat->u.raw_audio.format = B_AUDIO_FORMAT_FLOAT32;
			break;

		default:
			TRACE("RawDecoder::Setup: unknown input format\n");
			return B_ERROR;
	}

	// since we can translate to a different buffer size,
	// suggest something nicer than delivered by the
	// file reader (perhaps we should even report wildcard?)
	// I don't believe we can negotiate the buffer size with the reader
//	ioEncodedFormat->u.raw_audio.buffer_size = AudioBufferSize(
//														ioEncodedFormat->u.raw_audio.channel_count,
//														ioEncodedFormat->u.raw_audio.format,
//														ioEncodedFormat->u.raw_audio.frame_rate);
	return B_OK;
}


status_t
RawDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	// BeBook says: The codec will find and return in ioFormat its best matching format
	// => This means, we never return an error, and always change the format values
	//    that we don't support to something more applicable
	if (fInputFormat.type == B_MEDIA_RAW_VIDEO)
		return NegotiateVideoOutputFormat(ioDecodedFormat);
	if (fInputFormat.type == B_MEDIA_RAW_AUDIO)
		return NegotiateAudioOutputFormat(ioDecodedFormat);
	debugger("RawDecoder::NegotiateOutputFormat: wrong encoded format type");
	return B_ERROR;
}


status_t
RawDecoder::NegotiateVideoOutputFormat(media_format *ioDecodedFormat)
{
	return B_ERROR;
}


status_t
RawDecoder::NegotiateAudioOutputFormat(media_format *ioDecodedFormat)
{
	char s[1024];

	TRACE("RawDecoder::NegotiateAudioOutputFormat enter:\n");

	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	switch (ioDecodedFormat->u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
		case media_raw_audio_format::B_AUDIO_SHORT:
		case media_raw_audio_format::B_AUDIO_UCHAR:
		case media_raw_audio_format::B_AUDIO_CHAR:
			ioDecodedFormat->u.raw_audio.valid_bits = 0;
			break; // we can produce this on request

		case media_raw_audio_format::B_AUDIO_INT:
			ioDecodedFormat->u.raw_audio.valid_bits = fInputFormat.u.raw_audio.valid_bits;
			break; // we can produce this on request

		default:
			switch (fInputFormat.u.raw_audio.format & B_AUDIO_FORMAT_MASK) {
				case media_raw_audio_format::B_AUDIO_SHORT:
				case media_raw_audio_format::B_AUDIO_UCHAR:
				case media_raw_audio_format::B_AUDIO_CHAR:
					ioDecodedFormat->u.raw_audio.format = fInputFormat.u.raw_audio.format & B_AUDIO_FORMAT_MASK;
					ioDecodedFormat->u.raw_audio.valid_bits = 0;
					break;

				case media_raw_audio_format::B_AUDIO_INT:
				case B_AUDIO_FORMAT_INT24:
					ioDecodedFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_INT;
					ioDecodedFormat->u.raw_audio.valid_bits = fInputFormat.u.raw_audio.valid_bits;
					break;

				case media_raw_audio_format::B_AUDIO_FLOAT:
				case B_AUDIO_FORMAT_FLOAT64:
				default:
					ioDecodedFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
					ioDecodedFormat->u.raw_audio.valid_bits = 0;
					break;
			}
			break;
	}

	ioDecodedFormat->u.raw_audio.frame_rate = fInputFormat.u.raw_audio.frame_rate;
	ioDecodedFormat->u.raw_audio.channel_count = fInputFormat.u.raw_audio.channel_count;

	fFrameRate = (int32) ioDecodedFormat->u.raw_audio.frame_rate;

	fOutputSampleSize = (ioDecodedFormat->u.raw_audio.format & B_AUDIO_FORMAT_SIZE_MASK);
	fOutputFrameSize = fOutputSampleSize * ioDecodedFormat->u.raw_audio.channel_count;

	if (ioDecodedFormat->u.raw_audio.byte_order == 0)
		ioDecodedFormat->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;

	ioDecodedFormat->u.raw_audio.channel_mask = 0;
	ioDecodedFormat->u.raw_audio.matrix_mask = 0;

	ioDecodedFormat->u.raw_audio.buffer_size = fInputFormat.u.raw_audio.buffer_size;

// I don't believe we can negotiate the buffer size with the reader
// the decoder might use a different buffer for output but it must read all bytes given.
//	if (ioDecodedFormat->u.raw_audio.buffer_size < 128 || ioDecodedFormat->u.raw_audio.buffer_size > 65536) {
//		ioDecodedFormat->u.raw_audio.buffer_size = AudioBufferSize(
//														ioDecodedFormat->u.raw_audio.channel_count,
//														ioDecodedFormat->u.raw_audio.format,
//														ioDecodedFormat->u.raw_audio.frame_rate);
//	} else {
		// round down to exact multiple of output frame size
//		ioDecodedFormat->u.raw_audio.buffer_size = (ioDecodedFormat->u.raw_audio.buffer_size / fOutputFrameSize) * fOutputFrameSize;
//	}

	fOutputBufferFrameCount = ioDecodedFormat->u.raw_audio.buffer_size / fOutputFrameSize;

	// setup input swapping function
	if (fInputFormat.u.raw_audio.byte_order == B_MEDIA_HOST_ENDIAN) {
		fSwapInput = 0;
	} else {
		switch (fInputFormat.u.raw_audio.format & B_AUDIO_FORMAT_MASK) {
			case B_AUDIO_FORMAT_INT16:
				fSwapInput = &swap_int16;
				break;
			case B_AUDIO_FORMAT_INT24:
				fSwapInput = &swap_int24;
				break;
			case B_AUDIO_FORMAT_INT32:
				fSwapInput = &swap_int32;
				break;
			case B_AUDIO_FORMAT_FLOAT32:
				fSwapInput = &swap_float32;
				break;
			case B_AUDIO_FORMAT_FLOAT64:
				fSwapInput = &swap_float64;
				break;
			case B_AUDIO_FORMAT_UINT8:
			case B_AUDIO_FORMAT_INT8:
				fSwapInput = 0;
				break;
			default:
				debugger("RawDecoder::NegotiateAudioOutputFormat unknown input format\n");
				break;
		}
	}

	// setup output swapping function
	if (ioDecodedFormat->u.raw_audio.byte_order == B_MEDIA_HOST_ENDIAN) {
		fSwapOutput = 0;
	} else {
		switch (ioDecodedFormat->u.raw_audio.format) {
			case B_AUDIO_FORMAT_INT16:
				fSwapOutput = &swap_int16;
				break;
			case B_AUDIO_FORMAT_INT32:
				fSwapOutput = &swap_int32;
				break;
			case B_AUDIO_FORMAT_FLOAT32:
				fSwapOutput = &swap_float32;
				break;
			case B_AUDIO_FORMAT_UINT8:
			case B_AUDIO_FORMAT_INT8:
				fSwapOutput = 0;
				break;
			default:
				debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
				break;
		}
	}

	// setup sample conversation function
	switch (fInputFormat.u.raw_audio.format & B_AUDIO_FORMAT_MASK) {
		case B_AUDIO_FORMAT_UINT8:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &uint8_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &uint8_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &uint8_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &uint8_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &uint8_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		case B_AUDIO_FORMAT_INT8:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &int8_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &int8_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &int8_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &int8_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &int8_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		case B_AUDIO_FORMAT_INT16:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &int16_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &int16_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &int16_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &int16_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &int16_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		case B_AUDIO_FORMAT_INT24:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &int24_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &int24_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &int24_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &int24_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &int24_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		case B_AUDIO_FORMAT_INT32:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &int32_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &int32_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &int32_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &int32_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &int32_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		case B_AUDIO_FORMAT_FLOAT32:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &float32_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &float32_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &float32_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &float32_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &float32_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		case B_AUDIO_FORMAT_FLOAT64:
			switch (ioDecodedFormat->u.raw_audio.format) {
				case B_AUDIO_FORMAT_UINT8:
					fConvert = &float64_to_uint8;
					break;
				case B_AUDIO_FORMAT_INT8:
					fConvert = &float64_to_int8;
					break;
				case B_AUDIO_FORMAT_INT16:
					fConvert = &float64_to_int16;
					break;
				case B_AUDIO_FORMAT_INT32:
					fConvert = &float64_to_int32;
					break;
				case B_AUDIO_FORMAT_FLOAT32:
					fConvert = &float64_to_float32;
					break;
				default:
					debugger("RawDecoder::NegotiateAudioOutputFormat unknown output format\n");
					break;
			}
			break;

		default:
			debugger("RawDecoder::NegotiateAudioOutputFormat unknown input format\n");
			break;
	}

	fChunkBuffer = 0;
	fChunkSize = 0;
	fStartTime = 0;

	string_for_format(*ioDecodedFormat, s, sizeof(s));
	TRACE("RawDecoder::NegotiateAudioOutputFormat leave: %s\n", s);

	if (ioDecodedFormat->type == 0)
		debugger("RawDecoder::NegotiateAudioOutputFormat ioDecodedFormat->type == 0");
/*
	TRACE("fFrameRate              %ld\n", fFrameRate);
	TRACE("fInputFrameSize         %ld\n", fInputFrameSize);
	TRACE("fOutputFrameSize        %ld\n", fOutputFrameSize);
	TRACE("fInputSampleSize        %ld\n", fInputSampleSize);
	TRACE("fOutputSampleSize       %ld\n", fOutputSampleSize);
	TRACE("fOutputBufferFrameCount %ld\n", fOutputBufferFrameCount);
	TRACE("fSwapInput              %p\n", fSwapInput);
	TRACE("fConvert                %p\n", fConvert);
	TRACE("fSwapOutput             %p\n", fSwapOutput);
*/
	return B_OK;
}


status_t
RawDecoder::SeekedTo(int64 frame, bigtime_t time)
{
	fChunkSize = 0;
	
	TRACE("RawDecoder::SeekedTo called\n");

	fStartTime = time;

	return B_OK;
}


status_t
RawDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
	char *output_buffer = (char *)buffer;
	mediaHeader->start_time = fStartTime;
	*frameCount = 0;
	while (*frameCount < fOutputBufferFrameCount) {
		if (fChunkSize == 0) {
			media_header mh;
			status_t err;
			err = GetNextChunk(&fChunkBuffer, &fChunkSize, &mh);
			if (err != B_OK || fChunkSize < (size_t)fInputFrameSize) {
				fChunkSize = 0;
				break;
			}
			if (fSwapInput)
				fSwapInput(const_cast<void *>(fChunkBuffer), fChunkSize / fInputSampleSize); // XXX TODO! FIX THIS, we write to a const buffer!!!
			fStartTime = mh.start_time;
			continue;
		}
		int32 frames = min_c(fOutputBufferFrameCount - *frameCount,
			(int64)(fChunkSize / fInputFrameSize));
		if (frames == 0)
			break;

		int32 samples = frames * fInputFormat.u.raw_audio.channel_count;
		fConvert(output_buffer, fChunkBuffer, samples);
		fChunkBuffer = (const char *)fChunkBuffer + frames * fInputFrameSize;
		fChunkSize -= frames * fInputFrameSize;
		output_buffer += frames * fOutputFrameSize;
		*frameCount += frames;
		fStartTime += (1000000LL * frames) / fFrameRate;
	}
	// XXX should change channel order here for
	// B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE and B_AUDIO_FORMAT_CHANNEL_ORDER_AIFF

	if (fSwapOutput)
		fSwapOutput(buffer, *frameCount * fInputFormat.u.raw_audio.channel_count);
	
	TRACE("framecount %Ld, time %Ld\n",*frameCount, mediaHeader->start_time);
		
	return *frameCount ? B_OK : B_ERROR;
}


Decoder *
RawDecoderPlugin::NewDecoder(uint index)
{
	return new RawDecoder;
}


static media_format raw_formats[2];

status_t
RawDecoderPlugin::GetSupportedFormats(media_format ** formats, size_t * count)
{
	BMediaFormats mediaFormats;
	media_format_description description;
	media_format format;

	// audio decoder

	description.family = B_BEOS_FORMAT_FAMILY;
	description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_multi_audio_format::wildcard;

	status_t status = mediaFormats.MakeFormatFor(&description, 1, &format);
	if (status < B_OK)
		return status;
	raw_formats[0] = format;

	// video decoder

	description.u.beos.format = B_BEOS_FORMAT_RAW_VIDEO;
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video = media_raw_video_format::wildcard;

	status = mediaFormats.MakeFormatFor(&description, 1, &format);
	if (status < B_OK)
		return status;
	raw_formats[1] = format;

	*formats = raw_formats;
	*count = 2;

	return B_OK;
}

MediaPlugin *instantiate_plugin()
{
	return new RawDecoderPlugin;
}

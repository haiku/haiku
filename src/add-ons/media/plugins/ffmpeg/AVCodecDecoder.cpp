/*
 * Copyright (C) 2001 Carlos Hasan
 * Copyright (C) 2001 François Revol
 * Copyright (C) 2001 Axel Dörfler
 * Copyright (C) 2004 Marcus Overhagen
 * Copyright (C) 2009 Stephan Amßus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//! libavcodec based decoder for Haiku

#include "AVCodecDecoder.h"

#include <new>

#include <string.h>

#include <Bitmap.h>
#include <Debug.h>


#undef TRACE
//#define TRACE_AV_CODEC
#ifdef TRACE_AV_CODEC
#	define TRACE(x...)	printf(x)
#	define TRACE_AUDIO(x...)	printf(x)
#	define TRACE_VIDEO(x...)	printf(x)
#else
#	define TRACE(x...)
#	define TRACE_AUDIO(x...)
#	define TRACE_VIDEO(x...)
#endif

//#define LOG_STREAM_TO_FILE
#ifdef LOG_STREAM_TO_FILE
#	include <File.h>
	static BFile sStreamLogFile("/boot/home/Desktop/AVCodecDebugStream.raw",
		B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	static int sDumpedPackets = 0;
#endif

#define USE_SWS_FOR_COLOR_SPACE_CONVERSION 0
// NOTE: David's color space conversion is much faster than the FFmpeg
// version. Perhaps the SWS code can be used for unsupported conversions?
// Otherwise the alternative code could simply be removed from this file.


struct wave_format_ex {
	uint16 format_tag;
	uint16 channels;
	uint32 frames_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	// extra_data[extra_size]
} _PACKED;


// profiling related globals
#define DO_PROFILING 0

static bigtime_t decodingTime = 0;
static bigtime_t conversionTime = 0;
static long profileCounter = 0;


AVCodecDecoder::AVCodecDecoder()
	:
	fHeader(),
	fInputFormat(),
	fOutputVideoFormat(),
	fFrame(0),
	fIsAudio(false),
	fCodec(NULL),
	fContext(avcodec_alloc_context()),
	fInputPicture(avcodec_alloc_frame()),
	fOutputPicture(avcodec_alloc_frame()),

	fCodecInitDone(false),

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	fSwsContext(NULL),
#else
	fFormatConversionFunc(NULL),
#endif

	fExtraData(NULL),
	fExtraDataSize(0),
	fBlockAlign(0),

	fStartTime(0),
	fOutputFrameCount(0),
	fOutputFrameRate(1.0),
	fOutputFrameSize(0),

	fChunkBuffer(NULL),
	fChunkBufferOffset(0),
	fChunkBufferSize(0),
	fAudioDecodeError(false),

	fOutputBuffer(NULL),
	fOutputBufferOffset(0),
	fOutputBufferSize(0)
{
	TRACE("AVCodecDecoder::AVCodecDecoder()\n");

	fContext->error_recognition = FF_ER_CAREFUL;
	fContext->error_concealment = 3;
	avcodec_thread_init(fContext, 1);
}


AVCodecDecoder::~AVCodecDecoder()
{
	TRACE("[%c] AVCodecDecoder::~AVCodecDecoder()\n", fIsAudio?('a'):('v'));

#ifdef DO_PROFILING
	if (profileCounter > 0) {
		printf("[%c] profile: d1 = %lld, d2 = %lld (%Ld)\n",
			fIsAudio?('a'):('v'), decodingTime / profileCounter,
			conversionTime / profileCounter, fFrame);
	}
#endif

	if (fCodecInitDone)
		avcodec_close(fContext);

	av_free(fOutputPicture);
	av_free(fInputPicture);
	av_free(fContext);

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext != NULL)
		sws_freeContext(fSwsContext);
#endif

	delete[] fExtraData;
	delete[] fOutputBuffer;
}


void
AVCodecDecoder::GetCodecInfo(media_codec_info* mci)
{
	snprintf(mci->short_name, 32, "%s", fCodec->name);
	snprintf(mci->pretty_name, 96, "%s", fCodec->long_name);
	mci->id = 0;
	mci->sub_id = fCodec->id;
}


status_t
AVCodecDecoder::Setup(media_format* ioEncodedFormat, const void* infoBuffer,
	size_t infoSize)
{
	if (ioEncodedFormat->type != B_MEDIA_ENCODED_AUDIO
		&& ioEncodedFormat->type != B_MEDIA_ENCODED_VIDEO)
		return B_ERROR;

	fIsAudio = (ioEncodedFormat->type == B_MEDIA_ENCODED_AUDIO);
	TRACE("[%c] AVCodecDecoder::Setup()\n", fIsAudio?('a'):('v'));

	if (fIsAudio && fOutputBuffer == NULL) {
		fOutputBuffer = new(std::nothrow) char[AVCODEC_MAX_AUDIO_FRAME_SIZE];
		if (fOutputBuffer == NULL)
			return B_NO_MEMORY;
	}

#ifdef TRACE_AV_CODEC
	char buffer[1024];
	string_for_format(*ioEncodedFormat, buffer, sizeof(buffer));
	TRACE("[%c]   input_format = %s\n", fIsAudio?('a'):('v'), buffer);
	TRACE("[%c]   infoSize = %ld\n", fIsAudio?('a'):('v'), infoSize);
	TRACE("[%c]   user_data_type = %08lx\n", fIsAudio?('a'):('v'),
		ioEncodedFormat->user_data_type);
	TRACE("[%c]   meta_data_size = %ld\n", fIsAudio?('a'):('v'),
		ioEncodedFormat->MetaDataSize());
#endif

	media_format_description description;
	if (BMediaFormats().GetCodeFor(*ioEncodedFormat,
			B_MISC_FORMAT_FAMILY, &description) == B_OK) {
		if (description.u.misc.file_format != 'ffmp')
			return B_NOT_SUPPORTED;
		fCodec = avcodec_find_decoder(static_cast<CodecID>(
			description.u.misc.codec));
		if (fCodec == NULL) {
			TRACE("  unable to find the correct FFmpeg "
				"decoder (id = %lu)\n", description.u.misc.codec);
			return B_ERROR;
		}
		TRACE("  found decoder %s\n", fCodec->name);

		const void* extraData = infoBuffer;
		fExtraDataSize = infoSize;
		if (description.family == B_WAV_FORMAT_FAMILY
				&& infoSize >= sizeof(wave_format_ex)) {
			TRACE("  trying to use wave_format_ex\n");
			// Special case extra data in B_WAV_FORMAT_FAMILY
			const wave_format_ex* waveFormatData
				= (const wave_format_ex*)infoBuffer;

			size_t waveFormatSize = infoSize;
			if (waveFormatData != NULL && waveFormatSize > 0) {
				fBlockAlign = waveFormatData->block_align;
				TRACE("  found block align: %d\n", fBlockAlign);
				fExtraDataSize = waveFormatData->extra_size;
				// skip the wave_format_ex from the extra data.
				extraData = waveFormatData + 1;
			}
		} else {
			if (fIsAudio) {
				fBlockAlign
					= ioEncodedFormat->u.encoded_audio.output
						.buffer_size;
				TRACE("  using buffer_size as block align: %d\n",
					fBlockAlign);
			}
		}
		if (extraData != NULL && fExtraDataSize > 0) {
			TRACE("AVCodecDecoder: extra data size %ld\n", infoSize);
			delete[] fExtraData;
			fExtraData = new(std::nothrow) char[fExtraDataSize];
			if (fExtraData != NULL)
				memcpy(fExtraData, infoBuffer, fExtraDataSize);
			else
				fExtraDataSize = 0;
		}

		fInputFormat = *ioEncodedFormat;
		return B_OK;
	} else {
		TRACE("AVCodecDecoder: BMediaFormats().GetCodeFor() failed.\n");
	}

	printf("AVCodecDecoder::Setup failed!\n");
	return B_ERROR;
}


status_t
AVCodecDecoder::SeekedTo(int64 frame, bigtime_t time)
{
	status_t ret = B_OK;
	// Reset the FFmpeg codec to flush buffers, so we keep the sync
	if (fCodecInitDone)
		avcodec_flush_buffers(fContext);

	// Flush internal buffers as well.
	fChunkBuffer = NULL;
	fChunkBufferOffset = 0;
	fChunkBufferSize = 0;
	fOutputBufferOffset = 0;
	fOutputBufferSize = 0;

	fFrame = frame;
	fStartTime = time;

	return ret;
}


status_t
AVCodecDecoder::NegotiateOutputFormat(media_format* inOutFormat)
{
	TRACE("AVCodecDecoder::NegotiateOutputFormat() [%c] \n",
		fIsAudio?('a'):('v'));

#ifdef TRACE_AV_CODEC
	char buffer[1024];
	string_for_format(*inOutFormat, buffer, sizeof(buffer));
	TRACE("  [%c]  requested format = %s\n", fIsAudio?('a'):('v'), buffer);
#endif

	if (fIsAudio)
		return _NegotiateAudioOutputFormat(inOutFormat);
	else
		return _NegotiateVideoOutputFormat(inOutFormat);
}


status_t
AVCodecDecoder::Decode(void* outBuffer, int64* outFrameCount,
	media_header* mediaHeader, media_decode_info* info)
{
	if (!fCodecInitDone)
		return B_NO_INIT;

//	TRACE("[%c] AVCodecDecoder::Decode() for time %Ld\n", fIsAudio?('a'):('v'),
//		fStartTime);

	mediaHeader->start_time = fStartTime;

	status_t ret;
	if (fIsAudio)
		ret = _DecodeAudio(outBuffer, outFrameCount, mediaHeader, info);
	else
		ret = _DecodeVideo(outBuffer, outFrameCount, mediaHeader, info);

	return ret;
}


// #pragma mark -


status_t
AVCodecDecoder::_NegotiateAudioOutputFormat(media_format* inOutFormat)
{
	TRACE("AVCodecDecoder::_NegotiateAudioOutputFormat()\n");

	media_multi_audio_format outputAudioFormat;
	outputAudioFormat = media_raw_audio_format::wildcard;
	outputAudioFormat.byte_order = B_MEDIA_HOST_ENDIAN;
	outputAudioFormat.frame_rate
		= fInputFormat.u.encoded_audio.output.frame_rate;
	outputAudioFormat.channel_count
		= fInputFormat.u.encoded_audio.output.channel_count;
	outputAudioFormat.format = fInputFormat.u.encoded_audio.output.format;
	outputAudioFormat.buffer_size
		= inOutFormat->u.raw_audio.buffer_size;
	// Check that format is not still a wild card!
	if (outputAudioFormat.format == 0) {
		TRACE("  format still a wild-card, assuming B_AUDIO_SHORT.\n");
		outputAudioFormat.format = media_raw_audio_format::B_AUDIO_SHORT;
	}
	size_t sampleSize = outputAudioFormat.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	// Check that channel count is not still a wild card!
	if (outputAudioFormat.channel_count == 0) {
		TRACE("  channel_count still a wild-card, assuming stereo.\n");
		outputAudioFormat.channel_count = 2;
	}

	if (outputAudioFormat.buffer_size == 0) {
		outputAudioFormat.buffer_size = 512
			* sampleSize * outputAudioFormat.channel_count;
	}
	inOutFormat->type = B_MEDIA_RAW_AUDIO;
	inOutFormat->u.raw_audio = outputAudioFormat;

	fContext->bit_rate = (int)fInputFormat.u.encoded_audio.bit_rate;
	fContext->frame_size = (int)fInputFormat.u.encoded_audio.frame_size;
	fContext->sample_rate
		= (int)fInputFormat.u.encoded_audio.output.frame_rate;
	fContext->channels = outputAudioFormat.channel_count;
	fContext->block_align = fBlockAlign;
	fContext->extradata = (uint8_t*)fExtraData;
	fContext->extradata_size = fExtraDataSize;

	// TODO: This probably needs to go away, there is some misconception
	// about extra data / info buffer and meta data. See
	// Reader::GetStreamInfo(). The AVFormatReader puts extradata and
	// extradata_size into media_format::MetaData(), but used to ignore
	// the infoBuffer passed to GetStreamInfo(). I think this may be why
	// the code below was added.
	if (fInputFormat.MetaDataSize() > 0) {
		fContext->extradata = (uint8_t*)fInputFormat.MetaData();
		fContext->extradata_size = fInputFormat.MetaDataSize();
	}

	TRACE("  bit_rate %d, sample_rate %d, channels %d, block_align %d, "
		"extradata_size %d\n", fContext->bit_rate, fContext->sample_rate,
		fContext->channels, fContext->block_align, fContext->extradata_size);

	// close any previous instance
	if (fCodecInitDone) {
		fCodecInitDone = false;
		avcodec_close(fContext);
	}

	// open new
	int result = avcodec_open(fContext, fCodec);
	fCodecInitDone = (result >= 0);

	fStartTime = 0;
	fOutputFrameSize = sampleSize * outputAudioFormat.channel_count;
	fOutputFrameCount = outputAudioFormat.buffer_size / fOutputFrameSize;
	fOutputFrameRate = outputAudioFormat.frame_rate;

	TRACE("  bit_rate = %d, sample_rate = %d, channels = %d, init = %d, "
		"output frame size: %d, count: %ld, rate: %.2f\n",
		fContext->bit_rate, fContext->sample_rate, fContext->channels,
		result, fOutputFrameSize, fOutputFrameCount, fOutputFrameRate);

	fChunkBuffer = NULL;
	fChunkBufferOffset = 0;
	fChunkBufferSize = 0;
	fAudioDecodeError = false;
	fOutputBufferOffset = 0;
	fOutputBufferSize = 0;

	av_init_packet(&fTempPacket);

	inOutFormat->require_flags = 0;
	inOutFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

	if (!fCodecInitDone) {
		TRACE("avcodec_open() failed!\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
AVCodecDecoder::_NegotiateVideoOutputFormat(media_format* inOutFormat)
{
	TRACE("AVCodecDecoder::_NegotiateVideoOutputFormat()\n");

	fOutputVideoFormat = fInputFormat.u.encoded_video.output;

	fContext->width = fOutputVideoFormat.display.line_width;
	fContext->height = fOutputVideoFormat.display.line_count;
//	fContext->frame_rate = (int)(fOutputVideoFormat.field_rate
//		* fContext->frame_rate_base);

	fOutputFrameRate = fOutputVideoFormat.field_rate;

	fContext->extradata = (uint8_t*)fExtraData;
	fContext->extradata_size = fExtraDataSize;

	TRACE("  requested video format 0x%x\n",
		inOutFormat->u.raw_video.display.format);

	// Make MediaPlayer happy (if not in rgb32 screen depth and no overlay,
	// it will only ask for YCbCr, which DrawBitmap doesn't handle, so the
	// default colordepth is RGB32).
	if (inOutFormat->u.raw_video.display.format == B_YCbCr422)
		fOutputVideoFormat.display.format = B_YCbCr422;
	else
		fOutputVideoFormat.display.format = B_RGB32;

	// Search for a pixel-format the codec handles
	// TODO: We should try this a couple of times until it succeeds, each
	// time using another pixel-format that is supported by the decoder.
	// But libavcodec doesn't seem to offer any way to tell the decoder
	// which format it should use.
#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext != NULL)
		sws_freeContext(fSwsContext);
	fSwsContext = NULL;
#else
	fFormatConversionFunc = 0;
#endif
	// Iterate over supported codec formats
	for (int i = 0; i < 1; i++) {
		// close any previous instance
		if (fCodecInitDone) {
			fCodecInitDone = false;
			avcodec_close(fContext);
		}
		// TODO: Set n-th fContext->pix_fmt here
		if (avcodec_open(fContext, fCodec) >= 0) {
			fCodecInitDone = true;

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
			fSwsContext = sws_getContext(fContext->width, fContext->height,
				fContext->pix_fmt, fContext->width, fContext->height,
				colorspace_to_pixfmt(fOutputVideoFormat.display.format),
				SWS_FAST_BILINEAR, NULL, NULL, NULL);
		}
#else
			fFormatConversionFunc = resolve_colorspace(
				fOutputVideoFormat.display.format, fContext->pix_fmt, 
				fContext->width, fContext->height);
		}
		if (fFormatConversionFunc != NULL)
			break;
#endif
	}

	if (!fCodecInitDone) {
		TRACE("avcodec_open() failed to init codec!\n");
		return B_ERROR;
	}

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext == NULL) {
		TRACE("No SWS Scale context or decoder has not set the pixel format "
			"yet!\n");
	}
#else
	if (fFormatConversionFunc == NULL) {
		TRACE("no pixel format conversion function found or decoder has "
			"not set the pixel format yet!\n");
	}
#endif

	if (fOutputVideoFormat.display.format == B_YCbCr422) {
		fOutputVideoFormat.display.bytes_per_row
			= 2 * fOutputVideoFormat.display.line_width;
	} else {
		fOutputVideoFormat.display.bytes_per_row
			= 4 * fOutputVideoFormat.display.line_width;
	}

	inOutFormat->type = B_MEDIA_RAW_VIDEO;
	inOutFormat->u.raw_video = fOutputVideoFormat;

	inOutFormat->require_flags = 0;
	inOutFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

#ifdef TRACE_AV_CODEC
	char buffer[1024];
	string_for_format(*inOutFormat, buffer, sizeof(buffer));
	TRACE("[v]  outFormat = %s\n", buffer);
	TRACE("  returned  video format 0x%x\n",
		inOutFormat->u.raw_video.display.format);
#endif

	return B_OK;
}


status_t
AVCodecDecoder::_DecodeAudio(void* _buffer, int64* outFrameCount,
	media_header* mediaHeader, media_decode_info* info)
{
	TRACE_AUDIO("AVCodecDecoder::_DecodeAudio(audio start_time %.6fs)\n",
		mediaHeader->start_time / 1000000.0);

	*outFrameCount = 0;

	uint8* buffer = reinterpret_cast<uint8*>(_buffer);
	while (*outFrameCount < fOutputFrameCount) {
		// Check conditions which would hint at broken code below.
		if (fOutputBufferSize < 0) {
			debugger("Decoding read past the end of the output buffer!");
			fOutputBufferSize = 0;
		}
		if (fChunkBufferSize < 0) {
			debugger("Decoding read past the end of the chunk buffer!");
			fChunkBufferSize = 0;
		}

		if (fOutputBufferSize > 0) {
			// We still have decoded audio frames from the last
			// invokation, which start at fOutputBuffer + fOutputBufferOffset
			// and are of fOutputBufferSize. Copy those into the buffer,
			// but not more than it can hold.
			int32 frames = min_c(fOutputFrameCount - *outFrameCount,
				fOutputBufferSize / fOutputFrameSize);
			if (frames == 0)
				debugger("fOutputBufferSize not multiple of frame size!");
			size_t remainingSize = frames * fOutputFrameSize;
			memcpy(buffer, fOutputBuffer + fOutputBufferOffset, remainingSize);
			fOutputBufferOffset += remainingSize;
			fOutputBufferSize -= remainingSize;
			buffer += remainingSize;
			*outFrameCount += frames;
			fStartTime += (bigtime_t)((1000000LL * frames) / fOutputFrameRate);
			continue;
		}
		if (fChunkBufferSize == 0) {
			// Time to read the next chunk buffer. We use a separate
			// media_header, since the chunk header may not belong to
			// the start of the decoded audio frames we return. For
			// example we may have used frames from a previous invokation,
			// or we may have to read several chunks until we fill up the
			// output buffer.
			media_header chunkMediaHeader;
			status_t err = GetNextChunk(&fChunkBuffer, &fChunkBufferSize,
				&chunkMediaHeader);
			if (err == B_LAST_BUFFER_ERROR) {
				TRACE_AUDIO("  Last Chunk with chunk size %ld\n",
					fChunkBufferSize);
				fChunkBufferSize = 0;
				return err;
			}
			if (err != B_OK || fChunkBufferSize < 0) {
				printf("GetNextChunk error %ld\n",fChunkBufferSize);
				fChunkBufferSize = 0;
				break;
			}
			fChunkBufferOffset = 0;
			fStartTime = chunkMediaHeader.start_time;
		}

		fTempPacket.data = (uint8_t*)fChunkBuffer + fChunkBufferOffset;
		fTempPacket.size = fChunkBufferSize;
		// Initialize decodedBytes to the output buffer size.
		int decodedBytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
		int usedBytes = avcodec_decode_audio3(fContext,
			(int16*)fOutputBuffer, &decodedBytes, &fTempPacket);
		if (usedBytes < 0 && !fAudioDecodeError) {
			// Report failure if not done already
			printf("########### audio decode error, "
				"fChunkBufferSize %ld, fChunkBufferOffset %ld\n",
				fChunkBufferSize, fChunkBufferOffset);
			fAudioDecodeError = true;
		}
		if (usedBytes <= 0) {
			// Error or failure to produce decompressed output.
			// Skip the chunk buffer data entirely.
			usedBytes = fChunkBufferSize;
			decodedBytes = 0;
			// Assume the audio decoded until now is broken.
			memset(_buffer, 0, buffer - (uint8*)_buffer);
		} else {
			// Success
			fAudioDecodeError = false;
		}
//printf("  chunk size: %d, decoded: %d, used: %d\n",
//fTempPacket.size, decodedBytes, usedBytes);

		fChunkBufferOffset += usedBytes;
		fChunkBufferSize -= usedBytes;
		fOutputBufferOffset = 0;
		fOutputBufferSize = decodedBytes;
	}
	fFrame += *outFrameCount;
	TRACE_AUDIO("  frame count: %lld current: %lld\n", *outFrameCount, fFrame);

	return B_OK;
}


status_t
AVCodecDecoder::_DecodeVideo(void* outBuffer, int64* outFrameCount,
	media_header* mediaHeader, media_decode_info* info)
{
	bool firstRun = true;
	while (true) {
		const void* data;
		size_t size;
		media_header chunkMediaHeader;
		status_t err = GetNextChunk(&data, &size, &chunkMediaHeader);
		if (err != B_OK) {
			TRACE("AVCodecDecoder::_DecodeVideo(): error from "
				"GetNextChunk(): %s\n", strerror(err));
			return err;
		}
#ifdef LOG_STREAM_TO_FILE
		if (sDumpedPackets < 100) {
			sStreamLogFile.Write(data, size);
			printf("wrote %ld bytes\n", size);
			sDumpedPackets++;
		} else if (sDumpedPackets == 100)
			sStreamLogFile.Unset();
#endif

		if (firstRun) {
			firstRun = false;

			mediaHeader->type = B_MEDIA_RAW_VIDEO;
			mediaHeader->start_time = chunkMediaHeader.start_time;
			fStartTime = chunkMediaHeader.start_time;
			mediaHeader->file_pos = 0;
			mediaHeader->orig_size = 0;
			mediaHeader->u.raw_video.field_gamma = 1.0;
			mediaHeader->u.raw_video.field_sequence = fFrame;
			mediaHeader->u.raw_video.field_number = 0;
			mediaHeader->u.raw_video.pulldown_number = 0;
			mediaHeader->u.raw_video.first_active_line = 1;
			mediaHeader->u.raw_video.line_count
				= fOutputVideoFormat.display.line_count;

			TRACE("[v] start_time=%02d:%02d.%02d field_sequence=%lu\n",
				int((mediaHeader->start_time / 60000000) % 60),
				int((mediaHeader->start_time / 1000000) % 60),
				int((mediaHeader->start_time / 10000) % 100),
				mediaHeader->u.raw_video.field_sequence);
		}

#if DO_PROFILING
		bigtime_t startTime = system_time();
#endif

		// NOTE: In the FFmpeg code example I've read, the length returned by
		// avcodec_decode_video() is completely ignored. Furthermore, the
		// packet buffers are supposed to contain complete frames only so we
		// don't seem to be required to buffer any packets because not the
		// complete packet has been read.
		fTempPacket.data = (uint8_t*)data;
		fTempPacket.size = size;
		int gotPicture = 0;
		int len = avcodec_decode_video2(fContext, fInputPicture, &gotPicture,
			&fTempPacket);
		if (len < 0) {
			TRACE("[v] AVCodecDecoder: error in decoding frame %lld: %d\n",
				fFrame, len);
			// NOTE: An error from avcodec_decode_video() seems to be ignored
			// in the ffplay sample code.
//			return B_ERROR;
		}


//TRACE("FFDEC: PTS = %d:%d:%d.%d - fContext->frame_number = %ld "
//	"fContext->frame_rate = %ld\n", (int)(fContext->pts / (60*60*1000000)),
//	(int)(fContext->pts / (60*1000000)), (int)(fContext->pts / (1000000)),
//	(int)(fContext->pts % 1000000), fContext->frame_number,
//	fContext->frame_rate);
//TRACE("FFDEC: PTS = %d:%d:%d.%d - fContext->frame_number = %ld "
//	"fContext->frame_rate = %ld\n",
//	(int)(fInputPicture->pts / (60*60*1000000)),
//	(int)(fInputPicture->pts / (60*1000000)),
//	(int)(fInputPicture->pts / (1000000)),
//	(int)(fInputPicture->pts % 1000000), fContext->frame_number,
//	fContext->frame_rate);

		if (gotPicture) {
			int width = fOutputVideoFormat.display.line_width;
			int height = fOutputVideoFormat.display.line_count;
			AVPicture deinterlacedPicture;
			bool useDeinterlacedPicture = false;

			if (fInputPicture->interlaced_frame) {
				AVPicture source;
				source.data[0] = fInputPicture->data[0];
				source.data[1] = fInputPicture->data[1];
				source.data[2] = fInputPicture->data[2];
				source.data[3] = fInputPicture->data[3];
				source.linesize[0] = fInputPicture->linesize[0];
				source.linesize[1] = fInputPicture->linesize[1];
				source.linesize[2] = fInputPicture->linesize[2];
				source.linesize[3] = fInputPicture->linesize[3];

				avpicture_alloc(&deinterlacedPicture,
					fContext->pix_fmt, width, height);

				if (avpicture_deinterlace(&deinterlacedPicture, &source,
						fContext->pix_fmt, width, height) < 0) {
					TRACE("[v] avpicture_deinterlace() - error\n");
				} else
					useDeinterlacedPicture = true;
			}

#if DO_PROFILING
			bigtime_t formatConversionStart = system_time();
#endif
//			TRACE("ONE FRAME OUT !! len=%d size=%ld (%s)\n", len, size,
//				pixfmt_to_string(fContext->pix_fmt));

			// Some decoders do not set pix_fmt until they have decoded 1 frame
#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
			if (fSwsContext == NULL) {
				fSwsContext = sws_getContext(fContext->width, fContext->height,
					fContext->pix_fmt, fContext->width, fContext->height,
					colorspace_to_pixfmt(fOutputVideoFormat.display.format),
					SWS_FAST_BILINEAR, NULL, NULL, NULL);
			}
#else
			if (fFormatConversionFunc == NULL) {
				fFormatConversionFunc = resolve_colorspace(
					fOutputVideoFormat.display.format, fContext->pix_fmt, 
					fContext->width, fContext->height);
			}
#endif

			fOutputPicture->data[0] = (uint8_t*)outBuffer;
			fOutputPicture->linesize[0]
				= fOutputVideoFormat.display.bytes_per_row;

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
			if (fSwsContext != NULL) {
#else
			if (fFormatConversionFunc != NULL) {
#endif
				if (useDeinterlacedPicture) {
					AVFrame inputFrame;
					inputFrame.data[0] = deinterlacedPicture.data[0];
					inputFrame.data[1] = deinterlacedPicture.data[1];
					inputFrame.data[2] = deinterlacedPicture.data[2];
					inputFrame.data[3] = deinterlacedPicture.data[3];
					inputFrame.linesize[0] = deinterlacedPicture.linesize[0];
					inputFrame.linesize[1] = deinterlacedPicture.linesize[1];
					inputFrame.linesize[2] = deinterlacedPicture.linesize[2];
					inputFrame.linesize[3] = deinterlacedPicture.linesize[3];

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
					sws_scale(fSwsContext, inputFrame.data,
						inputFrame.linesize, 0, fContext->height,
						fOutputPicture->data, fOutputPicture->linesize);
#else
					(*fFormatConversionFunc)(&inputFrame,
						fOutputPicture, width, height);
#endif
				} else {
#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
					sws_scale(fSwsContext, fInputPicture->data,
						fInputPicture->linesize, 0, fContext->height,
						fOutputPicture->data, fOutputPicture->linesize);
#else
					(*fFormatConversionFunc)(fInputPicture, fOutputPicture,
						width, height);
#endif
				}
			}
			if (fInputPicture->interlaced_frame)
				avpicture_free(&deinterlacedPicture);
#ifdef DEBUG
			dump_ffframe(fInputPicture, "ffpict");
//			dump_ffframe(fOutputPicture, "opict");
#endif
			*outFrameCount = 1;
			fFrame++;

#if DO_PROFILING
			bigtime_t doneTime = system_time();
			decodingTime += formatConversionStart - startTime;
			conversionTime += doneTime - formatConversionStart;
			profileCounter++;
			if (!(fFrame % 5)) {
				if (info) {
					printf("[v] profile: d1 = %lld, d2 = %lld (%lld) required "
						"%Ld\n",
						decodingTime / profileCounter,
						conversionTime / profileCounter,
						fFrame, info->time_to_decode);
				} else {
					printf("[v] profile: d1 = %lld, d2 = %lld (%lld) required "
						"%Ld\n",
						decodingTime / profileCounter,
						conversionTime / profileCounter,
						fFrame, bigtime_t(1000000LL / fOutputFrameRate));
				}
				decodingTime = 0;
				conversionTime = 0;
				profileCounter = 0;
			}
#endif
			return B_OK;
		} else {
			TRACE("frame %lld - no picture yet, len: %d, chunk size: %ld\n",
				fFrame, len, size);
		}
	}
}



/*
 * Copyright (C) 2001 Carlos Hasan
 * Copyright (C) 2001 François Revol
 * Copyright (C) 2001 Axel Dörfler
 * Copyright (C) 2004 Marcus Overhagen
 * Copyright (C) 2009 Stephan Amßus <superstippi@gmx.de>
 * Copyright (C) 2014 Colin Günther <coling@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//! libavcodec based decoder for Haiku

#include "AVCodecDecoder.h"

#include <new>

#include <assert.h>
#include <string.h>

#include <Bitmap.h>
#include <Debug.h>

#include "Utilities.h"


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

#ifdef __x86_64
#define USE_SWS_FOR_COLOR_SPACE_CONVERSION 1
#else
#define USE_SWS_FOR_COLOR_SPACE_CONVERSION 0
// NOTE: David's color space conversion is much faster than the FFmpeg
// version. Perhaps the SWS code can be used for unsupported conversions?
// Otherwise the alternative code could simply be removed from this file.
#endif


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
	fFrame(0),
	fIsAudio(false),
	fCodec(NULL),
	fContext(avcodec_alloc_context3(NULL)),
	fDecodedData(NULL),
	fDecodedDataSizeInBytes(0),
	fPostProcessedDecodedPicture(avcodec_alloc_frame()),
	fRawDecodedPicture(avcodec_alloc_frame()),

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
	fOutputColorSpace(B_NO_COLOR_SPACE),
	fOutputFrameCount(0),
	fOutputFrameRate(1.0),
	fOutputFrameSize(0),

	fChunkBuffer(NULL),
	fVideoChunkBuffer(NULL),
	fChunkBufferOffset(0),
	fChunkBufferSize(0),
	fAudioDecodeError(false),

	fOutputFrame(avcodec_alloc_frame()),
	fOutputBufferOffset(0),
	fOutputBufferSize(0)
{
	TRACE("AVCodecDecoder::AVCodecDecoder()\n");

	system_info info;
	get_system_info(&info);

	fContext->err_recognition = AV_EF_CAREFUL;
	fContext->error_concealment = 3;
	fContext->thread_count = info.cpu_count;
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

	free(fVideoChunkBuffer);
		// TODO: Replace with fChunkBuffer, once audio path is
		// responsible for freeing the chunk buffer, too.
	free(fDecodedData);

	av_free(fPostProcessedDecodedPicture);
	av_free(fRawDecodedPicture);
	av_free(fContext);
	av_free(fOutputFrame);

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext != NULL)
		sws_freeContext(fSwsContext);
#endif

	delete[] fExtraData;
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
	if (fCodecInitDone) {
		avcodec_flush_buffers(fContext);
		_ResetTempPacket();
	}

	// Flush internal buffers as well.
	free(fVideoChunkBuffer);
		// TODO: Replace with fChunkBuffer, once audio path is
		// responsible for freeing the chunk buffer, too.
	fVideoChunkBuffer = NULL;
	fChunkBuffer = NULL;
	fChunkBufferOffset = 0;
	fChunkBufferSize = 0;
	fOutputBufferOffset = 0;
	fOutputBufferSize = 0;
	fDecodedDataSizeInBytes = 0;

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


void
AVCodecDecoder::_ResetTempPacket()
{
	av_init_packet(&fTempPacket);
	fTempPacket.size = 0;
	fTempPacket.data = NULL;
}


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
	int result = avcodec_open2(fContext, fCodec, NULL);
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

	_ResetTempPacket();

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

	TRACE("  requested video format 0x%x\n",
		inOutFormat->u.raw_video.display.format);

	// Make MediaPlayer happy (if not in rgb32 screen depth and no overlay,
	// it will only ask for YCbCr, which DrawBitmap doesn't handle, so the
	// default colordepth is RGB32).
	if (inOutFormat->u.raw_video.display.format == B_YCbCr422)
		fOutputColorSpace = B_YCbCr422;
	else
		fOutputColorSpace = B_RGB32;

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext != NULL)
		sws_freeContext(fSwsContext);
	fSwsContext = NULL;
#else
	fFormatConversionFunc = 0;
#endif

	fContext->extradata = (uint8_t*)fExtraData;
	fContext->extradata_size = fExtraDataSize;

	bool codecCanHandleIncompleteFrames
		= (fCodec->capabilities & CODEC_CAP_TRUNCATED) != 0;
	if (codecCanHandleIncompleteFrames) {
		// Expect and handle video frames to be splitted across consecutive
		// data chunks.
		fContext->flags |= CODEC_FLAG_TRUNCATED;
	}

	// close any previous instance
	if (fCodecInitDone) {
		fCodecInitDone = false;
		avcodec_close(fContext);
	}

	if (avcodec_open2(fContext, fCodec, NULL) >= 0)
		fCodecInitDone = true;
	else {
		TRACE("avcodec_open() failed to init codec!\n");
		return B_ERROR;
	}

	_ResetTempPacket();

	status_t statusOfDecodingFirstFrame = _DecodeNextVideoFrame();
	if (statusOfDecodingFirstFrame != B_OK) {
		TRACE("[v] decoding first video frame failed\n");
		return B_ERROR;
	}

	// Note: fSwsContext / fFormatConversionFunc should have been initialized
	// by first call to _DecodeNextVideoFrame() above.
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

	inOutFormat->type = B_MEDIA_RAW_VIDEO;
	inOutFormat->require_flags = 0;
	inOutFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

	inOutFormat->u.raw_video = fInputFormat.u.encoded_video.output;

	inOutFormat->u.raw_video.interlace = 1;
		// Progressive (non-interlaced) video frames are delivered
	inOutFormat->u.raw_video.first_active = fHeader.u.raw_video.first_active_line;
	inOutFormat->u.raw_video.last_active = fHeader.u.raw_video.line_count;
	inOutFormat->u.raw_video.pixel_width_aspect = fHeader.u.raw_video.pixel_width_aspect;
	inOutFormat->u.raw_video.pixel_height_aspect = fHeader.u.raw_video.pixel_height_aspect;
	inOutFormat->u.raw_video.field_rate = fOutputFrameRate;
		// Was calculated by first call to _DecodeNextVideoFrame()

	inOutFormat->u.raw_video.display.format = fOutputColorSpace;
	inOutFormat->u.raw_video.display.line_width = fHeader.u.raw_video.display_line_width;
	inOutFormat->u.raw_video.display.line_count = fHeader.u.raw_video.display_line_count;
	inOutFormat->u.raw_video.display.bytes_per_row = fHeader.u.raw_video.bytes_per_row;

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
			fprintf(stderr, "Decoding read past the end of the output buffer! "
				"%ld\n", fOutputBufferSize);
			fOutputBufferSize = 0;
		}
		if (fChunkBufferSize < 0) {
			fprintf(stderr, "Decoding read past the end of the chunk buffer! "
				"%ld\n", fChunkBufferSize);
			fChunkBufferSize = 0;
		}

		if (fOutputBufferSize > 0) {
			// We still have decoded audio frames from the last
			// invokation, which start at fOutputBufferOffset
			// and are of fOutputBufferSize. Copy those into the buffer,
			// but not more than it can hold.
			int32 frames = min_c(fOutputFrameCount - *outFrameCount,
				fOutputBufferSize / fOutputFrameSize);
			if (frames == 0)
				debugger("fOutputBufferSize not multiple of frame size!");
			size_t remainingSize = frames * fOutputFrameSize;
			memcpy(buffer, fOutputFrame->data[0] + fOutputBufferOffset,
				remainingSize);
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

		avcodec_get_frame_defaults(fOutputFrame);
		int gotFrame = 0;
		int usedBytes = avcodec_decode_audio4(fContext,
			fOutputFrame, &gotFrame, &fTempPacket);
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
			fOutputBufferSize = 0;
			// Assume the audio decoded until now is broken.
			memset(_buffer, 0, buffer - (uint8*)_buffer);
		} else {
			// Success
			fAudioDecodeError = false;
			if (gotFrame == 1) {
				fOutputBufferSize = av_samples_get_buffer_size(NULL,
					fContext->channels, fOutputFrame->nb_samples,
					fContext->sample_fmt, 1);
				if (fOutputBufferSize < 0)
					fOutputBufferSize = 0;
			} else
				fOutputBufferSize = 0;
		}
//printf("  chunk size: %d, decoded: %d, used: %d\n",
//fTempPacket.size, decodedBytes, usedBytes);

		fChunkBufferOffset += usedBytes;
		fChunkBufferSize -= usedBytes;
		fOutputBufferOffset = 0;
	}
	fFrame += *outFrameCount;
	TRACE_AUDIO("  frame count: %lld current: %lld\n", *outFrameCount, fFrame);

	return B_OK;
}


/*! \brief Fills the outBuffer with an already decoded video frame.

	Besides the main duty described above, this method also fills out the other
	output parameters as documented below.

	\param outBuffer Pointer to the output buffer to copy the decoded video
		frame to.
	\param outFrameCount Pointer to the output variable to assign the number of
		copied video frames (usually one video frame).
	\param mediaHeader Pointer to the output media header that contains the
		decoded video frame properties.
	\param info TODO (not used at the moment)

	\returns B_OK Decoding a video frame succeeded.
	\returns B_LAST_BUFFER_ERROR There are no more video frames available.
	\returns other error codes
*/
status_t
AVCodecDecoder::_DecodeVideo(void* outBuffer, int64* outFrameCount,
	media_header* mediaHeader, media_decode_info* info)
{
	status_t videoDecodingStatus
		= fDecodedDataSizeInBytes > 0 ? B_OK : _DecodeNextVideoFrame();

	if (videoDecodingStatus != B_OK)
		return videoDecodingStatus;

	*outFrameCount = 1;
	*mediaHeader = fHeader;
	memcpy(outBuffer, fDecodedData, mediaHeader->size_used);

	fDecodedDataSizeInBytes = 0;

	return B_OK;
}


/*! \brief Decodes next video frame.

    We decode exactly one video frame into fDecodedData. To achieve this goal,
    we might need to request several chunks of encoded data resulting in a
    variable execution time of this function.

    The length of the decoded video frame is stored in
    fDecodedDataSizeInBytes. If this variable is greater than zero, you can
    assert that there is a valid video frame available in fDecodedData.

    The decoded video frame in fDecodedData has color space conversion and
    deinterlacing already applied.

    To every decoded video frame there is a media_header populated in
    fHeader, containing the corresponding video frame properties.

	Normally every decoded video frame has a start_time field populated in the
	associated fHeader, that determines the presentation time of the frame.
	This relationship will only hold true, when each data chunk that is
	provided via GetNextChunk() contains data for exactly one encoded video
	frame (one complete frame) - not more and not less.

	We can decode data chunks that contain partial video frame data, too. In
	that case, you cannot trust the value of the start_time field in fHeader.
	We simply have no logic in place to establish a meaningful relationship
	between an incomplete frame and the start time it should be presented.
	Though this	might change in the future.

	We can decode data chunks that contain more than one video frame, too. In
	that case, you cannot trust the value of the start_time field in fHeader.
	We simply have no logic in place to track the start_time across multiple
	video frames. So a meaningful relationship between the 2nd, 3rd, ... frame
	and the start time it should be presented isn't established at the moment.
	Though this	might change in the future.

    More over the fOutputFrameRate variable is updated for every decoded video
    frame.

	On first call the member variables fSwsContext / fFormatConversionFunc	are
	initialized.

	\returns B_OK when we successfully decoded one video frame
	\returns B_LAST_BUFFER_ERROR when there are no more video frames available.
	\returns B_NO_MEMORY when we have no memory left for correct operation.
	\returns Other Errors
 */
status_t
AVCodecDecoder::_DecodeNextVideoFrame()
{
	assert(fTempPacket.size >= 0);

	while (true) {
		status_t loadingChunkStatus
			= _LoadNextVideoChunkIfNeededAndAssignStartTime();

		if (loadingChunkStatus == B_LAST_BUFFER_ERROR)
			return _FlushOneVideoFrameFromDecoderBuffer();

		if (loadingChunkStatus != B_OK) {
			TRACE("AVCodecDecoder::_DecodeNextVideoFrame(): error from "
				"GetNextChunk(): %s\n", strerror(err));
			return loadingChunkStatus;
		}

#if DO_PROFILING
		bigtime_t startTime = system_time();
#endif

		// NOTE: In the FFMPEG 0.10.2 code example decoding_encoding.c, the
		// length returned by avcodec_decode_video2() is used to update the
		// packet buffer size (here it is fTempPacket.size). This way the
		// packet buffer is allowed to contain incomplete frames so we are
		// required to buffer the packets between different calls to
		// _DecodeNextVideoFrame().
		int gotVideoFrame = 0;
		int decodedDataSizeInBytes = avcodec_decode_video2(fContext,
			fRawDecodedPicture, &gotVideoFrame, &fTempPacket);
		if (decodedDataSizeInBytes < 0) {
			TRACE("[v] AVCodecDecoder: ignoring error in decoding frame %lld:"
				" %d\n", fFrame, len);
			// NOTE: An error from avcodec_decode_video2() is ignored by the
			// FFMPEG 0.10.2 example decoding_encoding.c. Only the packet
			// buffers are flushed accordingly
			fTempPacket.data = NULL;
			fTempPacket.size = 0;
			continue;
		}

		fTempPacket.size -= decodedDataSizeInBytes;
		fTempPacket.data += decodedDataSizeInBytes;

		bool gotNoVideoFrame = gotVideoFrame == 0;
		if (gotNoVideoFrame) {
			TRACE("frame %lld - no picture yet, decodedDataSizeInBytes: %d, "
				"chunk size: %ld\n", fFrame, decodedDataSizeInBytes,
				fChunkBufferSize);
			continue;
		}

#if DO_PROFILING
		bigtime_t formatConversionStart = system_time();
#endif

		_HandleNewVideoFrameAndUpdateSystemState();

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
	}
}


/*! \brief Loads the next video chunk into fVideoChunkBuffer and assigns it
		(including the start time) to fTempPacket accordingly only if
		fTempPacket is empty.

	\returns B_OK
		1. meaning: Next video chunk is loaded.
		2. meaning: No need to load and assign anything. Proceed as usual.
	\returns B_LAST_BUFFER_ERROR No more video chunks available.
		fVideoChunkBuffer and fTempPacket are left untouched.
	\returns Other errors Caller should bail out because fVideoChunkBuffer and
		fTempPacket are in unknown states. Normal operation cannot be
		guaranteed.
*/
status_t
AVCodecDecoder::_LoadNextVideoChunkIfNeededAndAssignStartTime()
{
	// TODO: Rename fVideoChunkBuffer to fChunkBuffer, once the audio path is
	// responsible for releasing the chunk buffer, too.

	if (fTempPacket.size > 0)
		return B_OK;

	const void* chunkBuffer = NULL;
	size_t chunkBufferSize = 0;
		// In the case that GetNextChunk() returns an error fChunkBufferSize
		// should be left untouched.
	media_header chunkMediaHeader;

	status_t getNextChunkStatus = GetNextChunk(&chunkBuffer,
		&chunkBufferSize, &chunkMediaHeader);
	if (getNextChunkStatus != B_OK)
		return getNextChunkStatus;

	status_t chunkBufferPaddingStatus
		= _CopyChunkToVideoChunkBufferAndAddPadding(chunkBuffer,
		chunkBufferSize);
	if (chunkBufferPaddingStatus != B_OK)
		return chunkBufferPaddingStatus;

	fTempPacket.data = fVideoChunkBuffer;
	fTempPacket.size = fChunkBufferSize;
	fTempPacket.dts = chunkMediaHeader.start_time;
		// Let FFMPEG handle the correct relationship between start_time and
		// decoded video frame. By doing so we are simply copying the way how
		// it is implemented in ffplay.c
		// \see http://git.videolan.org/?p=ffmpeg.git;a=blob;f=ffplay.c;h=09623db374e5289ed20b7cc28c262c4375a8b2e4;hb=9153b33a742c4e2a85ff6230aea0e75f5a8b26c2#l1502
		//
		// FIXME: Research how to establish a meaningful relationship
		// between start_time and decoded video frame when the received
		// chunk buffer contains partial video frames. Maybe some data
		// formats contain time stamps (ake pts / dts fields) that can
		// be evaluated by FFMPEG. But as long as I don't have such
		// video data to test it, it makes no sense to implement it.
		//
		// FIXME: Implement tracking start_time of video frames
		// originating in data chunks that encode more than one video
		// frame at a time. In that case on would increment the
		// start_time for each consecutive frame of such a data chunk
		// (like it is done for audio frame decoding). But as long as
		// I don't have such video data to test it, it makes no sense
		// to implement it.

#ifdef LOG_STREAM_TO_FILE
	if (sDumpedPackets < 100) {
		sStreamLogFile.Write(chunkBuffer, fChunkBufferSize);
		printf("wrote %ld bytes\n", fChunkBufferSize);
		sDumpedPackets++;
	} else if (sDumpedPackets == 100)
		sStreamLogFile.Unset();
#endif

	return B_OK;
}


/*! \brief Copies a chunk into fVideoChunkBuffer and adds a "safety net" of
		additional memory as required by FFMPEG for input buffers to video
		decoders.

	This is needed so that some decoders can read safely a predefined number of
	bytes at a time for performance optimization purposes.

	The additional memory has a size of FF_INPUT_BUFFER_PADDING_SIZE as defined
	in avcodec.h.

	Ownership of fVideoChunkBuffer memory is with the class so it needs to be
	freed at the right times (on destruction, on seeking).

	Also update fChunkBufferSize to reflect the size of the contained video
	data (leaving out the padding).
	
	\param chunk The chunk to copy.
	\param chunkSize Size of the chunk in bytes

	\returns B_OK Padding was successful. You are responsible for releasing the
		allocated memory. fChunkBufferSize is set to chunkSize.
	\returns B_NO_MEMORY Padding failed.
		fVideoChunkBuffer is set to NULL making it safe to call free() on it.
		fChunkBufferSize is set to 0 to reflect the size of fVideoChunkBuffer.
*/
status_t
AVCodecDecoder::_CopyChunkToVideoChunkBufferAndAddPadding(const void* chunk,
	size_t chunkSize)
{
	// TODO: Rename fVideoChunkBuffer to fChunkBuffer, once the audio path is
	// responsible for releasing the chunk buffer, too.

	fVideoChunkBuffer = static_cast<uint8_t*>(realloc(fVideoChunkBuffer,
		chunkSize + FF_INPUT_BUFFER_PADDING_SIZE));
	if (fVideoChunkBuffer == NULL) {
		fChunkBufferSize = 0;
		return B_NO_MEMORY;
	}

	memcpy(fVideoChunkBuffer, chunk, chunkSize);
	memset(fVideoChunkBuffer + chunkSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
		// Establish safety net, by zero'ing the padding area.

	fChunkBufferSize = chunkSize;

	return B_OK;
}


/*! \brief Executes all steps needed for a freshly decoded video frame.

	\see _UpdateMediaHeaderForVideoFrame() and
	\see _DeinterlaceAndColorConvertVideoFrame() for when you are allowed to
	call this method.
*/
void
AVCodecDecoder::_HandleNewVideoFrameAndUpdateSystemState()
{
	_DeinterlaceAndColorConvertVideoFrame();
	_UpdateMediaHeaderForVideoFrame();

	ConvertAVCodecContextToVideoFrameRate(*fContext, fOutputFrameRate);

#ifdef DEBUG
	dump_ffframe(fRawDecodedPicture, "ffpict");
//	dump_ffframe(fPostProcessedDecodedPicture, "opict");
#endif
	fFrame++;
}


/*! \brief Flushes one video frame - if any - still buffered by the decoder.

	Some FFMPEG decoder are buffering video frames. To retrieve those buffered
	frames the decoder needs to be told so.

	The intended use of this method is to call it, once there are no more data
	chunks for decoding left. Reframed in other words: Once GetNextChunk()
	returns with status B_LAST_BUFFER_ERROR it is time to start flushing.

	\returns B_OK Retrieved one video frame, handled it accordingly and updated
		the system state accordingly.
		There maybe more video frames left. So it is valid for the client of
		AVCodecDecoder to call it one more time.

	\returns B_LAST_BUFFER_ERROR No video frame left.
		The client of the AVCodecDecoder should stop calling it now.
*/
status_t
AVCodecDecoder::_FlushOneVideoFrameFromDecoderBuffer()
{
	// Create empty fTempPacket to tell the video decoder it is time to flush
	fTempPacket.data = NULL;
	fTempPacket.size = 0;

	int gotVideoFrame = 0;
	avcodec_decode_video2(fContext,	fRawDecodedPicture, &gotVideoFrame,
		&fTempPacket);
		// We are only interested in complete frames now, so ignore the return
		// value.

	bool gotNoVideoFrame = gotVideoFrame == 0;
	if (gotNoVideoFrame) {
		// video buffer is flushed successfully
		return B_LAST_BUFFER_ERROR;
	}

	_HandleNewVideoFrameAndUpdateSystemState();

	return B_OK;
}


/*! \brief Updates relevant fields of the class member fHeader with the
		properties of the most recently decoded video frame.

	It is assumed that this function is called only	when the following asserts
	hold true:
		1. We actually got a new picture decoded by the video decoder.
		2. fHeader wasn't updated for the new picture yet. You MUST call this
		   method only once per decoded video frame.
		3. This function MUST be called after
		   _DeinterlaceAndColorConvertVideoFrame() as it relys on an updated
		    fDecodedDataSizeInBytes.
		4. There will be at maximumn only one decoded video frame in our cache
		   at any single point in time. Otherwise you couldn't tell to which
		   cached decoded video frame the properties in fHeader relate to.
		5. AVCodecContext is still valid for this video frame (This is the case
		   when this function is called after avcodec_decode_video2() and
		   before the next call to avcodec_decode_video2().
*/
void
AVCodecDecoder::_UpdateMediaHeaderForVideoFrame()
{
	fHeader.type = B_MEDIA_RAW_VIDEO;
	fHeader.file_pos = 0;
	fHeader.orig_size = 0;
	fHeader.start_time = fRawDecodedPicture->pkt_dts;
	fHeader.size_used = fDecodedDataSizeInBytes;
	fHeader.u.raw_video.display_line_width = fRawDecodedPicture->width;
	fHeader.u.raw_video.display_line_count = fRawDecodedPicture->height;
	fHeader.u.raw_video.bytes_per_row
		= CalculateBytesPerRowWithColorSpaceAndVideoWidth(fOutputColorSpace,
			fRawDecodedPicture->width);
	fHeader.u.raw_video.field_gamma = 1.0;
	fHeader.u.raw_video.field_sequence = fFrame;
	fHeader.u.raw_video.field_number = 0;
	fHeader.u.raw_video.pulldown_number = 0;
	fHeader.u.raw_video.first_active_line = 1;
	fHeader.u.raw_video.line_count = fRawDecodedPicture->height;

	ConvertAVCodecContextToVideoAspectWidthAndHeight(*fContext,
		fHeader.u.raw_video.pixel_width_aspect,
		fHeader.u.raw_video.pixel_height_aspect);

	TRACE("[v] start_time=%02d:%02d.%02d field_sequence=%lu\n",
		int((fHeader.start_time / 60000000) % 60),
		int((fHeader.start_time / 1000000) % 60),
		int((fHeader.start_time / 10000) % 100),
		fHeader.u.raw_video.field_sequence);
}


/*! \brief This function applies deinterlacing (only if needed) and color
	conversion to the video frame in fRawDecodedPicture.

	It is assumed that fRawDecodedPicture wasn't deinterlaced and color
	converted yet (otherwise this function behaves in unknown manners).

	You should only call this function when you	got a new picture decoded by
	the video decoder..

	When this function finishes the postprocessed video frame will be available
	in fPostProcessedDecodedPicture and fDecodedData (fDecodedDataSizeInBytes
	will be set accordingly).
*/
void
AVCodecDecoder::_DeinterlaceAndColorConvertVideoFrame()
{
	int displayWidth = fRawDecodedPicture->width;
	int displayHeight = fRawDecodedPicture->height;
	AVPicture deinterlacedPicture;
	bool useDeinterlacedPicture = false;

	if (fRawDecodedPicture->interlaced_frame) {
		AVPicture rawPicture;
		rawPicture.data[0] = fRawDecodedPicture->data[0];
		rawPicture.data[1] = fRawDecodedPicture->data[1];
		rawPicture.data[2] = fRawDecodedPicture->data[2];
		rawPicture.data[3] = fRawDecodedPicture->data[3];
		rawPicture.linesize[0] = fRawDecodedPicture->linesize[0];
		rawPicture.linesize[1] = fRawDecodedPicture->linesize[1];
		rawPicture.linesize[2] = fRawDecodedPicture->linesize[2];
		rawPicture.linesize[3] = fRawDecodedPicture->linesize[3];

		avpicture_alloc(&deinterlacedPicture, fContext->pix_fmt, displayWidth,
			displayHeight);

		if (avpicture_deinterlace(&deinterlacedPicture, &rawPicture,
				fContext->pix_fmt, displayWidth, displayHeight) < 0) {
			TRACE("[v] avpicture_deinterlace() - error\n");
		} else
			useDeinterlacedPicture = true;
	}

	// Some decoders do not set pix_fmt until they have decoded 1 frame
#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext == NULL) {
		fSwsContext = sws_getContext(displayWidth, displayHeight,
			fContext->pix_fmt, displayWidth, displayHeight,
			colorspace_to_pixfmt(fOutputColorSpace),
			SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}
#else
	if (fFormatConversionFunc == NULL) {
		fFormatConversionFunc = resolve_colorspace(fOutputColorSpace,
			fContext->pix_fmt, displayWidth, displayHeight);
	}
#endif

	fDecodedDataSizeInBytes = avpicture_get_size(
		colorspace_to_pixfmt(fOutputColorSpace), displayWidth, displayHeight);

	if (fDecodedData == NULL)
		fDecodedData
			= static_cast<uint8_t*>(malloc(fDecodedDataSizeInBytes));

	fPostProcessedDecodedPicture->data[0] = fDecodedData;
	fPostProcessedDecodedPicture->linesize[0]
		= fHeader.u.raw_video.bytes_per_row;

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
	if (fSwsContext != NULL) {
#else
	if (fFormatConversionFunc != NULL) {
#endif
		if (useDeinterlacedPicture) {
			AVFrame deinterlacedFrame;
			deinterlacedFrame.data[0] = deinterlacedPicture.data[0];
			deinterlacedFrame.data[1] = deinterlacedPicture.data[1];
			deinterlacedFrame.data[2] = deinterlacedPicture.data[2];
			deinterlacedFrame.data[3] = deinterlacedPicture.data[3];
			deinterlacedFrame.linesize[0]
				= deinterlacedPicture.linesize[0];
			deinterlacedFrame.linesize[1]
				= deinterlacedPicture.linesize[1];
			deinterlacedFrame.linesize[2]
				= deinterlacedPicture.linesize[2];
			deinterlacedFrame.linesize[3]
				= deinterlacedPicture.linesize[3];

#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
			sws_scale(fSwsContext, deinterlacedFrame.data,
				deinterlacedFrame.linesize, 0, displayHeight,
				fPostProcessedDecodedPicture->data,
				fPostProcessedDecodedPicture->linesize);
#else
			(*fFormatConversionFunc)(&deinterlacedFrame,
				fPostProcessedDecodedPicture, displayWidth, displayHeight);
#endif
		} else {
#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
			sws_scale(fSwsContext, fRawDecodedPicture->data,
				fRawDecodedPicture->linesize, 0, displayHeight,
				fPostProcessedDecodedPicture->data,
				fPostProcessedDecodedPicture->linesize);
#else
			(*fFormatConversionFunc)(fRawDecodedPicture,
				fPostProcessedDecodedPicture, displayWidth, displayHeight);
#endif
		}
	}

	if (fRawDecodedPicture->interlaced_frame)
		avpicture_free(&deinterlacedPicture);
}

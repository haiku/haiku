/*
 * Copyright 2009, Stephan Amßus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AVCodecEncoder.h"

#include <new>

#include <stdio.h>

extern "C" {
	#include "rational.h"
}


#undef TRACE
#define TRACE_AV_CODEC_ENCODER
#ifdef TRACE_AV_CODEC_ENCODER
#	define TRACE	printf
#	define TRACE_IO(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#endif


static const size_t kDefaultChunkBufferSize = 2 * 1024 * 1024;


AVCodecEncoder::AVCodecEncoder(uint32 codecID)
	:
	Encoder(),
	fCodec(NULL),
	fContext(avcodec_alloc_context()),
	fCodecInitDone(false),

	fFrame(avcodec_alloc_frame()),
	fSwsContext(NULL),

	fFramesWritten(0),

	fChunkBuffer(new(std::nothrow) uint8[kDefaultChunkBufferSize])
{
	TRACE("AVCodecEncoder::AVCodecEncoder()\n");

	fCodec = avcodec_find_encoder((enum CodecID)codecID);
	TRACE("  found AVCodec: %p\n", fCodec);

	memset(&fInputFormat, 0, sizeof(media_format));
}


AVCodecEncoder::~AVCodecEncoder()
{
	TRACE("AVCodecEncoder::~AVCodecEncoder()\n");

	if (fCodecInitDone)
		avcodec_close(fContext);

	sws_freeContext(fSwsContext);

	avpicture_free(&fDstFrame);
	// NOTE: Do not use avpicture_free() on fSrcFrame!! We fill the picture
	// data on the file with the media buffer data passed to Encode().

	if (fFrame != NULL) {
		fFrame->data[0] = NULL;
		fFrame->data[1] = NULL;
		fFrame->data[2] = NULL;
		fFrame->data[3] = NULL;

		fFrame->linesize[0] = 0;
		fFrame->linesize[1] = 0;
		fFrame->linesize[2] = 0;
		fFrame->linesize[3] = 0;
		free(fFrame);
	}

	free(fContext);

	delete[] fChunkBuffer;
}


status_t
AVCodecEncoder::AcceptedFormat(const media_format* proposedInputFormat,
	media_format* _acceptedInputFormat)
{
	TRACE("AVCodecEncoder::AcceptedFormat(%p, %p)\n", proposedInputFormat,
		_acceptedInputFormat);

	if (proposedInputFormat == NULL)
		return B_BAD_VALUE;

	if (_acceptedInputFormat != NULL) {
		memcpy(_acceptedInputFormat, proposedInputFormat,
			sizeof(media_format));
	}

	return B_OK;
}


status_t
AVCodecEncoder::SetUp(const media_format* inputFormat)
{
	TRACE("AVCodecEncoder::SetUp()\n");

	if (fContext == NULL || fCodec == NULL)
		return B_NO_INIT;

	if (inputFormat == NULL)
		return B_BAD_VALUE;

	if (fCodecInitDone) {
		fCodecInitDone = false;
		avcodec_close(fContext);
	}

	fInputFormat = *inputFormat;
	fFramesWritten = 0;

	if (fInputFormat.type == B_MEDIA_RAW_VIDEO) {
		// frame rate
		fContext->time_base.den = (int)fInputFormat.u.raw_video.field_rate;
		fContext->time_base.num = 1;
		// video size
		fContext->width = fInputFormat.u.raw_video.display.line_width;
		fContext->height = fInputFormat.u.raw_video.display.line_count;
//		fContext->gop_size = 12;
		// TODO: Fix pixel format or setup conversion method...
		fContext->pix_fmt = PIX_FMT_YUV420P;
//		fContext->rate_emu = 0;
		// TODO: Setup rate control:
//		fContext->rc_eq = NULL;
//		fContext->rc_max_rate = 0;
//		fContext->rc_min_rate = 0;
		fContext->sample_aspect_ratio.num
			= fInputFormat.u.raw_video.pixel_width_aspect;
		fContext->sample_aspect_ratio.den
			= fInputFormat.u.raw_video.pixel_height_aspect;
		if (fContext->sample_aspect_ratio.num == 0
			|| fContext->sample_aspect_ratio.den == 0) {
			av_reduce(&fContext->sample_aspect_ratio.num,
				&fContext->sample_aspect_ratio.den, fContext->width,
				fContext->height, 255);
		}

		// TODO: This should already happen in AcceptFormat()
		if (fInputFormat.u.raw_video.display.bytes_per_row == 0) {
			fInputFormat.u.raw_video.display.bytes_per_row
				= fContext->width * 4;
		}

		fFrame->pts = 0;

		// Allocate space for colorspace converted AVPicture
		// TODO: Check allocations...
		avpicture_alloc(&fDstFrame, fContext->pix_fmt, fContext->width,
			fContext->height);

		// Make the frame point to the data in the converted AVPicture
		fFrame->data[0] = fDstFrame.data[0];
		fFrame->data[1] = fDstFrame.data[1];
		fFrame->data[2] = fDstFrame.data[2];
		fFrame->data[3] = fDstFrame.data[3];

		fFrame->linesize[0] = fDstFrame.linesize[0];
		fFrame->linesize[1] = fDstFrame.linesize[1];
		fFrame->linesize[2] = fDstFrame.linesize[2];
		fFrame->linesize[3] = fDstFrame.linesize[3];

		// TODO: Use actual pixel format from media_format!
		fSwsContext = sws_getContext(fContext->width, fContext->height,
			PIX_FMT_RGB32, fContext->width, fContext->height,
			fContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	} else if (fInputFormat.type == B_MEDIA_RAW_AUDIO) {
		// frame rate
		fContext->sample_rate = (int)fInputFormat.u.raw_audio.frame_rate;
		fContext->time_base.den = (int)fInputFormat.u.raw_audio.frame_rate;
		fContext->time_base.num = 1;
		// channels
		fContext->channels = fInputFormat.u.raw_audio.channel_count;
		switch (fInputFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				fContext->sample_fmt = SAMPLE_FMT_FLT;
				break;
			case media_raw_audio_format::B_AUDIO_DOUBLE:
				fContext->sample_fmt = SAMPLE_FMT_DBL;
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				fContext->sample_fmt = SAMPLE_FMT_S32;
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				fContext->sample_fmt = SAMPLE_FMT_S16;
				break;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				fContext->sample_fmt = SAMPLE_FMT_U8;
				break;

			case media_raw_audio_format::B_AUDIO_CHAR:
			default:
				return B_MEDIA_BAD_FORMAT;
				break;
		}
		if (fInputFormat.u.raw_audio.channel_mask == 0) {
			// guess the channel mask...
			switch (fInputFormat.u.raw_audio.channel_count) {
				default:
				case 2:
					fContext->channel_layout = CH_LAYOUT_STEREO;
					break;
				case 1:
					fContext->channel_layout = CH_LAYOUT_MONO;
					break;
				case 3:
					fContext->channel_layout = CH_LAYOUT_SURROUND;
					break;
				case 4:
					fContext->channel_layout = CH_LAYOUT_QUAD;
					break;
				case 5:
					fContext->channel_layout = CH_LAYOUT_5POINT0;
					break;
				case 6:
					fContext->channel_layout = CH_LAYOUT_5POINT1;
					break;
				case 8:
					fContext->channel_layout = CH_LAYOUT_7POINT1;
					break;
				case 10:
					fContext->channel_layout = CH_LAYOUT_7POINT1_WIDE;
					break;
			}
		} else {
			// The bits match 1:1 for media_multi_channels and FFmpeg defines.
			fContext->channel_layout = fInputFormat.u.raw_audio.channel_mask;
		}
	} else {
		return B_NOT_SUPPORTED;
	}

	// Open the codec
	int result = avcodec_open(fContext, fCodec);
	fCodecInitDone = (result >= 0);

	TRACE("  avcodec_open(): %d\n", result);

	return fCodecInitDone ? B_OK : B_ERROR;
}


status_t
AVCodecEncoder::GetEncodeParameters(encode_parameters* parameters) const
{
	TRACE("AVCodecEncoder::GetEncodeParameters(%p)\n", parameters);

	return B_NOT_SUPPORTED;
}


status_t
AVCodecEncoder::SetEncodeParameters(encode_parameters* parameters) const
{
	TRACE("AVCodecEncoder::SetEncodeParameters(%p)\n", parameters);

	return B_NOT_SUPPORTED;
}


status_t
AVCodecEncoder::Encode(const void* buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE("AVCodecEncoder::Encode(%p, %lld, %p)\n", buffer, frameCount, info);

	if (!fCodecInitDone)
		return B_NO_INIT;

	if (fInputFormat.type == B_MEDIA_RAW_AUDIO)
		return _EncodeAudio(buffer, frameCount, info);
	else if (fInputFormat.type == B_MEDIA_RAW_VIDEO)
		return _EncodeVideo(buffer, frameCount, info);
	else
		return B_NO_INIT;
}


// #pragma mark -


status_t
AVCodecEncoder::_EncodeAudio(const void* _buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE("AVCodecEncoder::_EncodeAudio(%p, %lld, %p)\n", _buffer, frameCount,
		info);

	if (fChunkBuffer == NULL)
		return B_NO_MEMORY;

	status_t ret = B_OK;

	const uint8* buffer = reinterpret_cast<const uint8*>(_buffer);

	size_t inputSampleSize = fInputFormat.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	size_t inputFrameSize = inputSampleSize
		* fInputFormat.u.raw_audio.channel_count;

	size_t outSampleSize = av_get_bits_per_sample_format(
		fContext->sample_fmt) / 8;
	size_t outSize = outSampleSize * fContext->channels;
	TRACE("  sampleSize: %ld/%ld, frameSize: %ld/%ld\n",
		inputSampleSize, inputFrameSize, outSampleSize, outSize);

	size_t bufferSize = frameCount * inputFrameSize;
	bufferSize = min_c(bufferSize, kDefaultChunkBufferSize);

	while (frameCount > 0) {
		if (frameCount < fContext->frame_size) {
			TRACE("  ERROR: too few frames left! (left: %lld, needed: %d)\n",
				frameCount, fContext->frame_size);
			// TODO: Handle this some way. Maybe use an av_fifo to buffer data?
			return B_ERROR;
		}

		int chunkFrames = fContext->frame_size;

		TRACE("  frames left: %lld, chunk frames: %d\n",
			frameCount, chunkFrames);

		// Encode one audio chunk/frame.
		int usedBytes = avcodec_encode_audio(fContext, fChunkBuffer,
			bufferSize, reinterpret_cast<const short*>(buffer));

		if (usedBytes < 0) {
			TRACE("  avcodec_encode_video() failed: %d\n", usedBytes);
			return B_ERROR;
		}

		// Setup media_encode_info, most important is the time stamp.
		info->start_time = (bigtime_t)(fFramesWritten * 1000000LL
			/ fInputFormat.u.raw_audio.frame_rate);

		// Write the chunk
		ret = WriteChunk(fChunkBuffer, usedBytes, info);
		if (ret != B_OK)
			break;

		size_t framesWritten = usedBytes / inputFrameSize;
		if (chunkFrames == 1) {
			// For PCM data:
			framesWritten = usedBytes / inputFrameSize;
		} else {
			// For encoded audio:
			framesWritten = chunkFrames * inputFrameSize;
		}

		// Skip to next chunk of buffer.
		fFramesWritten += framesWritten;
		frameCount -= framesWritten;
		buffer += usedBytes;
	}

	return ret;
}


status_t
AVCodecEncoder::_EncodeVideo(const void* buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE_IO("AVCodecEncoder::_EncodeVideo(%p, %lld, %p)\n", buffer, frameCount,
		info);

	if (fChunkBuffer == NULL)
		return B_NO_MEMORY;

	status_t ret = B_OK;

	while (frameCount > 0) {
		size_t bpr = fInputFormat.u.raw_video.display.bytes_per_row;
		size_t bufferSize = fInputFormat.u.raw_video.display.line_count * bpr;

		// We should always get chunky bitmaps, so this code should be safe.
		fSrcFrame.data[0] = (uint8_t*)buffer;
		fSrcFrame.linesize[0] = bpr;

		// Run the pixel format conversion
		sws_scale(fSwsContext, fSrcFrame.data, fSrcFrame.linesize, 0,
			fInputFormat.u.raw_video.display.line_count, fDstFrame.data,
			fDstFrame.linesize);

		// TODO: Look into this... avcodec.h says we need to set it.
		fFrame->pts++;

		// Encode one video chunk/frame.
		int usedBytes = avcodec_encode_video(fContext, fChunkBuffer,
			kDefaultChunkBufferSize, fFrame);

		if (usedBytes < 0) {
			TRACE("  avcodec_encode_video() failed: %d\n", usedBytes);
			return B_ERROR;
		}

		// Setup media_encode_info, most important is the time stamp.
		info->start_time = (bigtime_t)(fFramesWritten * 1000000LL
			/ fInputFormat.u.raw_video.field_rate);

		// Write the chunk
		ret = WriteChunk(fChunkBuffer, usedBytes, info);
		if (ret != B_OK)
			break;

		// Skip to the next frame (but usually, there is only one to encode
		// for video).
		frameCount--;
		fFramesWritten++;
		buffer = (const void*)((const uint8*)buffer + bufferSize);
	}

	return ret;
}


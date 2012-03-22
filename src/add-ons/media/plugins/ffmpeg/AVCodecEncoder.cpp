/*
 * Copyright 2009-2010, Stephan Amßus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AVCodecEncoder.h"

#include <new>

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Roster.h>

extern "C" {
	#include "rational.h"
}

#include "EncoderTable.h"
#include "gfx_util.h"


#undef TRACE
//#define TRACE_AV_CODEC_ENCODER
#ifdef TRACE_AV_CODEC_ENCODER
#	define TRACE	printf
#	define TRACE_IO(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#endif


static const size_t kDefaultChunkBufferSize = 2 * 1024 * 1024;


AVCodecEncoder::AVCodecEncoder(uint32 codecID, int bitRateScale)
	:
	Encoder(),
	fBitRateScale(bitRateScale),
	fCodecID((enum CodecID)codecID),
	fCodec(NULL),
	fOwnContext(avcodec_alloc_context()),
	fContext(fOwnContext),
	fCodecInitStatus(CODEC_INIT_NEEDED),

	fFrame(avcodec_alloc_frame()),
	fSwsContext(NULL),

	fFramesWritten(0),

	fChunkBuffer(new(std::nothrow) uint8[kDefaultChunkBufferSize])
{
	TRACE("AVCodecEncoder::AVCodecEncoder()\n");

	if (fCodecID > 0) {
		fCodec = avcodec_find_encoder(fCodecID);
		TRACE("  found AVCodec for %u: %p\n", fCodecID, fCodec);
	}

	memset(&fInputFormat, 0, sizeof(media_format));

	fAudioFifo = av_fifo_alloc(0);

	fDstFrame.data[0] = NULL;
	fDstFrame.data[1] = NULL;
	fDstFrame.data[2] = NULL;
	fDstFrame.data[3] = NULL;

	fDstFrame.linesize[0] = 0;
	fDstFrame.linesize[1] = 0;
	fDstFrame.linesize[2] = 0;
	fDstFrame.linesize[3] = 0;

	// Initial parameters, so we know if the user changed them
	fEncodeParameters.avg_field_size = 0;
	fEncodeParameters.max_field_size = 0;
	fEncodeParameters.quality = 1.0f;
}


AVCodecEncoder::~AVCodecEncoder()
{
	TRACE("AVCodecEncoder::~AVCodecEncoder()\n");

	_CloseCodecIfNeeded();

	if (fSwsContext != NULL)
		sws_freeContext(fSwsContext);

	av_fifo_free(fAudioFifo);

	avpicture_free(&fDstFrame);
	// NOTE: Do not use avpicture_free() on fSrcFrame!! We fill the picture
	// data on the fly with the media buffer data passed to Encode().

	if (fFrame != NULL) {
		fFrame->data[0] = NULL;
		fFrame->data[1] = NULL;
		fFrame->data[2] = NULL;
		fFrame->data[3] = NULL;

		fFrame->linesize[0] = 0;
		fFrame->linesize[1] = 0;
		fFrame->linesize[2] = 0;
		fFrame->linesize[3] = 0;
		av_free(fFrame);
	}

	av_free(fOwnContext);

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

	if (fContext == NULL)
		return B_NO_INIT;

	if (inputFormat == NULL)
		return B_BAD_VALUE;

	// Codec IDs for raw-formats may need to be figured out here.
	if (fCodec == NULL && fCodecID == CODEC_ID_NONE) {
		fCodecID = raw_audio_codec_id_for(*inputFormat);
		if (fCodecID != CODEC_ID_NONE)
			fCodec = avcodec_find_encoder(fCodecID);
	}
	if (fCodec == NULL) {
		TRACE("  encoder not found!\n");
		return B_NO_INIT;
	}

	_CloseCodecIfNeeded();

	fInputFormat = *inputFormat;
	fFramesWritten = 0;

	const uchar* userData = inputFormat->user_data;
	if (*(uint32*)userData == 'ffmp') {
		userData += sizeof(uint32);
		// The Writer plugin used is the FFmpeg plugin. It stores the
		// AVCodecContext pointer in the user data section. Use this
		// context instead of our own. It requires the Writer living in
		// the same team, of course.
		app_info appInfo;
		if (be_app->GetAppInfo(&appInfo) == B_OK
			&& *(team_id*)userData == appInfo.team) {
			userData += sizeof(team_id);
			// Use the AVCodecContext from the Writer. This works better
			// than using our own context with some encoders.
			fContext = *(AVCodecContext**)userData;
		}
	}

	return _Setup();
}


status_t
AVCodecEncoder::GetEncodeParameters(encode_parameters* parameters) const
{
	TRACE("AVCodecEncoder::GetEncodeParameters(%p)\n", parameters);

// TODO: Implement maintaining an automatically calculated bit_rate versus
// a user specified (via SetEncodeParameters()) bit_rate. At this point, the
// fContext->bit_rate may not yet have been specified (_Setup() was never
// called yet). So it cannot work like the code below, but in any case, it's
// showing how to convert between the values (albeit untested).
//	int avgBytesPerSecond = fContext->bit_rate / 8;
//	int maxBytesPerSecond = (fContext->bit_rate
//		+ fContext->bit_rate_tolerance) / 8;
//
//	if (fInputFormat.type == B_MEDIA_RAW_AUDIO) {
//		fEncodeParameters.avg_field_size = (int32)(avgBytesPerSecond
//			/ fInputFormat.u.raw_audio.frame_rate);
//		fEncodeParameters.max_field_size = (int32)(maxBytesPerSecond
//			/ fInputFormat.u.raw_audio.frame_rate);
//	} else if (fInputFormat.type == B_MEDIA_RAW_VIDEO) {
//		fEncodeParameters.avg_field_size = (int32)(avgBytesPerSecond
//			/ fInputFormat.u.raw_video.field_rate);
//		fEncodeParameters.max_field_size = (int32)(maxBytesPerSecond
//			/ fInputFormat.u.raw_video.field_rate);
//	}

	parameters->quality = fEncodeParameters.quality;

	return B_OK;
}


status_t
AVCodecEncoder::SetEncodeParameters(encode_parameters* parameters)
{
	TRACE("AVCodecEncoder::SetEncodeParameters(%p)\n", parameters);

	if (fFramesWritten > 0)
		return B_NOT_SUPPORTED;

	fEncodeParameters.quality = parameters->quality;
	TRACE("  quality: %.5f\n", parameters->quality);
	if (fEncodeParameters.quality == 0.0f) {
		TRACE("  using default quality (1.0)\n");
		fEncodeParameters.quality = 1.0f;
	}

// TODO: Auto-bit_rate versus user supplied. See above.
//	int avgBytesPerSecond = 0;
//	int maxBytesPerSecond = 0;
//
//	if (fInputFormat.type == B_MEDIA_RAW_AUDIO) {
//		avgBytesPerSecond = (int)(parameters->avg_field_size
//			* fInputFormat.u.raw_audio.frame_rate);
//		maxBytesPerSecond = (int)(parameters->max_field_size
//			* fInputFormat.u.raw_audio.frame_rate);
//	} else if (fInputFormat.type == B_MEDIA_RAW_VIDEO) {
//		avgBytesPerSecond = (int)(parameters->avg_field_size
//			* fInputFormat.u.raw_video.field_rate);
//		maxBytesPerSecond = (int)(parameters->max_field_size
//			* fInputFormat.u.raw_video.field_rate);
//	}
//
//	if (maxBytesPerSecond < avgBytesPerSecond)
//		maxBytesPerSecond = avgBytesPerSecond;
//
//	// Reset these, so we can tell the difference between uninitialized
//	// and initialized...
//	if (avgBytesPerSecond > 0) {
//		fContext->bit_rate = avgBytesPerSecond * 8;
//		fContext->bit_rate_tolerance = (maxBytesPerSecond
//			- avgBytesPerSecond) * 8;
//		fBitRateControlledByUser = true;
//	}

	return _Setup();
}


status_t
AVCodecEncoder::Encode(const void* buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE("AVCodecEncoder::Encode(%p, %lld, %p)\n", buffer, frameCount, info);

	if (!_OpenCodecIfNeeded())
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
AVCodecEncoder::_Setup()
{
	TRACE("AVCodecEncoder::_Setup\n");

	int rawBitRate;

	if (fInputFormat.type == B_MEDIA_RAW_VIDEO) {
		TRACE("  B_MEDIA_RAW_VIDEO\n");
		// frame rate
		fContext->time_base.den = (int)fInputFormat.u.raw_video.field_rate;
		fContext->time_base.num = 1;
		// video size
		fContext->width = fInputFormat.u.raw_video.display.line_width;
		fContext->height = fInputFormat.u.raw_video.display.line_count;
		fContext->gop_size = 12;
		// TODO: Fix pixel format or setup conversion method...
		for (int i = 0; fCodec->pix_fmts[i] != PIX_FMT_NONE; i++) {
			// Use the last supported pixel format, which we hope is the
			// one with the best quality.
			fContext->pix_fmt = fCodec->pix_fmts[i];
		}

		// TODO: Setup rate control:
//		fContext->rate_emu = 0;
//		fContext->rc_eq = NULL;
//		fContext->rc_max_rate = 0;
//		fContext->rc_min_rate = 0;
		// TODO: Try to calculate a good bit rate...
		rawBitRate = (int)(fContext->width * fContext->height * 2
			* fInputFormat.u.raw_video.field_rate) * 8;

		// Pixel aspect ratio
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

		fSwsContext = sws_getContext(fContext->width, fContext->height,
			colorspace_to_pixfmt(fInputFormat.u.raw_video.display.format),
			fContext->width, fContext->height,
			fContext->pix_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	} else if (fInputFormat.type == B_MEDIA_RAW_AUDIO) {
		TRACE("  B_MEDIA_RAW_AUDIO\n");
		// frame rate
		fContext->sample_rate = (int)fInputFormat.u.raw_audio.frame_rate;
		// channels
		fContext->channels = fInputFormat.u.raw_audio.channel_count;
		// raw bitrate
		rawBitRate = fContext->sample_rate * fContext->channels
			* (fInputFormat.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK) * 8;
		// sample format
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
		TRACE("  UNSUPPORTED MEDIA TYPE!\n");
		return B_NOT_SUPPORTED;
	}

	// TODO: Support letting the user overwrite this via
	// SetEncodeParameters(). See comments there...
	int wantedBitRate = (int)(rawBitRate / fBitRateScale
		* fEncodeParameters.quality);
	if (wantedBitRate == 0)
		wantedBitRate = (int)(rawBitRate / fBitRateScale);

	fContext->bit_rate = wantedBitRate;

	if (fInputFormat.type == B_MEDIA_RAW_AUDIO) {
		// Some audio encoders support certain bitrates only. Use the
		// closest match to the wantedBitRate.
		const int kBitRates[] = {
			32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000,
			160000, 192000, 224000, 256000, 320000, 384000, 448000, 512000,
			576000, 640000
		};
		int diff = wantedBitRate;
		for (int i = 0; i < sizeof(kBitRates) / sizeof(int); i++) {
			int currentDiff = abs(wantedBitRate - kBitRates[i]);
			if (currentDiff < diff) {
				fContext->bit_rate = kBitRates[i];
				diff = currentDiff;
			} else
				break;
		}
	}

	TRACE("  rawBitRate: %d, wantedBitRate: %d (%.1f), "
		"context bitrate: %d\n", rawBitRate, wantedBitRate,
		fEncodeParameters.quality, fContext->bit_rate);

	// Add some known fixes from the FFmpeg API example:
	if (fContext->codec_id == CODEC_ID_MPEG2VIDEO) {
		// Just for testing, we also add B frames */
		fContext->max_b_frames = 2;
    } else if (fContext->codec_id == CODEC_ID_MPEG1VIDEO){
		// Needed to avoid using macroblocks in which some coeffs overflow.
		// This does not happen with normal video, it just happens here as
		// the motion of the chroma plane does not match the luma plane.
		fContext->mb_decision = 2;
    }

	// Unfortunately, we may fail later, when we try to open the codec
	// for real... but we need to delay this because we still allow
	// parameter/quality changes.
	return B_OK;
}


bool
AVCodecEncoder::_OpenCodecIfNeeded()
{
	if (fContext != fOwnContext) {
		// We are using the AVCodecContext of the AVFormatWriter plugin,
		// and don't maintain it's open/close state.
		return true;
	}

	if (fCodecInitStatus == CODEC_INIT_DONE)
		return true;

	if (fCodecInitStatus == CODEC_INIT_FAILED)
		return false;

	// Open the codec
	int result = avcodec_open(fContext, fCodec);
	if (result >= 0)
		fCodecInitStatus = CODEC_INIT_DONE;
	else
		fCodecInitStatus = CODEC_INIT_FAILED;

	TRACE("  avcodec_open(%p, %p): %d\n", fContext, fCodec, result);

	return fCodecInitStatus == CODEC_INIT_DONE;

}


void
AVCodecEncoder::_CloseCodecIfNeeded()
{
	if (fContext != fOwnContext) {
		// See _OpenCodecIfNeeded().
		return;
	}

	if (fCodecInitStatus == CODEC_INIT_DONE) {
		avcodec_close(fContext);
		fCodecInitStatus = CODEC_INIT_NEEDED;
	}
}


static const int64 kNoPTSValue = 0x8000000000000000LL;
	// NOTE: For some reasons, I have trouble with the avcodec.h define:
	// #define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)
	// INT64_C is not defined here.

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

	size_t bufferSize = frameCount * inputFrameSize;
	bufferSize = min_c(bufferSize, kDefaultChunkBufferSize);

	if (fContext->frame_size > 1) {
		// Encoded audio. Things work differently from raw audio. We need
		// the fAudioFifo to pipe data.
		if (av_fifo_realloc2(fAudioFifo,
				av_fifo_size(fAudioFifo) + bufferSize) < 0) {
			TRACE("  av_fifo_realloc2() failed\n");
            return B_NO_MEMORY;
        }
        av_fifo_generic_write(fAudioFifo, const_cast<uint8*>(buffer),
        	bufferSize, NULL);

		int frameBytes = fContext->frame_size * inputFrameSize;
		uint8* tempBuffer = new(std::nothrow) uint8[frameBytes];
		if (tempBuffer == NULL)
			return B_NO_MEMORY;

		// Encode as many chunks as can be read from the FIFO.
		while (av_fifo_size(fAudioFifo) >= frameBytes) {
			av_fifo_generic_read(fAudioFifo, tempBuffer, frameBytes, NULL);

			ret = _EncodeAudio(tempBuffer, frameBytes, fContext->frame_size,
				info);
			if (ret != B_OK)
				break;
		}

		delete[] tempBuffer;
	} else {
		// Raw audio. The number of bytes returned from avcodec_encode_audio()
		// is always the same as the number of input bytes.
		return _EncodeAudio(buffer, bufferSize, frameCount,
			info);
	}

	return ret;
}


status_t
AVCodecEncoder::_EncodeAudio(const uint8* buffer, size_t bufferSize,
	int64 frameCount, media_encode_info* info)
{
	// Encode one audio chunk/frame. The bufferSize has already been adapted
	// to the needed size for fContext->frame_size, or we are writing raw
	// audio.
	int usedBytes = avcodec_encode_audio(fContext, fChunkBuffer,
		bufferSize, reinterpret_cast<const short*>(buffer));

	if (usedBytes < 0) {
		TRACE("  avcodec_encode_audio() failed: %d\n", usedBytes);
		return B_ERROR;
	}
	if (usedBytes == 0)
		return B_OK;

//	// Maybe we need to use this PTS to calculate start_time:
//	if (fContext->coded_frame->pts != kNoPTSValue) {
//		TRACE("  codec frame PTS: %lld (codec time_base: %d/%d)\n",
//			fContext->coded_frame->pts, fContext->time_base.num,
//			fContext->time_base.den);
//	} else {
//		TRACE("  codec frame PTS: N/A (codec time_base: %d/%d)\n",
//			fContext->time_base.num, fContext->time_base.den);
//	}

	// Setup media_encode_info, most important is the time stamp.
	info->start_time = (bigtime_t)(fFramesWritten * 1000000LL
		/ fInputFormat.u.raw_audio.frame_rate);
	info->flags = B_MEDIA_KEY_FRAME;

	// Write the chunk
	status_t ret = WriteChunk(fChunkBuffer, usedBytes, info);
	if (ret != B_OK) {
		TRACE("  error writing chunk: %s\n", strerror(ret));
		return ret;
	}

	fFramesWritten += frameCount;

	return B_OK;
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

		// Encode one video chunk/frame.
		int usedBytes = avcodec_encode_video(fContext, fChunkBuffer,
			kDefaultChunkBufferSize, fFrame);

		// avcodec.h says we need to set it.
		fFrame->pts++;

		if (usedBytes < 0) {
			TRACE("  avcodec_encode_video() failed: %d\n", usedBytes);
			return B_ERROR;
		}

		// Maybe we need to use this PTS to calculate start_time:
		if (fContext->coded_frame->pts != kNoPTSValue) {
			TRACE("  codec frame PTS: %lld (codec time_base: %d/%d)\n",
				fContext->coded_frame->pts, fContext->time_base.num,
				fContext->time_base.den);
		} else {
			TRACE("  codec frame PTS: N/A (codec time_base: %d/%d)\n",
				fContext->time_base.num, fContext->time_base.den);
		}

		// Setup media_encode_info, most important is the time stamp.
		info->start_time = (bigtime_t)(fFramesWritten * 1000000LL
			/ fInputFormat.u.raw_video.field_rate);

		info->flags = 0;
		if (fContext->coded_frame->key_frame)
			info->flags |= B_MEDIA_KEY_FRAME;

		// Write the chunk
		ret = WriteChunk(fChunkBuffer, usedBytes, info);
		if (ret != B_OK) {
			TRACE("  error writing chunk: %s\n", strerror(ret));
			break;
		}

		// Skip to the next frame (but usually, there is only one to encode
		// for video).
		frameCount--;
		fFramesWritten++;
		buffer = (const void*)((const uint8*)buffer + bufferSize);
	}

	return ret;
}


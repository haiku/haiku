/*
 * Copyright (C) 2001 Carlos Hasan.
 * Copyright (C) 2001 François Revol.
 * Copyright (C) 2001 Axel Dörfler.
 * Copyright (C) 2004 Marcus Overhagen.
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright (C) 2015 Adrien Destugues <pulkomandy@pulkomandy.tk>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef AVCODEC_DECODER_H
#define AVCODEC_DECODER_H

//! libavcodec based decoder for Haiku


#include <MediaFormats.h>


extern "C" {
	#include "avcodec.h"
#if LIBAVCODEC_VERSION_INT >= ((57 << 16) | (0 << 8))
	#include "avfilter.h"
	#include "buffersink.h"
	#include "buffersrc.h"
#endif
	#include "swresample.h"
	#include "swscale.h"
}


#include "DecoderPlugin.h"
#include "ReaderPlugin.h"

#include "CodecTable.h"
#include "gfx_util.h"


#ifdef __x86_64
#define USE_SWS_FOR_COLOR_SPACE_CONVERSION 1
#else
#define USE_SWS_FOR_COLOR_SPACE_CONVERSION 0
// NOTE: David's color space conversion is much faster than the FFmpeg
// version. Perhaps the SWS code can be used for unsupported conversions?
// Otherwise the alternative code could simply be removed from this file.
#endif


class AVCodecDecoder : public Decoder {
public:
						AVCodecDecoder();

	virtual				~AVCodecDecoder();

	virtual	void		GetCodecInfo(media_codec_info* mci);

	virtual	status_t	Setup(media_format* ioEncodedFormat,
							const void* infoBuffer, size_t infoSize);

	virtual	status_t	NegotiateOutputFormat(media_format* inOutFormat);

	virtual	status_t	Decode(void* outBuffer, int64* outFrameCount,
							media_header* mediaHeader,
							media_decode_info* info);

	virtual	status_t	SeekedTo(int64 trame, bigtime_t time);


private:
			void		_ResetTempPacket();

			status_t	_NegotiateAudioOutputFormat(media_format* inOutFormat);

			status_t	_NegotiateVideoOutputFormat(media_format* inOutFormat);

			status_t	_DecodeAudio(void* outBuffer, int64* outFrameCount,
							media_header* mediaHeader,
							media_decode_info* info);

			status_t	_DecodeVideo(void* outBuffer, int64* outFrameCount,
							media_header* mediaHeader,
							media_decode_info* info);

			status_t	_DecodeNextAudioFrame();
			void		_ApplyEssentialAudioContainerPropertiesToContext();
			status_t	_ResetRawDecodedAudio();
			void		_CheckAndFixConditionsThatHintAtBrokenAudioCodeBelow();
			void		_MoveAudioFramesToRawDecodedAudioAndUpdateStartTimes();
			status_t	_DecodeNextAudioFrameChunk();
			status_t	_DecodeSomeAudioFramesIntoEmptyDecodedDataBuffer();
			void		_UpdateMediaHeaderForAudioFrame();

			status_t	_DecodeNextVideoFrame();
			void		_ApplyEssentialVideoContainerPropertiesToContext();
			status_t	_LoadNextChunkIfNeededAndAssignStartTime();
			status_t	_CopyChunkToChunkBufferAndAddPadding(const void* chunk,
							size_t chunkSize);
			status_t	_HandleNewVideoFrameAndUpdateSystemState();
			status_t	_FlushOneVideoFrameFromDecoderBuffer();
			void		_UpdateMediaHeaderForVideoFrame();
			status_t	_DeinterlaceAndColorConvertVideoFrame();

#if LIBAVCODEC_VERSION_INT >= ((57 << 16) | (0 << 8))
			// video deinterlace filter graph
			status_t	_InitFilterGraph(enum AVPixelFormat pixfmt,
							int32 width, int32 height);
			status_t	_ProcessFilterGraph(AVPicture *dst,
							const AVPicture *src, enum AVPixelFormat pixfmt,
							int32 width, int32 height);
#endif

			media_header		fHeader;
									// Contains the properties of the current
									// decoded audio / video frame

			media_format		fInputFormat;

			int64				fFrame;
			bool				fIsAudio;

			// FFmpeg related members
			AVCodec*			fCodec;
			AVCodecContext*		fContext;
			SwrContext*			fResampleContext;
			uint8_t*			fDecodedData;
			size_t				fDecodedDataSizeInBytes;
			AVFrame*			fPostProcessedDecodedPicture;
			AVFrame*			fRawDecodedPicture;
			AVFrame*			fRawDecodedAudio;

			bool 				fCodecInitDone;

			gfx_convert_func	fFormatConversionFunc;
#if USE_SWS_FOR_COLOR_SPACE_CONVERSION
			SwsContext*			fSwsContext;
#endif

			char*				fExtraData;
			int					fExtraDataSize;
			int					fBlockAlign;

			color_space			fOutputColorSpace;
			int32				fOutputFrameCount;
			float				fOutputFrameRate;
			int					fOutputFrameSize;
									// sample size * channel count
			int					fInputFrameSize;
									// sample size * channel count
									// or just sample size for planar formats

			uint8_t*			fChunkBuffer;
			size_t				fChunkBufferSize;
			bool				fAudioDecodeError;

			AVFrame*			fDecodedDataBuffer;
			int32				fDecodedDataBufferOffset;
			int32				fDecodedDataBufferSize;

			AVPacket			fTempPacket;

#if LIBAVCODEC_VERSION_INT >= ((57 << 16) | (0 << 8))
			// video deinterlace feature
			AVFilterContext*	fBufferSinkContext;
			AVFilterContext*	fBufferSourceContext;
			AVFilterGraph*		fFilterGraph;
			AVFrame*			fFilterFrame;
			int32				fLastWidth;
			int32				fLastHeight;
			enum AVPixelFormat	fLastPixfmt;
#endif
};

#endif // AVCODEC_DECODER_H

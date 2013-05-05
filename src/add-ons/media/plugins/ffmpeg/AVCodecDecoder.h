/*
 * Copyright (C) 2001 Carlos Hasan.
 * Copyright (C) 2001 François Revol.
 * Copyright (C) 2001 Axel Dörfler.
 * Copyright (C) 2004 Marcus Overhagen.
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef AVCODEC_DECODER_H
#define AVCODEC_DECODER_H

//! libavcodec based decoder for Haiku

#include <MediaFormats.h>

extern "C" {
	#include "avcodec.h"
	#include "swscale.h"
}

#include "DecoderPlugin.h"
#include "ReaderPlugin.h"

#include "CodecTable.h"
#include "gfx_util.h"


class AVCodecDecoder : public Decoder {
public:
								AVCodecDecoder();

	virtual						~AVCodecDecoder();

	virtual	void				GetCodecInfo(media_codec_info* mci);

	virtual	status_t			Setup(media_format* ioEncodedFormat,
								   const void* infoBuffer, size_t infoSize);

	virtual	status_t			NegotiateOutputFormat(
									media_format* inOutFormat);

	virtual	status_t			Decode(void* outBuffer, int64* outFrameCount,
									media_header* mediaHeader,
									media_decode_info* info);

	virtual	status_t			SeekedTo(int64 trame, bigtime_t time);


private:
			status_t			_NegotiateAudioOutputFormat(
									media_format* inOutFormat);

			status_t			_NegotiateVideoOutputFormat(
									media_format* inOutFormat);

			status_t			_DecodeAudio(void* outBuffer,
									int64* outFrameCount,
									media_header* mediaHeader,
									media_decode_info* info);

			status_t			_DecodeVideo(void* outBuffer,
									int64* outFrameCount,
									media_header* mediaHeader,
									media_decode_info* info);


			media_header		fHeader;
			media_format		fInputFormat;
			media_raw_video_format fOutputVideoFormat;

			int64				fFrame;
			bool				fIsAudio;

			// FFmpeg related members
			AVCodec*			fCodec;
			AVCodecContext*		fContext;
			AVFrame*			fInputPicture;
			AVFrame*			fOutputPicture;

			bool 				fCodecInitDone;

			gfx_convert_func	fFormatConversionFunc;
			SwsContext*			fSwsContext;

			char*				fExtraData;
			int					fExtraDataSize;
			int					fBlockAlign;

			bigtime_t			fStartTime;
			int32				fOutputFrameCount;
			float				fOutputFrameRate;
			int					fOutputFrameSize;
									// sample size * channel count

			const void*			fChunkBuffer;
			int32				fChunkBufferOffset;
			size_t				fChunkBufferSize;
			bool				fAudioDecodeError;

			AVFrame*			fOutputFrame;
			int32				fOutputBufferOffset;
			int32				fOutputBufferSize;

			AVPacket			fTempPacket;
};

#endif // AVCODEC_DECODER_H

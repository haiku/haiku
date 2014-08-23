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
			void		_ResetRawDecodedAudio();
			void		_CheckAndFixConditionsThatHintAtBrokenAudioCodeBelow();
			void		_MoveAudioFramesToRawDecodedAudioAndUpdateStartTimes();
			status_t	_DecodeNextAudioFrameChunk();
			status_t	_LoadNextAudioChunkIfNeededAndAssignStartTime();
			status_t	_DecodeSomeAudioFramesIntoEmptyDecodedDataBuffer();
			void		_UpdateMediaHeaderForAudioFrame();

			status_t	_DecodeNextVideoFrame();
			void		_ApplyEssentialVideoContainerPropertiesToContext();
			status_t	_LoadNextVideoChunkIfNeededAndAssignStartTime();
							// TODO: Remove the "Video" word once
							// the audio path is responsible for
							// freeing the chunk buffer, too.
			status_t	_CopyChunkToVideoChunkBufferAndAddPadding(
							const void* chunk, size_t chunkSize);
							// TODO: Remove the "Video" word once
							// the audio path is responsible for
							// freeing the chunk buffer, too.
			void		_HandleNewVideoFrameAndUpdateSystemState();
			status_t	_FlushOneVideoFrameFromDecoderBuffer();
			void		_UpdateMediaHeaderForVideoFrame();
			void		_DeinterlaceAndColorConvertVideoFrame();


			media_header		fHeader;
									// Contains the properties of the current
									// decoded audio / video frame

			media_format		fInputFormat;

			int64				fFrame;
			bool				fIsAudio;

			// FFmpeg related members
			AVCodec*			fCodec;
			AVCodecContext*		fContext;
			uint8_t*			fDecodedData;
			size_t				fDecodedDataSizeInBytes;
			AVFrame*			fPostProcessedDecodedPicture;
			AVFrame*			fRawDecodedPicture;
			AVFrame*			fRawDecodedAudio;

			bool 				fCodecInitDone;

			gfx_convert_func	fFormatConversionFunc;
			SwsContext*			fSwsContext;

			char*				fExtraData;
			int					fExtraDataSize;
			int					fBlockAlign;

			color_space			fOutputColorSpace;
			int32				fOutputFrameCount;
			float				fOutputFrameRate;
			int					fOutputFrameSize;
									// sample size * channel count

			const void*			fChunkBuffer;
			uint8_t*			fVideoChunkBuffer;
									// TODO: Remove and use fChunkBuffer again
									// (with type uint8_t*) once the audio path
									// is responsible for freeing the chunk
									// buffer, too.
			size_t				fChunkBufferSize;
			bool				fAudioDecodeError;

			AVFrame*			fDecodedDataBuffer;
			int32				fDecodedDataBufferOffset;
			int32				fDecodedDataBufferSize;

			AVPacket			fTempPacket;
};

#endif // AVCODEC_DECODER_H

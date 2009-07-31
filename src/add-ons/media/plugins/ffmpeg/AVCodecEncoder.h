/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef AVCODEC_ENCODER_H
#define AVCODEC_ENCODER_H


#include <MediaFormats.h>

extern "C" {
	#include "avcodec.h"
}

#include "EncoderPlugin.h"


class AVCodecEncoder : public Encoder {
public:
								AVCodecEncoder(uint32 codecID);

	virtual						~AVCodecEncoder();

	virtual	status_t			AcceptedFormat(
									const media_format* proposedInputFormat,
									media_format* _acceptedInputFormat = NULL);

	virtual	status_t			SetUp(const media_format* inputFormat);

	virtual status_t			GetEncodeParameters(
									encode_parameters* parameters) const;
	virtual status_t			SetEncodeParameters(
									encode_parameters* parameters) const;

	virtual status_t			Encode(const void* buffer, int64 frameCount,
									media_encode_info* info);

private:
			status_t			_EncodeAudio(const void* buffer,
									int64 frameCount,
									media_encode_info* info);

			status_t			_EncodeVideo(const void* buffer,
									int64 frameCount,
									media_encode_info* info);

private:
			media_format		fInputFormat;

			// FFmpeg related members
			// TODO: Refactor common base class from AVCodec[De|En]Coder!
			AVCodec*			fCodec;
			AVCodecContext*		fContext;
			AVFrame*			fInputPicture;
//			AVFrame*			fOutputPicture;

			uint32				fAVCodecID;

			bool				fCodecInitDone;

			uint8*				fChunkBuffer;
};

#endif // AVCODEC_ENCODER_H

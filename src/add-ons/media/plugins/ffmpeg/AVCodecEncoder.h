/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef AVCODEC_ENCODER_H
#define AVCODEC_ENCODER_H


#include <MediaFormats.h>

#include "EncoderPlugin.h"


class AVCodecEncoder : public Encoder {
public:
								AVCodecEncoder(const char* shortName);

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
};

#endif // AVCODEC_ENCODER_H

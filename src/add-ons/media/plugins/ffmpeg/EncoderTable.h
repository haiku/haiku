/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ENCODER_TABLE_H
#define ENCODER_TABLE_H


#include <MediaFormats.h>

extern "C" {
	#include "avcodec.h"
}


struct EncoderDescription {
	media_codec_info		codec_info;
	media_format_family		format_family;
	media_type				input_type;
	media_type				output_type;
	int						bit_rate_scale;
};


extern const EncoderDescription gEncoderTable[];
extern const size_t gEncoderCount;

CodecID raw_audio_codec_id_for(const media_format& format);


#endif // ENCODER_TABLE_H

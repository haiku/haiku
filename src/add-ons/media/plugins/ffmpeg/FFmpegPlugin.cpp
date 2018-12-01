/*
 * Copyright (C) 2001 Carlos Hasan
 * Copyright (C) 2001 François Revol
 * Copyright (C) 2001 Axel Dörfler
 * Copyright (C) 2004 Marcus Overhagen
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2018 Dario Casalinuovo
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//! libavcodec/libavformat based Decoder and Reader plugin for Haiku

#include "FFmpegPlugin.h"

#include <stdio.h>

#include <new>

extern "C" {
	#include "avcodec.h"
	#include "avformat.h"
}

#include "AVCodecDecoder.h"
#include "AVCodecEncoder.h"
#include "AVFormatReader.h"
#include "AVFormatWriter.h"
#include "CodecTable.h"
#include "EncoderTable.h"
#include "MuxerTable.h"


//#define TRACE_FFMPEG_PLUGIN
#ifdef TRACE_FFMPEG_PLUGIN
#	define TRACE printf
#else
#	define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


B_DECLARE_CODEC_KIT_PLUGIN(
	FFmpegPlugin,
	"ffmpeg",
	B_CODEC_KIT_PLUGIN_VERSION
);


// #pragma mark -


BDecoder*
FFmpegPlugin::NewDecoder(uint index)
{
// TODO: Confirm we can check index here.
//	if (index == 0)
		return new(std::nothrow) AVCodecDecoder();
//	return NULL;
}


BReader*
FFmpegPlugin::NewReader()
{
	return new(std::nothrow) AVFormatReader();
}


status_t
FFmpegPlugin::GetSupportedFormats(media_format** _formats, size_t* _count)
{
	return build_decoder_formats(_formats, _count);
}


BWriter*
FFmpegPlugin::NewWriter()
{
	return new(std::nothrow) AVFormatWriter();
}


status_t
FFmpegPlugin::GetSupportedFileFormats(const media_file_format** _fileFormats,
	size_t* _count)
{
	*_fileFormats = gMuxerTable;
	*_count = gMuxerCount;
	return B_OK;
}


BEncoder*
FFmpegPlugin::NewEncoder(const media_codec_info& codecInfo)
{
	for (size_t i = 0; i < gEncoderCount; i++) {
		if (codecInfo.sub_id == gEncoderTable[i].codec_info.sub_id) {
			return new(std::nothrow)AVCodecEncoder(codecInfo.sub_id,
				gEncoderTable[i].bit_rate_scale);
		}
	}
	return NULL;
}


BEncoder*
FFmpegPlugin::NewEncoder(const media_format& format)
{
	for (size_t i = 0; i < gEncoderCount; i++) {
		if (format.type == gEncoderTable[i].output_type) {
			return new(std::nothrow)AVCodecEncoder(
				gEncoderTable[i].codec_info.sub_id,
				gEncoderTable[i].bit_rate_scale);
		}
	}
	return NULL;
}


status_t
FFmpegPlugin::RegisterNextEncoder(int32* cookie, media_codec_info* _codecInfo,
	media_format_family* _formatFamily, media_format* _inputFormat,
	media_format* _outputFormat)
{
	if (*cookie < 0 || *cookie >= (int32)gEncoderCount)
		return B_BAD_INDEX;

	*_codecInfo = gEncoderTable[*cookie].codec_info;
	*_formatFamily = gEncoderTable[*cookie].format_family;
	_inputFormat->type = gEncoderTable[*cookie].input_type;
	_outputFormat->type = gEncoderTable[*cookie].output_type;;

	*cookie = *cookie + 1;

	return B_OK;
}

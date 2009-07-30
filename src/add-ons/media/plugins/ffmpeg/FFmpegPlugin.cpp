/*
 * Copyright (C) 2001 Carlos Hasan
 * Copyright (C) 2001 François Revol
 * Copyright (C) 2001 Axel Dörfler
 * Copyright (C) 2004 Marcus Overhagen
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//! libavcodec/libavformat based Decoder and Reader plugin for Haiku

#include "FFmpegPlugin.h"

#include <stdio.h>

#include <new>

extern "C" {
	#include "avformat.h"
}

#include "AVCodecDecoder.h"
#include "AVFormatReader.h"
#include "AVFormatWriter.h"
#include "CodecTable.h"
#include "MuxerTable.h"


FFmpegPlugin::GlobalInitilizer::GlobalInitilizer()
{
	av_register_all();
		// This will also call av_codec_init() by registering codecs.
}


FFmpegPlugin::GlobalInitilizer::~GlobalInitilizer()
{
	// TODO: uninit anything here?
}


FFmpegPlugin::GlobalInitilizer FFmpegPlugin::sInitilizer;


// #pragma mark -


Decoder*
FFmpegPlugin::NewDecoder(uint index)
{
// TODO: Confirm we can check index here.
//	if (index == 0)
		return new(std::nothrow) AVCodecDecoder();
//	return NULL;
}


Reader*
FFmpegPlugin::NewReader()
{
	return new(std::nothrow) AVFormatReader();
}


status_t
FFmpegPlugin::GetSupportedFormats(media_format** _formats, size_t* _count)
{
	BMediaFormats mediaFormats;
	if (mediaFormats.InitCheck() != B_OK)
		return B_ERROR;

	for (int i = 0; i < gCodecCount; i++) {
		media_format_description description;
		description.family = gCodecTable[i].family;
		switch(description.family) {
			case B_WAV_FORMAT_FAMILY:
				description.u.wav.codec = gCodecTable[i].fourcc;
				break;
			case B_AIFF_FORMAT_FAMILY:
				description.u.aiff.codec = gCodecTable[i].fourcc;
				break;
			case B_AVI_FORMAT_FAMILY:
				description.u.avi.codec = gCodecTable[i].fourcc;
				break;
			case B_MPEG_FORMAT_FAMILY:
				description.u.mpeg.id = gCodecTable[i].fourcc;
				break;
			case B_QUICKTIME_FORMAT_FAMILY:
				description.u.quicktime.codec = gCodecTable[i].fourcc;
				break;
			case B_MISC_FORMAT_FAMILY:
				description.u.misc.file_format =
					(uint32)(gCodecTable[i].fourcc >> 32);
				description.u.misc.codec = (uint32) gCodecTable[i].fourcc;
				break;
			default:
				break;
		}
		media_format format;
		format.type = gCodecTable[i].type;
		format.require_flags = 0;
		format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
		if (mediaFormats.MakeFormatFor(&description, 1, &format) != B_OK)
			return B_ERROR;
		gAVCodecFormats[i] = format;
	}

	*_formats = gAVCodecFormats;
	*_count = gCodecCount;
	return B_OK;
}


Writer*
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


MediaPlugin*
instantiate_plugin()
{
	return new(std::nothrow) FFmpegPlugin;
}


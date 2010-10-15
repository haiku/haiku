/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MuxerTable.h"


const media_file_format gMuxerTable[] = {
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_AVI_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-msvideo",
		"AVI (Audio Video Interleaved)",
		"avi",
		"avi",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-msvideo",
		"DV video format",
		"dv",
		"dv",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_AVI_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-matroska",
		"Matroska file format",
		"mkv",
		"mkv",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MPEG_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/mpeg",
		"MPEG (Motion Picture Experts Group)",
		"mpg",
		"mpg",
		{ 0 }
	},
// TODO: This one rejects unknown codecs. We probably need to define
// a media_format_family for it so that Encoders can announce their support
// for it specifically.
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"application/ogg",
		"Ogg (Xiph.Org Foundation)",
		"ogg",
		"ogg",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_WAV_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-wav",
		"WAV Format",
		"wav",
		"wav",
		{ 0 }
	},
};

const size_t gMuxerCount = sizeof(gMuxerTable) / sizeof(media_file_format);


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
};

const size_t gMuxerCount = sizeof(gMuxerTable) / sizeof(media_file_format);


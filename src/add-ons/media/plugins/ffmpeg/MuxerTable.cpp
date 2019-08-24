/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MuxerTable.h"


const media_file_format gMuxerTable[] = {
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/3gpp",
		"3GPP video",
		"3gp",
		"3gp",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_WAV_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/ac3",
		"AC3",
		"ac3",
		"ac3",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO,
		{ 0 },
		B_AIFF_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-aiff",
		"Audio IFF",
		"aiff",
		"aiff",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_AVI_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-asf",
		"ASF Movie",
		"asf",
		"asf",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE | media_file_format::B_READABLE
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
		media_file_format::B_WRITABLE | media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/dv",
		"DV Movie",
		"dv",
		"dv",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_WAV_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-flac",
		"Free Lossless Audio",
		"flac",
		"flac",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-flv",
		"Flash video",
		"flv",
		"flv",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE | media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_ANY_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-matroska",
		"Matroska movie",
		"mkv",
		"mkv",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-mov",
		"Quicktime movie",
		"mov",
		"mov",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MPEG_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/mpeg",
		"MPEG Layer 3",
		"mp3",
		"mp3",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/mp4",
		"MPEG (Motion Picture Experts Group) format 4",
		"mp4",
		"mp4",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		50,
		{ 0 },
		"audio/mp4",
		"AAC in MPEG4 container",
		"aac",
		"aac",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE | media_file_format::B_READABLE
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
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_AVI_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/mpeg",
		"MPEG TS",
		"mpegts",
		"mpegts",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MPEG_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/mpeg",
		"MPEG",
		"mpeg",
		"mpeg",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/nsv",
		"NSV (NullSoft Video file)",
		"nsv",
		"nsv",
		{ 0 }
	},
// TODO: This one rejects unknown codecs. We probably need to define
// a media_format_family for it so that Encoders can announce their support
// for it specifically.
	{
		media_file_format::B_WRITABLE | media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/ogg",
		"Ogg Audio (Xiph.Org Foundation)",
		"ogg",
		"ogg",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE | media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		50,
		{ 0 },
		"video/ogg",
		"Ogg Video (Xiph.Org Foundation)",
		"ogv",
		"ogv",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_AVI_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/vnd.rn-realvideo",
		"RM (RealVideo clip)",
		"rm",
		"rm",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_VIDEO
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_QUICKTIME_FORMAT_FAMILY,
		100,
		{ 0 },
		"application/x-shockwave-flash",
		"Shockwave video",
		"swf",
		"swf",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MPEG_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/x-vob",
		"VOB movie",
		"vob",
		"vob",
		{ 0 }
	},
	{
		media_file_format::B_WRITABLE
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_WAV_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/wav",
		"WAV Format",
		"wav",
		"wav",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"video/webm",
		"WebM movie",
		"webm",
		"webm",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_ENCODED_VIDEO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		50,
		{ 0 },
		"audio/webm",
		"WebM audio",
		"webm",
		"webm",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/xm",
		"Fast Tracker eXtended Module",
		"xm",
		"xm",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/s3m",
		"Scream Tracker 3",
		"s3m",
		"s3m",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/it",
		"Impulse Tracker",
		"it",
		"it",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-mod",
		"Protracker MOD",
		"mod",
		"mod",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-mptm",
		"OpenMPT Module",
		"mptm",
		"mptm",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-mo3",
		"Compressed Tracker audio",
		"mo3",
		"mo3",
		{ 0 }
	},
	{
		media_file_format::B_READABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO,
		{ 0 },
		B_MISC_FORMAT_FAMILY,
		100,
		{ 0 },
		"audio/x-med",
		"Amiga MED/OctaMED Tracker Module",
		"med",
		"med",
		{ 0 }
	},
};

const size_t gMuxerCount = sizeof(gMuxerTable) / sizeof(media_file_format);


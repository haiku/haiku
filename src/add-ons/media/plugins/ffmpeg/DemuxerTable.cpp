/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DemuxerTable.h"

extern "C" {
	#include "avformat.h"
}


// NOTE: AVFormatReader will refuse any streams which do not match to any
// of these formats from the table. It could very well be that it could play
// these streams, but testing has to be done first.


static const DemuxerFormat gDemuxerTable[] = {
//	{
//		// Tested with a limited number of streams. Some videos show bad
//		// artifacts on keyframes with our own ASF Reader, while they play
//		// fine with this Reader. But seeking seems to be a problem.
//		"asf", "ASF Movie", "video/x-asf",
//		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
//	},
//	{
//		// Tested with many streams and either works ok, or has inconsistent
//		// seeking results for audio, i.e. often audio is not in sync.
//		// Keep using the avi_reader implementation by Marcus Overhagen.
//		"avi", "AVI (Audio Video Interleaved)", "video/x-msvideo",
//		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
//	},
	{
		"dv", "DV Movie", "video/dv",
		B_WAV_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
	},
	{
		"flac", "FLAC", "audio/x-flac",
		B_WAV_FORMAT_FAMILY, B_ANY_FORMAT_FAMILY
	},
	{
		"flv", "Flash Video", "video/x-flv",
		B_QUICKTIME_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
	},
	{
		// Tested and appears to work ok with regards to video,
		// audio seems a problem.
		"matroska", "Matroska Movie", "video/x-matroska",
		B_MISC_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
	},
//	{
//		// Plays the limited amount of files I could test with.
//		"mov", "Quicktime Movie", "video/x-mov",
//		B_QUICKTIME_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
//	},
	{
		// Plays all files I could test with perfectly.
		"mp4", "MPEG-4 Movie", "video/x-mp4",
		B_QUICKTIME_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
	},
	{
 		// Works very well with all files I tested.
		"mp3", "MPEG-3", "audio/mpg",
		B_MPEG_FORMAT_FAMILY, B_MPEG_FORMAT_FAMILY
	},
	{
		// NOTE: Tested with a couple of files and only audio works ok.
		// On some files, the duration and time_base is detected incorrectly
		// by libavformat and those streams don't play at all.
		"mpg", "MPEG", "video/mpeg",
		B_MPEG_FORMAT_FAMILY, B_MPEG_FORMAT_FAMILY
	},
	{
		// NOTE: keep this before "mpeg" so it detects "mpegts" first.
		"mpegts", "MPEG TS", "video/mpeg",
		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
	},
	{
		// TODO: Also covers "mpegvideo", plus see mpegts.
		"mpeg", "MPEG", "video/mpeg",
		B_MPEG_FORMAT_FAMILY, B_MPEG_FORMAT_FAMILY
	},
	{
		// TODO: untested!
		"nsv", "NSV (NullSoft Video File)", "video/nsv",
		B_QUICKTIME_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
	},
	{
		// TODO: untested!
		"rm", "RM (RealVideo Clip)", "video/vnd.rn-realvideo",
		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
	},
	{
		// TODO: untested!
		"vob", "VOB Movie", "video/x-vob",
		B_MPEG_FORMAT_FAMILY, B_MPEG_FORMAT_FAMILY
	},
	{
		"ac3", "AC3", "audio/ac3",
		B_WAV_FORMAT_FAMILY, B_ANY_FORMAT_FAMILY
	},
};


const DemuxerFormat*
demuxer_format_for(AVInputFormat* format)
{
	int32 demuxerFormatCount = sizeof(gDemuxerTable) / sizeof(DemuxerFormat);
	for (int32 i = 0; i < demuxerFormatCount; i++) {
		const DemuxerFormat* demuxerFormat = &gDemuxerTable[i];
		if (strstr(format->name, demuxerFormat->demuxer_name) != NULL)
			return demuxerFormat;
	}
	return NULL;
}


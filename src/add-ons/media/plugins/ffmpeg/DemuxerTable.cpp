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
//		// TODO: untested!
//		"asf", "ASF Movie", "video/x-asf",
//		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
//	},
//	{
//		// Tested with a limited amount of streams and works ok, keep using
//		// the avi_reader implementation by Marcus Overhagen.
//		"avi", "AVI (Audio Video Interleaved)", "video/x-msvideo",
//		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
//	},
//	{
//		TODO: untested!
//		"mov", "MOV (Quicktime Movie)", "video/x-mov",
//		B_QUICKTIME_FORMAT_FAMILY, B_QUICKTIME_FORMAT_FAMILY
//	},
//	{
// 		// TODO: Broken because of buggy FindKeyFrame() or Seek() support.
//		"mp3", "MPEG (Motion Picture Experts Group)", "audio/mpg",
//		B_MPEG_FORMAT_FAMILY, B_MPEG_FORMAT_FAMILY
//	},
	{
		// NOTE: Tested with a couple of files and only audio works ok.
		// On some files, the duration and time_base is detected incorrectly
		// by libavformat and those streams don't play at all.
		"mpg", "MPEG (Motion Picture Experts Group)", "video/mpeg",
		B_MPEG_FORMAT_FAMILY, B_MPEG_FORMAT_FAMILY
	},
	{
		// NOTE: keep this before "mpeg" so it detects "mpegts" first.
		"mpegts", "MPEG (Motion Picture Experts Group)", "video/mpeg",
		B_WAV_FORMAT_FAMILY, B_AVI_FORMAT_FAMILY
	},
	{
		// TODO: Also covers "mpegvideo", plus see above.
		"mpeg", "MPEG (Motion Picture Experts Group)", "video/mpeg",
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


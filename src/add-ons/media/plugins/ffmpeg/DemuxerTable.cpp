/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DemuxerTable.h"
#include "MuxerTable.h"

extern "C" {
	#include "avformat.h"
}


// NOTE: AVFormatReader uses this table only for better pretty names and
// the MIME type info, the latter which is unavailable from AVInputFormat.


const media_file_format*
demuxer_format_for(AVInputFormat* format)
{
	for (uint32 i = 0; i < gMuxerCount; i++) {
		const media_file_format* demuxerFormat = &gMuxerTable[i];

		if (!(demuxerFormat->capabilities & media_file_format::B_READABLE))
			continue;

		if (strstr(format->name, demuxerFormat->short_name) != NULL)
			return demuxerFormat;
	}
	return NULL;
}


/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DEMUXER_TABLE_H
#define DEMUXER_TABLE_H


#include <MediaDefs.h>


struct DemuxerFormat {
	const char*				demuxer_name;
	const char*				pretty_name;
	const char*				mime_type;
	media_format_family		audio_family;
	media_format_family		video_family;
};


struct AVInputFormat;

const DemuxerFormat* demuxer_format_for(AVInputFormat* format);


#endif // DEMUXER_TABLE_H

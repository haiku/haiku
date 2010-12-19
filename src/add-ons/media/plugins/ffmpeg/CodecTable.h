/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CODEC_TABLE_H
#define CODEC_TABLE_H


#include <MediaFormats.h>

struct AVInputFamily {
	media_format_family family;
	const char *avname;
};
extern struct AVInputFamily gAVInputFamilies[];

status_t build_decoder_formats(media_format** _formats, size_t* _count);


#endif // CODEC_TABLE_H

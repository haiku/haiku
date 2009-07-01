/*
 * Copyright (C) 2001 Carlos Hasan. All Rights Reserved.
 * Copyright (C) 2001 François Revol. All Rights Reserved.
 * Copyright (C) 2001 Axel Dörfler. All Rights Reserved.
 * Copyright (C) 2004 Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//! libavcodec based decoder for Haiku

#ifndef CODEC_TABLE_H
#define CODEC_TABLE_H

#include <MediaDefs.h>
#include <MediaFormats.h>

#include "gfx_util.h"


struct codec_table {
	CodecID					id;
	media_type				type;
	media_format_family		family;
	uint64					fourcc;
	const char*				prettyname;
};

extern const struct codec_table gCodecTable[];
extern const int gCodecCount;
extern media_format gAVCodecFormats[];


#endif // CODEC_TABLE_H

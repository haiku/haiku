/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EncoderTable.h"


const media_codec_info gEncoderTable[] = {
	{
		"MPEG2 Video",
		"mpeg2video",
		0,
		0,
		{ 0 }
	},
	{
		"WAV",
		"wav",
		0,
		0,
		{ 0 }
	},
};

const size_t gEncoderCount = sizeof(gEncoderTable) / sizeof(media_codec_info);


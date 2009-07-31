/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EncoderTable.h"


const EncoderDescription gEncoderTable[] = {
	{
		{
			"MPEG2 Video",
			"mpeg2video",
			0,
			0,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO
	},
	{
		{
			"WAV",
			"wav",
			0,
			0,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO
	}
};

const size_t gEncoderCount = sizeof(gEncoderTable) / sizeof(EncoderDescription);


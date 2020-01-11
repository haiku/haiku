/*
 *  Copyright 2020, Haiku, Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License.
 */
#ifndef _GAME_SOUND_DEFS_H
#define _GAME_SOUND_DEFS_H


#include <SupportDefs.h>


typedef int32 gs_id;

#define B_GS_CUR_API_VERSION B_BEOS_VERSION
#define B_GS_MIN_API_VERSION 0x100
#define B_GS_INVALID_SOUND ((gs_id)-1)
#define B_GS_MAIN_SOUND ((gs_id)-2)


enum {
	B_GS_BAD_HANDLE = -99999,
	B_GS_NO_SOUNDS,
	B_GS_NO_HARDWARE,
	B_GS_ALREADY_COMMITTED,
	B_GS_READ_ONLY_VALUE
};


struct gs_audio_format {
	enum format {
		B_GS_U8 = 0x11,
		B_GS_S16 = 0x2,
		B_GS_F = 0x24,
		B_GS_S32 = 0x4
	};
	float	frame_rate;
	uint32	channel_count;
	uint32	format;
	uint32	byte_order;
	size_t	buffer_size;
};


enum gs_attributes {
	B_GS_NO_ATTRIBUTE = 0,
	B_GS_MAIN_GAIN = 1,
	B_GS_CD_THROUGH_GAIN,
	B_GS_GAIN = 128,
	B_GS_PAN,
	B_GS_SAMPLING_RATE,
	B_GS_LOOPING,
	B_GS_FIRST_PRIVATE_ATTRIBUTE = 90000,
	B_GS_FIRST_USER_ATTRIBUTE = 100000
};


struct gs_attribute {
	int32		attribute;
	bigtime_t	duration;
	float		value;
	uint32		flags;
};


struct gs_attribute_info {
	int32	attribute;
	float	granularity;
	float	minimum;
	float	maximum;
};


#endif

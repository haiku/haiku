/*****************************************************************************/
// GameSoundDefs.h
//
// "Definitions of common types and constants for the gamesound kit."
//
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef GAME_KIT_GAME_SOUND_DEFS
#define GAME_KIT_GAME_SOUND_DEFS

#include <SupportDefs.h>

#define B_GS_CUR_API_VERSION B_BEOS_VERSION
#define B_GS_MIN_API_VERSION 0x100

typedef int32 gs_id;

// Invalid sound handle.
#define B_GS_INVALID_SOUND ((gs_id)-1)

// gs_id for the main mix buffer.
#define B_GS_MAIN_SOUND ((gs_id)-2)

// FIXME - renumber to real error messages.
enum
{
	B_GS_BAD_HANDLE = -99999,
	B_GS_NO_SOUNDS,
	B_GS_NO_HARDWARE,
	B_GS_ALREADY_COMMITTED,
	B_GS_READ_ONLY_VALUE
};

struct gs_audio_format
{
	// Same as media_raw_audio_format.
	enum format 
	{
		B_GS_U8 = 0x11,		// 128 == mid, 1 == bottom, 255 == top.
		B_GS_S16 = 0x2,		// 0 == mid, -32767 == bottom, +32767 == top.
		B_GS_F = 0x24,		// 0 == mid, -1.0 == bottom, 1.0 == top.
		B_GS_S32 = 0x4		// 0 == mid, 0x80000001 == bottom, 0x7fffffff == top.
	};
	
	float  frame_rate;
	uint32 channel_count;	// 1 or 2, mostly.
	uint32 format;			// For compressed formats, go to media_encoded_audio_format.
	uint32 byte_order;		// 2 for little endian, 1 for big endian.
	size_t buffer_size;		// Size of each buffer -- NOT GUARANTEED.
};


enum gs_attributes
{
	B_GS_NO_ATTRIBUTE = 0,	//	When there is no attribute.
	B_GS_MAIN_GAIN = 1,		//	0 == 0 dB, -6.0 == -6 dB (gs_id ignored).
	B_GS_CD_THROUGH_GAIN,	//	0 == 0 dB, -12.0 == -12 dB (gs_id ignored).
							//	But which CD?
	B_GS_GAIN = 128,		//	0 == 0 dB, -1.0 == -1 dB, +10.0 == +10 dB.
	B_GS_PAN,				//	0 == middle, -1.0 == left, +1.0 == right.
	B_GS_SAMPLING_RATE,		//	44100.0 == 44.1 kHz.
	B_GS_LOOPING,			//	0 == no.
	B_GS_FIRST_PRIVATE_ATTRIBUTE = 90000,
	B_GS_FIRST_USER_ATTRIBUTE = 100000
};

struct gs_attribute
{
	int32		attribute; 	// Which attribute.
	bigtime_t	duration;	// How long of time to ramp over for the change.
	float		value;		// Where the value stops changing.
	uint32		flags;		// Whatever flags are for the attribute.
};

struct gs_attribute_info
{
	int32	attribute;
	float	granularity;
	float	minimum;
	float	maximum;
};

#endif // GAME_KIT_GAME_SOUND_DEFS

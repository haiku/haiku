#ifndef _RAW_FORMATS_H
#define _RAW_FORMATS_H

#include <MediaDefs.h>

// The raw audio format types defined here are only to be used by
// the media kit codec API, they are not supported for application
// use. Only the raw decoder does understand them, they are created
// by the file readers, like WAV or AIFF reader

enum {
	B_AUDIO_FORMAT_UINT8	= 0x00100,
	B_AUDIO_FORMAT_INT8		= 0x00200,
	B_AUDIO_FORMAT_INT16	= 0x00400,
	B_AUDIO_FORMAT_INT24	= 0x00800,
	B_AUDIO_FORMAT_INT32	= 0x01000,
	B_AUDIO_FORMAT_FLOAT32	= 0x02000,
	B_AUDIO_FORMAT_FLOAT64	= 0x04000,
	B_AUDIO_FORMAT_MASK		= 0xfffff,
	B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE	= 0x100000,
	B_AUDIO_FORMAT_CHANNEL_ORDER_AIFF	= 0x200000,
};

// FYI: A few channel orders for 6 channel audio...
// DTS  channel order : C, FL, FR, SL, SR, LFE 
// AAC  channel order : C, FL, FR, SL, SR, LFE 
// AC3  channel order : FL, C, FR, SL, SR, LFE 
// wav  channel order : FL, FR, C, LFE, SL, SR
// aiff channel order : FL, SL, C, FR, SR, LFE 

#endif // _RAW_FORMATS_H

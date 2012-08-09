/*******************************************************************************
/
/	File:			SoundUtils.h
/
/   Description:	Utility functions for handling audio data.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if ! defined( _SoundUtils_h )
#define _SoundUtils_h

#include <MediaDefs.h>

//	Simple helper functions that come in handy when doing
//	buffer calculations.
double us_to_s(bigtime_t usecs);
bigtime_t s_to_us(double secs);

int bytes_per_frame(const media_raw_audio_format & format);
int frames_per_buffer(const media_raw_audio_format & format);
bigtime_t buffer_duration(const media_raw_audio_format & format);
bigtime_t frames_duration(const media_raw_audio_format & format,
	int64 num_frames);
int64 frames_for_duration(const media_raw_audio_format & format,
	bigtime_t duration);
int buffers_for_duration(const media_raw_audio_format & format,
	bigtime_t duration);

//	This is a common hook function interface for
//	SoundConsumer and SoundProducer to use.
typedef void (*SoundProcessFunc)(void * cookie,
	bigtime_t timestamp, void * data, size_t datasize,
	const media_raw_audio_format & format);
typedef void (*SoundNotifyFunc)(void * cookie,
	int32 code, ...);

//	These are special codes that we use in the Notify
//	function hook.
enum {
	B_WILL_START = 1,		//	performance_time
	B_WILL_STOP,			//	performance_time immediate
	B_WILL_SEEK,			//	performance_time media_time
	B_WILL_TIMEWARP,		//	real_time performance_time
	B_CONNECTED,			//	name (char*)
	B_DISCONNECTED,			//
	B_FORMAT_CHANGED,		//	media_raw_audio_format*
	B_NODE_DIES,			//	node will die!
	B_HOOKS_CHANGED,		//	
	B_OP_TIMED_OUT,			//	timeout that expired -- Consumer only
	B_PRODUCER_DATA_STATUS,	//	status performance_time -- Consumer only
	B_LATE_NOTICE			//	how_much performance_time -- Producer only
};

#endif /* _SoundUtils_h */

/*
 * Copyright 1998-1999, Be Incorporated.
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*******************************************************************************
/
/	File:			SoundUtils.h
/
/   Description:	Utility functions for handling audio data.
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

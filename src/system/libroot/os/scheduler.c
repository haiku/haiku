/*
 * Copyright 2004-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@gmail.com
 *		Michael Pfeiffer, laplace@users.sourceforge.net
 */


#include <scheduler.h>

#include <syscalls.h>


static struct {
	uint32 what;
	int32 priority;
} sWhatPriorityArray[] = {
	// highest priority first
	{B_MIDI_PROCESSING, 0x78},
	{B_AUDIO_RECORDING | B_AUDIO_PLAYBACK, 0x73},
	{B_LIVE_AUDIO_MANIPULATION, 0x6e},
	{B_VIDEO_RECORDING, 0x19},
	{B_VIDEO_PLAYBACK, 0x14},
	{B_USER_INPUT_HANDLING, 0x0f},
	{B_LIVE_VIDEO_MANIPULATION, 0x0e},
	{B_LIVE_3D_RENDERING, 0x0c},
	{B_STATUS_RENDERING, 0xa},
	{B_OFFLINE_PROCESSING, 0x06},
	{B_NUMBER_CRUNCHING, 0x05},
	{(uint32)-1, -1}
};

status_t __set_scheduler_mode(int32 mode);
int32 __get_scheduler_mode(void);


int32
suggest_thread_priority(uint32 what, int32 period, bigtime_t jitter,
	bigtime_t length)
{
	int i;
	int32 priority = what == B_DEFAULT_MEDIA_PRIORITY ? 0x0a : 0;
		// default priority

	// TODO: this needs kernel support, and is a pretty simplistic solution

	for (i = 0; sWhatPriorityArray[i].what != (uint32)-1; i ++) {
		if ((what & sWhatPriorityArray[i].what) != 0) {
			priority = sWhatPriorityArray[i].priority;
			break;
		}
	}

	return priority;
}


bigtime_t
estimate_max_scheduling_latency(thread_id thread)
{
	return _kern_estimate_max_scheduling_latency(thread);
}


status_t
__set_scheduler_mode(int32 mode)
{
	return _kern_set_scheduler_mode(mode);
}


int32
__get_scheduler_mode(void)
{
	return _kern_get_scheduler_mode();
}


B_DEFINE_WEAK_ALIAS(__set_scheduler_mode, set_scheduler_mode);
B_DEFINE_WEAK_ALIAS(__get_scheduler_mode, get_scheduler_mode);


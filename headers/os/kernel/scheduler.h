/*****************************************************************************

        File:           scheduler.h

		Description:    Scheduling inquiry functions

        Copyright 1998-, Be Incorporated, All Rights Reserved.

*****************************************************************************/


#if !defined(SCHEDULER_H)
#define SCHEDULER_H

#include <SupportDefs.h>
#include <OS.h>

/* To get a good thread priority, call suggest_thread_priority() with the following information: */
/* 'what' is a bit mask describing what you're doing in the thread. */
/* 'period' is how many times a second your thread needs to run (-1 if you're running continuously.) */
/* 'jitter' is an estimate (in us) of how much that period can vary, as long as it stays centered on the average. */
/* 'length' is how long (in us) you expect to run for each invocation. */
/* "invocation" means, typically, receiving a message, dispatching it, and then returning to reading a message. */
/* MANIPULATION means both filtering and compression/decompression. */
/* PLAYBACK and RECORDING means threads feeding/reading ACTUAL HARDWARE ONLY. */
/* 0 means don't care */
enum be_task_flags { /* bitmasks for "what" */
	B_DEFAULT_MEDIA_PRIORITY = 0,
	B_OFFLINE_PROCESSING = 0x1,
	B_STATUS_RENDERING = 0x2,		/* can also use this for "preview" type things */
	B_USER_INPUT_HANDLING = 0x4,
	B_LIVE_VIDEO_MANIPULATION = 0x8,	/* non-live processing is OFFLINE_PROCESSING */
	B_VIDEO_PLAYBACK = 0x10,	/* feeding hardware */
	B_VIDEO_RECORDING = 0x20,	/* grabbing from hardware */
	B_LIVE_AUDIO_MANIPULATION = 0x40,	/* non-live processing is OFFLINE_PROCESSING */
	B_AUDIO_PLAYBACK = 0x80,	/* feeding hardware */
	B_AUDIO_RECORDING = 0x100,	/* grabbing from hardware */
	B_LIVE_3D_RENDERING = 0x200,		/* non-live rendering is OFFLINE_PROCESSING */
	B_NUMBER_CRUNCHING = 0x400,
	B_MIDI_PROCESSING = 0x800
};
#if defined(__cplusplus)
extern "C" {
_IMPEXP_ROOT int32 suggest_thread_priority(uint32 task_flags = B_DEFAULT_MEDIA_PRIORITY, 
	int32 period = 0, bigtime_t jitter = 0, bigtime_t length = 0);
_IMPEXP_ROOT bigtime_t estimate_max_scheduling_latency(thread_id th = -1);	/* default is current thread */
}
#else
_IMPEXP_ROOT int32 suggest_thread_priority(uint32 what, int32 period, bigtime_t jitter, bigtime_t length);
_IMPEXP_ROOT bigtime_t estimate_max_scheduling_latency(thread_id th);	/* default is current thread */
#endif

#endif /* SCHEDULER_H */


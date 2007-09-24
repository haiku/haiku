/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NULL_AUDIO_DRIVER_H
#define NULL_AUDIO_DRIVER_H

#include <drivers/driver_settings.h>
#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include <hmulti_audio.h>
#include <string.h>
#include <stdlib.h>

#define FRAMES_PER_BUFFER 1024
#define MULTI_AUDIO_BASE_ID 1024
#define MULTI_AUDIO_DEV_PATH "audio/hmulti"
#define MULTI_AUDIO_MASTER_ID 0
#define STRMINBUF 2
#define STRMAXBUF 2

typedef struct {
	spinlock	lock;
	int bits;

	void*		buffers[STRMAXBUF];
	uint32		num_buffers;
	uint32		num_channels;
	uint32		format;
	uint32		rate;

	uint32		buffer_length;
	sem_id		buffer_ready_sem;
	uint32		frames_count;
	uint32		buffer_cycle;
	bigtime_t	real_time;

	area_id		buffer_area;
} device_stream_t;

typedef struct {
	device_stream_t playback_stream;
	device_stream_t record_stream;
	
	thread_id interrupt_thread;
	bool running;
} device_t;

extern device_hooks driver_hooks;
int32 format_to_sample_size(uint32 format);

status_t multi_audio_control(void* cookie, uint32 op, void* arg, size_t len);

status_t null_hw_create_virtual_buffers(device_stream_t* stream, const char* name);
status_t null_start_hardware(device_t* device);
void null_stop_hardware(device_t* device);

#endif /* NULL_AUDIO_DRIVER_H */


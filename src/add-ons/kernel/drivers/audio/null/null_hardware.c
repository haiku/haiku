/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bek, host.haiku@gmx.de
 */
#include "driver.h"


status_t
null_hw_create_virtual_buffers(device_stream_t* stream, const char* name)
{
	int i;
	int buffer_size;
	int area_size;
	uint8* buffer;

	buffer_size = stream->num_channels
				* format_to_sample_size(stream->format)
				* stream->buffer_length;
	buffer_size = (buffer_size + 127) & (~127);

	area_size = buffer_size * stream->num_buffers;
	area_size = (area_size + B_PAGE_SIZE - 1) & (~(B_PAGE_SIZE -1));

	stream->buffer_area = create_area("null_audio_buffers", (void**)&buffer,
							B_ANY_KERNEL_ADDRESS, area_size,
							B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (stream->buffer_area < B_OK)
		return stream->buffer_area;

	// Get the correct address for setting up the buffers
	// pointers being passed back to userland
	for (i = 0; i < stream->num_buffers; i++)
		stream->buffers[i] = buffer + (i * buffer_size);

	stream->buffer_ready_sem = create_sem(0, name);
	return B_OK;
}


static int32
null_fake_interrupt(void* cookie)
{
	// This thread is supposed to fake the interrupt
	// handling done in communication with the
	// hardware usually. What it does is nearly the
	// same like all soundrivers, get the interrupt
	// exchange the buffer pointer and update the
	// time information. Instead of exiting, we wait
	// until the next fake interrupt appears.
	bigtime_t sleepTime;
	device_t* device = (device_t*) cookie;
	int sampleRate;

	switch (device->playback_stream.rate) {
		case B_SR_48000:
			sampleRate = 48000;
			break;
		case B_SR_44100:
		default:
			sampleRate = 44100;
			break;
	}

	// The time between until we get a new valid buffer
	// from our soundcard: buffer_length / samplerate
	sleepTime = (device->playback_stream.buffer_length * 1000000LL) / sampleRate;

	while (device->running) {
		cpu_status status;
		status = disable_interrupts();
		acquire_spinlock(&device->playback_stream.lock);
		device->playback_stream.real_time = system_time();
		device->playback_stream.frames_count += device->playback_stream.buffer_length;
		device->playback_stream.buffer_cycle = (device->playback_stream.buffer_cycle +1) % device->playback_stream.num_buffers;
		release_spinlock(&device->playback_stream.lock);

		// TODO: Create a simple sinus wave, so that recording from
		// the virtual device actually returns something useful
		acquire_spinlock(&device->record_stream.lock);
		device->record_stream.real_time = device->playback_stream.real_time;
		device->record_stream.frames_count += device->record_stream.buffer_length;
		device->record_stream.buffer_cycle = (device->record_stream.buffer_cycle +1) % device->record_stream.num_buffers;
		release_spinlock(&device->record_stream.lock);

		restore_interrupts(status);

		release_sem_etc(device->playback_stream.buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
		release_sem_etc(device->record_stream.buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
		snooze(sleepTime);
	}
	return B_OK;
}


status_t
null_start_hardware(device_t* device)
{
	dprintf("null_audio: %s spawning fake interrupter\n", __func__);
	device->running = true;
	device->interrupt_thread = spawn_kernel_thread(null_fake_interrupt, "null_audio interrupter",
								B_REAL_TIME_PRIORITY, (void*)device);
	return resume_thread(device->interrupt_thread);
}


void
null_stop_hardware(device_t* device)
{
	device->running = false;
}


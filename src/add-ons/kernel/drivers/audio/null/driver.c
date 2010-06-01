/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bek, host.haiku@gmx.de
 */
#include "driver.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
device_t device;


status_t
init_hardware(void)
{
	dprintf("null_audio: %s\n", __func__);
	return B_OK;
}


status_t
init_driver(void)
{
	dprintf("null_audio: %s\n", __func__);
	device.running = false;
	return B_OK;
}


void
uninit_driver(void)
{
}


const char**
publish_devices(void)
{
	static const char* published_paths[] = {
		MULTI_AUDIO_DEV_PATH "/null/0",
		NULL
	};
	dprintf("null_audio: %s\n", __func__);

	return published_paths;
}


static status_t
null_audio_open (const char *name, uint32 flags, void** cookie)
{
	dprintf("null_audio: %s\n" , __func__ );
	*cookie = &device;
	return B_OK;
}


static status_t
null_audio_read (void* cookie, off_t a, void* b, size_t* num_bytes)
{
	dprintf("null_audio: %s\n" , __func__ );
	// Audio drivers are not supposed to return anything
	// inside here
	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t
null_audio_write (void* cookie, off_t a, const void* b, size_t* num_bytes)
{
	dprintf("null_audio: %s\n" , __func__ );
	// Audio drivers are not supposed to return anything
	// inside here
	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t
null_audio_control (void* cookie, uint32 op, void* arg, size_t len)
{
	//dprintf("null_audio: %s\n" , __func__ );
	// In case we have a valid cookie, initialized
	// the driver and hardware connection properly
	// Simply pass through to the multi audio hooks
	if (cookie)
		return multi_audio_control(cookie, op, arg, len);
	else
		dprintf("null_audio: %s called without cookie\n" , __func__);

	// Return error in case we have no valid setup
	return B_BAD_VALUE;
}


static status_t
null_audio_close (void* cookie)
{
	device_t* device = (device_t*) cookie;
	dprintf("null_audio: %s\n" , __func__ );
	if (device && device->running)
		null_stop_hardware(device);
	return B_OK;
}


static status_t
null_audio_free (void* cookie)
{
	dprintf("null_audio: %s\n" , __func__ );
	return B_OK;
}


device_hooks driver_hooks = {
	null_audio_open,
	null_audio_close,
	null_audio_free,
	null_audio_control,
	null_audio_read,
	null_audio_write
};


device_hooks*
find_device(const char* name)
{
	return &driver_hooks;
}


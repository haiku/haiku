/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */


#include "driver.h"


static status_t
hda_open(const char *name, uint32 flags, void** cookie)
{
	hda_controller* controller = NULL;

	for (uint32 i = 0; i < gNumCards; i++) {
		if (strcmp(gCards[i].devfs_path, name) == 0) {
			controller = &gCards[i];
			break;
		}
	}

	if (controller == NULL)
		return ENODEV;

	if (controller->opened)
		return B_BUSY;

	status_t status = hda_hw_init(controller);
	if (status != B_OK)
		return status;

	atomic_add(&controller->opened, 1);

	*cookie = controller;
	return B_OK;
}


static status_t
hda_read(void* cookie, off_t position, void *buf, size_t* numBytes)
{
	*numBytes = 0;
		/* tell caller nothing was read */
	return B_IO_ERROR;
}


static status_t
hda_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	*numBytes = 0;
		/* tell caller nothing was written */
	return B_IO_ERROR;
}


static status_t
hda_control(void* cookie, uint32 op, void* arg, size_t length)
{
	hda_controller* controller = (hda_controller*)cookie;
	if (controller->active_codec)
		return multi_audio_control(controller->active_codec, op, arg, length);

	return B_BAD_VALUE;
}


static status_t
hda_close(void* cookie)
{
	hda_controller* controller = (hda_controller*)cookie;
	hda_hw_stop(controller);
	atomic_add(&controller->opened, -1);

	return B_OK;
}


static status_t
hda_free(void* cookie)
{
	hda_controller* controller = (hda_controller*)cookie;
	hda_hw_uninit(controller);

	return B_OK;
}


device_hooks gDriverHooks = {
	hda_open, 		/* -> open entry point */
	hda_close, 		/* -> close entry point */
	hda_free,		/* -> free cookie */
	hda_control, 	/* -> control entry point */
	hda_read,		/* -> read entry point */
	hda_write		/* -> write entry point */
};

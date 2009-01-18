/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Jérôme Duval (korli@users.berlios.de)
 */


#include "driver.h"


static status_t
geode_open(const char *name, uint32 flags, void** cookie)
{
	geode_controller* controller = NULL;

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

	status_t status = geode_hw_init(controller);
	if (status != B_OK)
		return status;

	atomic_add(&controller->opened, 1);

	*cookie = controller;
	return B_OK;
}


static status_t
geode_read(void* cookie, off_t position, void *buf, size_t* numBytes)
{
	*numBytes = 0;
		/* tell caller nothing was read */
	return B_IO_ERROR;
}


static status_t
geode_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	*numBytes = 0;
		/* tell caller nothing was written */
	return B_IO_ERROR;
}


static status_t
geode_control(void* cookie, uint32 op, void* arg, size_t length)
{
	geode_controller* controller = (geode_controller*)cookie;
	return multi_audio_control(controller, op, arg, length);
}


static status_t
geode_close(void* cookie)
{
	geode_controller* controller = (geode_controller*)cookie;
	geode_hw_stop(controller);
	atomic_add(&controller->opened, -1);

	return B_OK;
}


static status_t
geode_free(void* cookie)
{
	geode_controller* controller = (geode_controller*)cookie;
	geode_hw_uninit(controller);

	return B_OK;
}


device_hooks gDriverHooks = {
	geode_open, 		/* -> open entry point */
	geode_close, 		/* -> close entry point */
	geode_free,		/* -> free cookie */
	geode_control, 	/* -> control entry point */
	geode_read,		/* -> read entry point */
	geode_write		/* -> write entry point */
};

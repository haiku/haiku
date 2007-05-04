/*
 * Copyright 2006, François Revol. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	fmap driver for Haiku

	Maps BEOS/IMAGE.BE files as virtual partitions.
*/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>

#define MAX_FMAPS 4
#define DEVNAME_FMT "disk/virtual/fmap/%2d"

int32 api_version = B_CUR_DRIVER_API_VERSION;

status_t
init_hardware (void)
{
	return B_OK;
}

status_t
init_driver (void)
{
	return B_OK;
}

void
uninit_driver (void)
{
}

static const char *fmap_name[MAX_FMAPS+1] = {
	NULL
};

device_hooks fmap_hooks = {
	NULL, /* open */
	NULL, /* close */
	NULL, /* free */
	NULL, /* control */
	NULL, /* read */
	NULL, /* write */
	NULL, NULL, NULL, NULL
};

const char**
publish_devices()
{
	return fmap_name;
}

device_hooks*
find_device(const char* name)
{
	return &fmap_hooks;
}

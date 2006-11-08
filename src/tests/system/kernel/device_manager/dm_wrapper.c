#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <drivers/device_manager.h>

#include "config_driver.h"
#include "dm_wrapper.h"

static int fd;

status_t init_dm_wrapper(void)
{
	static const char device[] = "/dev/" DM_DEVICE_NAME;

	fd = open(device, 0);
	if (fd < 0)
		return errno;
	return B_OK;
}

status_t uninit_dm_wrapper(void)
{
	close(fd);
	return B_OK;
}

status_t get_child(void)
{
	struct dm_ioctl_data params;
	
	params.magic = DM_GET_CHILD;
	params.attr = NULL;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_next_child(void)
{
	struct dm_ioctl_data params;

	params.magic = DM_GET_NEXT_CHILD;
	params.attr = NULL;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_parent(void)
{
	struct dm_ioctl_data params;

	params.magic = DM_GET_PARENT;
	params.attr = NULL;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t dm_get_next_attr(void)
{
	struct dm_ioctl_data params;

	params.magic = DM_GET_NEXT_ATTRIBUTE;
	params.attr = NULL;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t dm_retrieve_attr(struct dev_attr *attr)
{
	struct dm_ioctl_data params;

	params.magic = DM_RETRIEVE_ATTRIBUTE;
	params.attr = attr;

	return ioctl(fd, params.magic, &params, sizeof(params));
}


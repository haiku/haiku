/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Fast logging facilities.

	Devfs interface.

	Publishes a file where logging data can be read from
	directly via read. Once read, the logging data is freed.
	Obviously, you cannot seek in the file.
	
	Concurrent reads are permitted, but you will have fun
	merging read data.
*/


#include "fast_log_int.h"
#include <pnp_devfs.h>
#include <device_manager.h>


// Devfs node
pnp_node_handle fast_log_devfs_node;


static status_t
fast_log_devfs_open(void *device_cookie, uint32 flags, void **handle_cookie)
{
	handle_cookie = NULL;
	return B_OK;
}


static status_t
fast_log_devfs_close(void *cookie)
{
	return B_OK;
}


static status_t
fast_log_devfs_free(void *cookie)
{
	return B_OK;
}


static status_t
fast_log_devfs_control(void *cookie, uint32 op, void *data, size_t len)
{
	return B_ERROR;
}


static status_t
fast_log_devfs_read(void *cookie, off_t position, void *data, size_t *num_bytes)
{
	*num_bytes = fast_log_read_long(data, *num_bytes);
	return B_OK;
}


static status_t
fast_log_devfs_write(void *cookie, off_t position, const void *data, size_t *num_bytes)
{
	*num_bytes = 0;
	return B_ERROR;
}


//	#pragma mark -


/** create devfs entry */

static status_t
fast_log_create_devfs_entry(void)
{
	status_t res;
	pnp_node_attr attrs[] = {
		{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: FAST_LOG_DEVFS_MODULE_NAME }},
		{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: PNP_DEVFS_TYPE_NAME }},
		{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: PNP_DEVFS_MODULE_NAME }},
		{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "fast_log" }},

		{ PNP_DEVFS_FILENAME, B_STRING_TYPE, { string: FAST_LOG_DEVFS_NAME }},
		{ NULL }
	};

	res = pnp->register_device(NULL, attrs, NULL, &fast_log_devfs_node);
	if (res < 0)
		fast_log_devfs_node = NULL;

	return res;
}


static status_t
fast_log_devfs_init_device(pnp_node_handle node, void *user_cookie, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
fast_log_devfs_uninit_device(void *cookie)
{
	return B_OK;
}


static status_t
fast_log_devfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


// devfs interface
pnp_devfs_driver_info fast_log_devfs_module = {
	{
		{
			FAST_LOG_DEVFS_MODULE_NAME,
			0,
			fast_log_devfs_std_ops
		},

		fast_log_devfs_init_device,
		fast_log_devfs_uninit_device,

		NULL,
		NULL,
		NULL,
	},

	fast_log_devfs_open,
	fast_log_devfs_close,
	fast_log_devfs_free,
	fast_log_devfs_control,
	fast_log_devfs_read,
	fast_log_devfs_write
};


/** cleanup devfs interface */

void
fast_log_uninit_dev(void)
{
	if (fast_log_devfs_node != NULL)
		pnp->unregister_device(fast_log_devfs_node);
}


/** init devfs interface */

status_t
fast_log_init_dev(void)
{
	status_t res;

	fast_log_devfs_node = NULL;	

	if ((res = fast_log_create_devfs_entry()) < 0)
		return res;

	return B_OK;
}

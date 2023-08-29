/*
 * Copyright 2002-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@berlios.de
 *		Axel Dörfler, axeld@pinc-software.de
 *		David Reid
 */


#include <stdio.h>

#include <device_manager.h>
#include <Drivers.h>
#include <generic_syscall.h>
#include <kernel.h>
#include <malloc.h>
#include <random_defs.h>
#include <string.h>
#include <util/AutoLock.h>

#include "yarrow_rng.h"


//#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x...) dprintf("random: " x)
#else
#	define TRACE(x...) ;
#endif
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define	RANDOM_DRIVER_MODULE_NAME "bus_managers/random/driver_v1"
#define RANDOM_DEVICE_MODULE_NAME "bus_managers/random/device_v1"


static mutex sRandomLock;
static void *sRandomCookie;
device_manager_info* gDeviceManager;


typedef struct {
	device_node*			node;
} random_driver_info;


static status_t
random_queue_randomness(uint64 value)
{
	MutexLocker locker(&sRandomLock);
	RANDOM_ENQUEUE(value);
	return B_OK;
}


//	#pragma mark - device module API


static status_t
random_init_device(void* _info, void** _cookie)
{
	return B_OK;
}


static void
random_uninit_device(void* _cookie)
{
}


static status_t
random_open(void *deviceCookie, const char *name, int flags, void **cookie)
{
	TRACE("open(\"%s\")\n", name);
	return B_OK;
}


static status_t
random_read(void *cookie, off_t position, void *_buffer, size_t *_numBytes)
{
	TRACE("read(%lld,, %ld)\n", position, *_numBytes);

	MutexLocker locker(&sRandomLock);
	return RANDOM_READ(sRandomCookie, _buffer, _numBytes);
}


static status_t
random_write(void *cookie, off_t position, const void *buffer, size_t *_numBytes)
{
	TRACE("write(%lld,, %ld)\n", position, *_numBytes);
	MutexLocker locker(&sRandomLock);
	return RANDOM_WRITE(sRandomCookie, buffer, _numBytes);
}


static status_t
random_control(void *cookie, uint32 op, void *arg, size_t length)
{
	TRACE("ioctl(%ld)\n", op);
	return B_ERROR;
}


static status_t
random_generic_syscall(const char* subsystem, uint32 function, void* buffer,
	size_t bufferSize)
{
	switch (function) {
		case RANDOM_GET_ENTROPY:
		{
			random_get_entropy_args args;
			if (bufferSize != sizeof(args) || !IS_USER_ADDRESS(buffer))
				return B_BAD_VALUE;

			if (user_memcpy(&args, buffer, sizeof(args)) != B_OK)
				return B_BAD_ADDRESS;
			if (!IS_USER_ADDRESS(args.buffer))
				return B_BAD_ADDRESS;

			status_t result = random_read(NULL, 0, args.buffer, &args.length);
			if (result < 0)
				return result;

			return user_memcpy(buffer, &args, sizeof(args));
		}
	}
	return B_BAD_HANDLER;
}


static status_t
random_close(void *cookie)
{
	TRACE("close()\n");
	return B_OK;
}


static status_t
random_free(void *cookie)
{
	TRACE("free()\n");
	return B_OK;
}


static status_t
random_select(void *cookie, uint8 event, selectsync *sync)
{
	TRACE("select()\n");

	if (event == B_SELECT_READ) {
		/* tell there is already data to read */
		notify_select_event(sync, event);
	} else if (event == B_SELECT_WRITE) {
		/* we're now writable */
		notify_select_event(sync, event);
	}
	return B_OK;
}


static status_t
random_deselect(void *cookie, uint8 event, selectsync *sync)
{
	TRACE("deselect()\n");
	return B_OK;
}


//	#pragma mark - driver module API


static float
random_supports_device(device_node *parent)
{
	CALLED();
	const char *bus;

	// make sure parent is really device root
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "root"))
		return 0.0;

	return 1.0;
}


static status_t
random_register_device(device_node *node)
{
	CALLED();

	// ready to register
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "Random" }},
		{ B_DEVICE_FLAGS, B_UINT32_TYPE, { .ui32 = B_KEEP_DRIVER_LOADED }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, RANDOM_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
random_init_driver(device_node *node, void **cookie)
{
	CALLED();

	random_driver_info* info = (random_driver_info*)malloc(
		sizeof(random_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	mutex_init(&sRandomLock, "/dev/random lock");
	RANDOM_INIT();

	memset(info, 0, sizeof(*info));

	info->node = node;

	register_generic_syscall(RANDOM_SYSCALLS, random_generic_syscall, 1, 0);

	*cookie = info;
	return B_OK;
}


static void
random_uninit_driver(void *_cookie)
{
	CALLED();

	unregister_generic_syscall(RANDOM_SYSCALLS, 1);

	RANDOM_UNINIT();

	mutex_destroy(&sRandomLock);

	random_driver_info* info = (random_driver_info*)_cookie;
	free(info);
}


static status_t
random_register_child_devices(void* _cookie)
{
	CALLED();
	random_driver_info* info = (random_driver_info*)_cookie;
	status_t status = gDeviceManager->publish_device(info->node, "random",
		RANDOM_DEVICE_MODULE_NAME);
	if (status == B_OK) {
		gDeviceManager->publish_device(info->node, "urandom",
			RANDOM_DEVICE_MODULE_NAME);
	}
	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager},
	{}
};


struct device_module_info sRandomDevice = {
	{
		RANDOM_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	random_init_device,
	random_uninit_device,
	NULL, // remove,

	random_open,
	random_close,
	random_free,
	random_read,
	random_write,
	NULL,
	random_control,

	random_select,
	random_deselect,
};


struct driver_module_info sRandomDriver = {
	{
		RANDOM_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	random_supports_device,
	random_register_device,
	random_init_driver,
	random_uninit_driver,
	random_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


random_for_controller_interface sRandomForControllerModule = {
	{
		{
			RANDOM_FOR_CONTROLLER_MODULE_NAME,
			0,
			NULL
		},

		NULL, // supported devices
		NULL,
		NULL,
		NULL,
		NULL
	},

	random_queue_randomness,
};


module_info* modules[] = {
	(module_info*)&sRandomDriver,
	(module_info*)&sRandomDevice,
	(module_info*)&sRandomForControllerModule,
	NULL
};

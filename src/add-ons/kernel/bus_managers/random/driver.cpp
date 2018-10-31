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
static random_module_info *sRandomModule;
static void *sRandomCookie;
device_manager_info* gDeviceManager;


typedef struct {
	device_node*			node;
} random_driver_info;


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
	TRACE("read(%Ld,, %ld)\n", position, *_numBytes);

	MutexLocker locker(&sRandomLock);
	return sRandomModule->read(sRandomCookie, _buffer, _numBytes);
}


static status_t
random_write(void *cookie, off_t position, const void *buffer, size_t *_numBytes)
{
	TRACE("write(%Ld,, %ld)\n", position, *_numBytes);
	MutexLocker locker(&sRandomLock);
	if (sRandomModule->write == NULL) {
		*_numBytes = 0;
		return EINVAL;
	}
	return sRandomModule->write(sRandomCookie, buffer, _numBytes);
}


static status_t
random_control(void *cookie, uint32 op, void *arg, size_t length)
{
	TRACE("ioctl(%ld)\n", op);
	return B_ERROR;
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
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "Random" }},
		{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_KEEP_DRIVER_LOADED }},
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

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
random_uninit_driver(void *_cookie)
{
	CALLED();

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

	// add the default Yarrow RNG
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: "Yarrow RNG" }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: RANDOM_FOR_CONTROLLER_MODULE_NAME }},
		{ NULL }
	};

	device_node* node;
	return gDeviceManager->register_node(info->node,
		YARROW_RNG_SIM_MODULE_NAME, attrs, NULL, &node);
}


//	#pragma mark -


status_t
random_added_device(device_node *node)
{
	CALLED();

	status_t status = gDeviceManager->get_driver(node,
		(driver_module_info **)&sRandomModule, &sRandomCookie);

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
		random_added_device,
		NULL,
		NULL,
		NULL
	}
};


module_info* modules[] = {
	(module_info*)&sRandomDriver,
	(module_info*)&sRandomDevice,
	(module_info*)&sRandomForControllerModule,
	(module_info*)&gYarrowRandomModule,
	NULL
};

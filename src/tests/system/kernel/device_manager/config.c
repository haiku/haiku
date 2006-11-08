/* config driver
 * provides userland access to the device manager
 *
 * Copyright 2002-2004, Axel Doerfler. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors : Axel Doerfler, Jerome Duval
 */


#include <Drivers.h>
#include <drivers/device_manager.h>
#include <stdlib.h>
#include <string.h>

#include "config_driver.h"

#ifdef DEBUG
#define TRACE(x...) dprintf(x)
#else
#define TRACE(x...)
#endif

#define DEVICE_NAME "misc/config"

int32 api_version = B_CUR_DRIVER_API_VERSION;

device_manager_info *gDeviceManager;

typedef struct _driver_cookie {
	device_node_handle parent;
	device_node_handle child;
	device_attr_handle attr;
} driver_cookie;

//	Device interface


static status_t
config_open(const char *name, uint32 flags, void **_cookie)
{
	driver_cookie *cookie = malloc(sizeof(driver_cookie));
	if (cookie == NULL)
		return B_ERROR;
	*_cookie = cookie;
	cookie->child = gDeviceManager->get_root();
	cookie->parent = NULL;
	cookie->attr = NULL;
	return B_OK;
}


static status_t
config_close(void *cookie)
{
	return B_OK;
}


static status_t
config_free_cookie(void *cookie)
{
	driver_cookie *cook = (driver_cookie *)cookie;
	gDeviceManager->put_device_node(cook->child);
	if (cook->parent)
		gDeviceManager->put_device_node(cook->parent);
	return B_OK;
}


static status_t
config_ioctl(void *cookie, uint32 op, void *buffer, size_t len)
{
	driver_cookie *cook = (driver_cookie *)cookie;
	device_node_handle child = NULL;
	const device_attr *attr = NULL;
	
	struct dm_ioctl_data *params = (struct dm_ioctl_data *)buffer;
	status_t err = B_OK;

	// simple check for validity of the argument
	if (params == NULL || params->magic != op)
		return B_BAD_VALUE;

	switch (op) {
		case DM_GET_CHILD:
			TRACE("DM_GET_CHILD parent %p child %p\n", cook->parent, cook->child);
			if (cook->attr) {
				gDeviceManager->release_attr(cook->child, cook->attr);
				cook->attr = NULL;
			}
			err = gDeviceManager->get_next_child_device(cook->child, &child, NULL);
			if (err == B_OK) {
				if (cook->parent)
					gDeviceManager->put_device_node(cook->parent);
				cook->parent = cook->child;
				cook->child = child;
			}
			return err;
		case DM_GET_NEXT_CHILD:
			TRACE("DM_GET_NEXT_CHILD parent %p child %p\n", cook->parent, cook->child);
			if (!cook->parent)
				return B_ENTRY_NOT_FOUND;
			if (cook->attr) {
				gDeviceManager->release_attr(cook->child, cook->attr);
				cook->attr = NULL;
			}
			return gDeviceManager->get_next_child_device(cook->parent, &cook->child, NULL);
		case DM_GET_PARENT:
			TRACE("DM_GET_PARENT parent %p child %p\n", cook->parent, cook->child);
			if (!cook->parent)
				return B_ENTRY_NOT_FOUND;
			if (cook->attr) {
				gDeviceManager->release_attr(cook->child, cook->attr);
				cook->attr = NULL;
			}
			if (cook->child)
				gDeviceManager->put_device_node(cook->child);
			cook->child = cook->parent;
			cook->parent = gDeviceManager->get_parent(cook->child);
			return B_OK;
		case DM_GET_NEXT_ATTRIBUTE:
			TRACE("DM_NEXT_ATTRIBUTE parent %p child %p attr %p\n", cook->parent, cook->child, cook->attr);
			return gDeviceManager->get_next_attr(cook->child, &cook->attr);
		case DM_RETRIEVE_ATTRIBUTE:
			TRACE("DM_RETRIEVE_ATTRIBUTE parent %p child %p attr %p\n", cook->parent, cook->child, cook->attr);
			err = gDeviceManager->retrieve_attr(cook->attr, &attr);
			if (err == B_OK) {
				strlcpy(params->attr->name, attr->name, 254);
				params->attr->type = attr->type;
				switch (attr->type) {
					case B_UINT8_TYPE:
						params->attr->value.ui8 = attr->value.ui8; break;
					case B_UINT16_TYPE:
						params->attr->value.ui16 = attr->value.ui16; break;
					case B_UINT32_TYPE:
						params->attr->value.ui32 = attr->value.ui32; break;
					case B_UINT64_TYPE:
						params->attr->value.ui64 = attr->value.ui64; break;
					case B_STRING_TYPE:
						strlcpy(params->attr->value.string, attr->value.string, 254); break;
					case B_RAW_TYPE:
						if (params->attr->value.raw.length > attr->value.raw.length)
							params->attr->value.raw.length = attr->value.raw.length;
						memcpy(params->attr->value.raw.data, attr->value.raw.data, 
							params->attr->value.raw.length);
						break;
				}
			}
			return err;
	}

	return B_BAD_VALUE;
}


static status_t
config_read(void * cookie, off_t pos, void *buf, size_t *_length)
{
	*_length = 0;
	return B_OK;
}


static status_t
config_write(void * cookie, off_t pos, const void *buf, size_t *_length)
{
	*_length = 0;
	return B_OK;
}


//	#pragma mark -
//	Driver interface


status_t
init_hardware()
{
	return B_OK;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, 
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		&config_open,
		&config_close,
		&config_free_cookie,
		&config_ioctl,
		&config_read,
		&config_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}


status_t
init_driver()
{
	return get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager);
}


void
uninit_driver()
{
	if (gDeviceManager != NULL)
		put_module(B_DEVICE_MANAGER_MODULE_NAME);
}


/* config driver
 * provides userland access to the device configuration manager
 *
 * Copyright 2002-2004, Axel Doerfler. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Drivers.h>
#include <drivers/config_manager.h>
#include <string.h>

#include "config_driver.h"


#define DEVICE_NAME "misc/config"

int32 api_version = B_CUR_DRIVER_API_VERSION;

struct config_manager_for_driver_module_info *gConfigManager;


//	Device interface


static status_t
config_open(const char *name, uint32 flags, void **_cookie)
{
	*_cookie = NULL;
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
	return B_OK;
}


static status_t
config_ioctl(void *cookie, uint32 op, void *buffer, size_t len)
{
	struct cm_ioctl_data *params = (struct cm_ioctl_data *)buffer;

	// simple check for validity of the argument
	if (params == NULL || params->magic != op)
		return B_BAD_VALUE;

	// ToDo: the access of the params is not safe!

	switch (op) {
		case CM_GET_NEXT_DEVICE_INFO:
			return gConfigManager->get_next_device_info(params->bus, &params->cookie,
				(struct device_info *)params->data, params->data_len);
		case CM_GET_DEVICE_INFO_FOR:
			return gConfigManager->get_device_info_for(params->cookie,
				(struct device_info *)params->data, params->data_len);
		case CM_GET_SIZE_OF_CURRENT_CONFIGURATION_FOR:
			return gConfigManager->get_size_of_current_configuration_for(params->cookie);
		case CM_GET_CURRENT_CONFIGURATION_FOR:
			return gConfigManager->get_current_configuration_for(params->cookie,
				(struct device_configuration *)params->data, params->data_len);
		case CM_GET_SIZE_OF_POSSIBLE_CONFIGURATIONS_FOR:
			return gConfigManager->get_size_of_possible_configurations_for(params->cookie);
		case CM_GET_POSSIBLE_CONFIGURATIONS_FOR:
			return gConfigManager->get_possible_configurations_for(params->cookie,
				(struct possible_device_configurations *)params->data, params->data_len);
		case CM_COUNT_RESOURCE_DESCRIPTORS_OF_TYPE:
			return gConfigManager->count_resource_descriptors_of_type(params->config, params->type);
		case CM_GET_NTH_RESOURCE_DESCRIPTOR_OF_TYPE:
			return gConfigManager->get_nth_resource_descriptor_of_type(params->config, params->n,
				params->type, (resource_descriptor *)params->data, params->data_len);
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
	return get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&gConfigManager);
}


void
uninit_driver()
{
	put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
}


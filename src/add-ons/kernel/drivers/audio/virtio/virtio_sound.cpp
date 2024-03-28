/*
 *  Copyright 2024, Diego Roux, diegoroux04 at proton dot me
 *  Distributed under the terms of the MIT License.
 */

#include <fs/devfs.h>
#include <virtio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define VIRTIO_SOUND_DRIVER_MODULE_NAME 	"drivers/audio/virtio/driver_v1"
#define VIRTIO_SOUND_DEVICE_MODULE_NAME 	"drivers/audio/virtio/device_v1"
#define VIRTIO_SOUND_DEVICE_ID_GEN 			"virtio_sound/device_id"


struct {
	device_node* 				node;
	::virtio_device 			virtio_dev;
	virtio_device_interface*	iface;
	uint32						features;
} VirtIOSoundDriverInfo;

typedef struct {
    VirtIOSoundDriverInfo*		info;
} VirtIOSoundHandle;

static device_manager_info*		sDeviceManager;


const char*
get_feature_name(uint32 feature)
{
	// TODO: Implement this.
	return NULL;
}


static float
SupportsDevice(device_node* parent)
{
	uint16 deviceType;
	char* bus;

	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK) {
		return 0.0f;
	}

	if (strcmp(bus, "virtio") != 0)
		return 0.0f;

	if (deviceType != VIRTIO_DEVICE_ID_SOUND)
		return 0.0f;

	return 1.0f;
}


static status_t
InitDriver(device_node* node, void** cookie)
{
	VirtIOSoundDriverInfo* info = (VirtIOSoundDriverInfo*)malloc(sizeof(VirtIOSoundDriverInfo));

	if (info == NULL)
		return B_NO_MEMORY;

	info->node = node;
	*cookie = info;

	return B_OK;
}


static status_t
UninitDriver(void* cookie)
{
	free(cookie);
	return B_OK;
}


struct driver_module_info sVirtioSoundDriver = {
	{
		VIRTIO_SOUND_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	.supports_device = SupportsDevice,

	.init_driver = InitDriver,
	.uninit_driver = UninitDriver,
};


struct device_module_info sVirtioSoundDevice = {
	{
		VIRTIO_SOUND_DEVICE_MODULE_NAME,
		0,
		NULL
	},
};


module_info* modules[] = {
	(module_info*)&sVirtioSoundDriver,
	(module_info*)&sVirtioSoundDevice,
	NULL
};


module_dependency module_dependencies[] = {
	{
		B_DEVICE_MANAGER_MODULE_NAME,
		(module_info**)&sDeviceManager
	},
	{}
};

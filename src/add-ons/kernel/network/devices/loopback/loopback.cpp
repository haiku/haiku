/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <net_device.h>

#include <KernelExport.h>

#include <net/if.h>
#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>


struct loopback_device : net_device {
};


status_t
loopback_init(const char *name, net_device **_device)
{
	if (strncmp(name, "loop", 4))
		return B_BAD_VALUE;

	loopback_device *device = new (std::nothrow) loopback_device;
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(loopback_device));

	strcpy(device->name, name);
	device->flags = IFF_LOOPBACK;
	device->type = IFT_LOOP;
	device->mtu = 16384;

	*_device = device;
	return B_OK;
}


status_t
loopback_uninit(net_device *device)
{
	delete device;
	return B_OK;
}


status_t
loopback_up(net_device *device)
{
	return B_OK;
}


void
loopback_down(net_device *device)
{
}


status_t
loopback_control(net_device *device, int32 op, void *argument,
	size_t length)
{
	return B_BAD_VALUE;
}


status_t
loopback_send_data(net_device *device, net_buffer *buffer)
{
	return B_ERROR;
}


status_t
loopback_receive_data(net_device *device, net_buffer **_buffer)
{
	return B_ERROR;
}


status_t
loopback_set_mtu(net_device *device, size_t mtu)
{
	return B_ERROR;
}


status_t
loopback_set_promiscuous(net_device *device, bool promiscuous)
{
	return B_ERROR;
}


status_t
loopback_set_media(net_device *device, uint32 media)
{
	return B_ERROR;
}


static status_t
loopback_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_device_module_info sLoopbackModule = {
	{
		"network/devices/loopback/v1",
		0,
		loopback_std_ops
	},
	loopback_init,
	loopback_uninit,
	loopback_up,
	loopback_down,
	loopback_control,
	loopback_send_data,
	loopback_receive_data,
	loopback_set_mtu,
	loopback_set_promiscuous,
	loopback_set_media,
};

module_info *modules[] = {
	(module_info *)&sLoopbackModule,
	NULL
};

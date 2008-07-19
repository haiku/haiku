/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>

#include <KernelExport.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <new>
#include <stdlib.h>
#include <string.h>


struct loopback_device : net_device {
};


struct net_buffer_module_info *gBufferModule;
static struct net_stack_module_info *sStackModule;


//	#pragma mark -


status_t
loopback_init(const char *name, net_device **_device)
{
	loopback_device *device;

	if (strncmp(name, "loop", 4))
		return B_BAD_VALUE;

	status_t status = get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);
	if (status < B_OK)
		return status;

	status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK)
		goto err1;

	device = new (std::nothrow) loopback_device;
	if (device == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	memset(device, 0, sizeof(loopback_device));

	strcpy(device->name, name);
	device->flags = IFF_LOOPBACK | IFF_LINK;
	device->type = IFT_LOOP;
	device->mtu = 16384;
	device->media = IFM_ACTIVE;

	*_device = device;
	return B_OK;

err2:
	put_module(NET_BUFFER_MODULE_NAME);
err1:
	put_module(NET_STACK_MODULE_NAME);
	return status;
}


status_t
loopback_uninit(net_device *_device)
{
	loopback_device *device = (loopback_device *)_device;

	put_module(NET_STACK_MODULE_NAME);
	put_module(NET_BUFFER_MODULE_NAME);
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
	return sStackModule->device_enqueue_buffer(device, buffer);
}


status_t
loopback_set_mtu(net_device *device, size_t mtu)
{
	if (mtu > 65536 || mtu < 16)
		return B_BAD_VALUE;

	device->mtu = mtu;
	return B_OK;
}


status_t
loopback_set_promiscuous(net_device *device, bool promiscuous)
{
	return EOPNOTSUPP;
}


status_t
loopback_set_media(net_device *device, uint32 media)
{
	return EOPNOTSUPP;
}


status_t
loopback_add_multicast(net_device *device, const sockaddr *address)
{
	return B_OK;
}


status_t
loopback_remove_multicast(net_device *device, const sockaddr *address)
{
	return B_OK;
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
	NULL, // receive_data
	loopback_set_mtu,
	loopback_set_promiscuous,
	loopback_set_media,
	loopback_add_multicast,
	loopback_remove_multicast,
};

module_info *modules[] = {
	(module_info *)&sLoopbackModule,
	NULL
};

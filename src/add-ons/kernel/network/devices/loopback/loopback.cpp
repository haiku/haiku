/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
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
#include <new>
#include <stdlib.h>
#include <string.h>


struct loopback_device : net_device {
	net_fifo	fifo;
};


struct net_buffer_module_info *gBufferModule;
static struct net_stack_module_info *sStackModule;


/*!
	Swaps \a size bytes of the memory pointed to by \a and \b with each other.
*/
void
swap_memory(void *a, void *b, size_t size)
{
	uint32 *a4 = (uint32 *)a;
	uint32 *b4 = (uint32 *)b;
	while (size > 4) {
		uint32 temp = *a4;
		*(a4++) = *b4;
		*(b4++) = temp;

		size -= 4;
	}

	uint8 *a1 = (uint8 *)a4;
	uint8 *b1 = (uint8 *)b4;
	while (size > 0) {
		uint8 temp = *a1;
		*(a1++) = *b1;
		*(b1++) = temp;

		size--;
	}
}


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
	device->flags = IFF_LOOPBACK;
	device->type = IFT_LOOP;
	device->mtu = 16384;

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
loopback_up(net_device *_device)
{
	loopback_device *device = (loopback_device *)_device;
	return sStackModule->init_fifo(&device->fifo, "loopback fifo", 65536);
}


void
loopback_down(net_device *_device)
{
	loopback_device *device = (loopback_device *)_device;
	sStackModule->uninit_fifo(&device->fifo);
}


status_t
loopback_control(net_device *device, int32 op, void *argument,
	size_t length)
{
	return B_BAD_VALUE;
}


status_t
loopback_send_data(net_device *_device, net_buffer *buffer)
{
	loopback_device *device = (loopback_device *)_device;
	return sStackModule->fifo_enqueue_buffer(&device->fifo, buffer);
}


status_t
loopback_receive_data(net_device *_device, net_buffer **_buffer)
{
	loopback_device *device = (loopback_device *)_device;
	net_buffer *buffer;

	status_t status = sStackModule->fifo_dequeue_buffer(&device->fifo, 0, 0, &buffer);
	if (status < B_OK)
		return status;

	// switch network addresses before delivering
	swap_memory(&buffer->source, &buffer->destination,
		max_c(buffer->source.ss_len, buffer->destination.ss_len));

	*_buffer = buffer;
	return B_OK;
}


status_t
loopback_set_mtu(net_device *device, size_t mtu)
{
	if (mtu > 65536
		|| mtu < 16)
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

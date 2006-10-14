/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <ethernet.h>
#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_datalink.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <new>
#include <string.h>


struct loopback_frame_protocol : net_datalink_protocol {
};


struct net_buffer_module_info *sBufferModule;


int32
loopback_deframe(net_device *device, net_buffer *buffer)
{
	// there is not that much to do...
	return 0;
}


//	#pragma mark -


status_t
loopback_frame_init(struct net_interface *interface, net_datalink_protocol **_protocol)
{
	// We currently only support a single family and type!
	if (interface->device->type != IFT_LOOP)
		return B_BAD_TYPE;

	loopback_frame_protocol *protocol;

	net_stack_module_info *stack;
	status_t status = get_module(NET_STACK_MODULE_NAME, (module_info **)&stack);
	if (status < B_OK)
		return status;

	status = stack->register_device_deframer(interface->device, &loopback_deframe);
	if (status < B_OK)
		goto err1;

	// We also register the domain as a handler for our packets
	status = stack->register_domain_device_handler(interface->device,
		0, interface->domain);
	if (status < B_OK)
		goto err2;

	put_module(NET_STACK_MODULE_NAME);

	protocol = new (std::nothrow) loopback_frame_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	*_protocol = protocol;
	return B_OK;

err2:
	stack->unregister_device_deframer(interface->device);
err1:
	put_module(NET_STACK_MODULE_NAME);
	return status;
}


status_t
loopback_frame_uninit(net_datalink_protocol *protocol)
{
	net_stack_module_info *stack;
	if (get_module(NET_STACK_MODULE_NAME, (module_info **)&stack) == B_OK) {
		stack->unregister_device_deframer(protocol->interface->device);
		stack->unregister_device_handler(protocol->interface->device, 0);
		put_module(NET_STACK_MODULE_NAME);
	}

	delete protocol;
	return B_OK;
}


status_t
loopback_frame_send_data(net_datalink_protocol *protocol,
	net_buffer *buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
loopback_frame_up(net_datalink_protocol *_protocol)
{
	loopback_frame_protocol *protocol = (loopback_frame_protocol *)_protocol;

	return protocol->next->module->interface_up(protocol->next);
}


void
loopback_frame_down(net_datalink_protocol *protocol)
{
	return protocol->next->module->interface_down(protocol->next);
}


status_t
loopback_frame_control(net_datalink_protocol *protocol,
	int32 op, void *argument, size_t length)
{
	return protocol->next->module->control(protocol->next, op, argument, length);
}


static status_t
loopback_frame_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return get_module(NET_BUFFER_MODULE_NAME, (module_info **)&sBufferModule);
		case B_MODULE_UNINIT:
			put_module(NET_BUFFER_MODULE_NAME);
			return B_OK;

		default:
			return B_ERROR;
	}
}


static net_datalink_protocol_module_info sLoopbackFrameModule = {
	{
		"network/datalink_protocols/loopback_frame/v1",
		0,
		loopback_frame_std_ops
	},
	loopback_frame_init,
	loopback_frame_uninit,
	loopback_frame_send_data,
	loopback_frame_up,
	loopback_frame_down,
	loopback_frame_control,
};

module_info *modules[] = {
	(module_info *)&sLoopbackFrameModule,
	NULL
};

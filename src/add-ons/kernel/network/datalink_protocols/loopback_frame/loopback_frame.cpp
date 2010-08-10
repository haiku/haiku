/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
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


struct net_buffer_module_info* gBufferModule;


status_t
loopback_deframe(net_device* device, net_buffer* buffer)
{
	// there is not that much to do...
	return B_OK;
}


//	#pragma mark -


status_t
loopback_frame_init(struct net_interface*interface, net_domain* domain,
	net_datalink_protocol** _protocol)
{
	// We only support a single type!
	if (interface->device->type != IFT_LOOP)
		return B_BAD_TYPE;

	loopback_frame_protocol* protocol;

	net_stack_module_info* stack;
	status_t status = get_module(NET_STACK_MODULE_NAME, (module_info**)&stack);
	if (status != B_OK)
		return status;

	status = stack->register_device_deframer(interface->device,
		&loopback_deframe);
	if (status != B_OK)
		goto err1;

	// Locally received buffers don't need a domain device handler, as the
	// buffer reception is handled internally.

	protocol = new(std::nothrow) loopback_frame_protocol;
	if (protocol == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	put_module(NET_STACK_MODULE_NAME);

	*_protocol = protocol;
	return B_OK;

err2:
	stack->unregister_device_deframer(interface->device);
err1:
	put_module(NET_STACK_MODULE_NAME);
	return status;
}


status_t
loopback_frame_uninit(net_datalink_protocol* protocol)
{
	net_stack_module_info* stack;
	if (get_module(NET_STACK_MODULE_NAME, (module_info**)&stack) == B_OK) {
		stack->unregister_device_deframer(protocol->interface->device);
		stack->unregister_device_handler(protocol->interface->device, 0);
		put_module(NET_STACK_MODULE_NAME);
	}

	delete protocol;
	return B_OK;
}


status_t
loopback_frame_send_data(net_datalink_protocol* protocol, net_buffer* buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
loopback_frame_up(net_datalink_protocol* protocol)
{
	return protocol->next->module->interface_up(protocol->next);
}


void
loopback_frame_down(net_datalink_protocol* protocol)
{
	return protocol->next->module->interface_down(protocol->next);
}


status_t
loopback_frame_change_address(net_datalink_protocol* protocol,
	net_interface_address* address, int32 option,
	const struct sockaddr* oldAddress, const struct sockaddr* newAddress)
{
	return protocol->next->module->change_address(protocol->next, address,
		option, oldAddress, newAddress);
}


status_t
loopback_frame_control(net_datalink_protocol* protocol, int32 option,
	void* argument, size_t length)
{
	return protocol->next->module->control(protocol->next, option, argument,
		length);
}


static status_t
loopback_frame_join_multicast(net_datalink_protocol* protocol,
	const sockaddr* address)
{
	return protocol->next->module->join_multicast(protocol->next, address);
}


static status_t
loopback_frame_leave_multicast(net_datalink_protocol* protocol,
	const sockaddr* address)
{
	return protocol->next->module->leave_multicast(protocol->next, address);
}


static status_t
loopback_frame_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return get_module(NET_BUFFER_MODULE_NAME,
				(module_info**)&gBufferModule);
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
	loopback_frame_change_address,
	loopback_frame_control,
	loopback_frame_join_multicast,
	loopback_frame_leave_multicast,
};

module_info* modules[] = {
	(module_info*)&sLoopbackFrameModule,
	NULL
};

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


struct ethernet_frame_protocol : net_datalink_protocol {
};


static const uint8 kBroadcastAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct net_buffer_module_info *sBufferModule;


int32
ethernet_deframe(net_device *device, net_buffer *buffer)
{
	//dprintf("asked to deframe buffer for device %s\n", device->name);

	NetBufferHeader<ether_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	ether_header &header = bufferHeader.Data();
	uint16 type = ntohs(header.type);

	struct sockaddr_dl &source = *(struct sockaddr_dl *)&buffer->source;
	struct sockaddr_dl &destination = *(struct sockaddr_dl *)&buffer->destination;

	source.sdl_len = sizeof(sockaddr_dl);
	source.sdl_family = AF_DLI;
	source.sdl_index = device->index;
	source.sdl_type = IFT_ETHER;
	source.sdl_e_type = type;
	source.sdl_nlen = source.sdl_slen = 0;
	source.sdl_alen = ETHER_ADDRESS_LENGTH;
	memcpy(source.sdl_data, header.source, ETHER_ADDRESS_LENGTH);

	destination.sdl_len = sizeof(sockaddr_dl);
	destination.sdl_family = AF_DLI;
	destination.sdl_index = device->index;
	destination.sdl_type = IFT_ETHER;
	destination.sdl_nlen = destination.sdl_slen = 0;
	destination.sdl_alen = ETHER_ADDRESS_LENGTH;
	memcpy(destination.sdl_data, header.destination, ETHER_ADDRESS_LENGTH);

	// mark buffer if it was a broadcast/multicast packet
	if (!memcmp(header.destination, kBroadcastAddress, ETHER_ADDRESS_LENGTH))
		buffer->flags |= MSG_BCAST;
	else if (header.destination[0] & 0x01)
		buffer->flags |= MSG_MCAST;

	return ETHER_FRAME_TYPE | type;
}


//	#pragma mark -


status_t
ethernet_frame_init(struct net_interface *interface, net_datalink_protocol **_protocol)
{
	net_stack_module_info *stack;
	status_t status = get_module(NET_STACK_MODULE_NAME, (module_info **)&stack);
	if (status < B_OK)
		return status;

	status = stack->register_device_deframer(interface->device, &ethernet_deframe);
	put_module(NET_STACK_MODULE_NAME);

	if (status < B_OK)
		return status;

	ethernet_frame_protocol *protocol = new (std::nothrow) ethernet_frame_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	*_protocol = protocol;
	return B_OK;
}


status_t
ethernet_frame_uninit(net_datalink_protocol *protocol)
{
	net_stack_module_info *stack;
	if (get_module(NET_STACK_MODULE_NAME, (module_info **)&stack) == B_OK) {
		stack->unregister_device_deframer(protocol->interface->device);
		put_module(NET_STACK_MODULE_NAME);
	}

	delete protocol;
	return B_OK;
}


status_t
ethernet_frame_send_data(net_datalink_protocol *protocol,
	net_buffer *buffer)
{
	struct sockaddr_dl &source = *(struct sockaddr_dl *)&buffer->source;
	struct sockaddr_dl &destination = *(struct sockaddr_dl *)&buffer->destination;

	if (source.sdl_family != AF_DLI || source.sdl_type != IFT_ETHER)
		return B_ERROR;

	NetBufferPrepend<ether_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	ether_header &header = bufferHeader.Data();

	header.type = htons(source.sdl_e_type);
	memcpy(header.source, source.sdl_data, ETHER_ADDRESS_LENGTH);
	if (buffer->flags & MSG_BCAST)
		memcpy(header.destination, kBroadcastAddress, ETHER_ADDRESS_LENGTH);
	else
		memcpy(header.destination, destination.sdl_data, ETHER_ADDRESS_LENGTH);

	bufferHeader.Detach();
		// make sure the framing is already written to the buffer at this point

	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
ethernet_frame_up(net_datalink_protocol *_protocol)
{
	ethernet_frame_protocol *protocol = (ethernet_frame_protocol *)_protocol;

	return protocol->next->module->interface_up(protocol->next);
}


void
ethernet_frame_down(net_datalink_protocol *protocol)
{
	return protocol->next->module->interface_down(protocol->next);
}


status_t
ethernet_frame_control(net_datalink_protocol *protocol,
	int32 op, void *argument, size_t length)
{
	return protocol->next->module->control(protocol->next, op, argument, length);
}


static status_t
ethernet_frame_std_ops(int32 op, ...)
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


static net_datalink_protocol_module_info sEthernetFrameModule = {
	{
		"network/datalink_protocols/ethernet_frame/v1",
		0,
		ethernet_frame_std_ops
	},
	ethernet_frame_init,
	ethernet_frame_uninit,
	ethernet_frame_send_data,
	ethernet_frame_up,
	ethernet_frame_down,
	ethernet_frame_control,
};

module_info *modules[] = {
	(module_info *)&sEthernetFrameModule,
	NULL
};

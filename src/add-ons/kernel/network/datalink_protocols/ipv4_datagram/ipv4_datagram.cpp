/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hugo Santos, hugosantos@gmail.com
 */


#include <net_datalink.h>
#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_stack.h>

#include <KernelExport.h>

#include <net/ethernet.h> // for ETHER_TYPE_IP
#include <net/if_types.h>

#include <new>


static net_stack_module_info *sStackModule;
// TODO ETHER_FRAME_TYPE doesn't belong there, we need Layer 2
//      independence.
static const int32 kIPv4FrameType = ETHER_FRAME_TYPE | ETHER_TYPE_IP;


static status_t
ipv4_datalink_init(net_interface *interface, net_datalink_protocol **_protocol)
{
	dprintf("ipv4_datalink_init(%s)\n", interface->name);

	if (interface->domain->family != AF_INET)
		return B_BAD_TYPE;

	// While the loopback doesn't get a header to mux protocols,
	// we let it do all of the registration work.
	if (interface->device->type == IFT_LOOP)
		return B_BAD_TYPE;

	net_datalink_protocol *protocol = new (std::nothrow) net_datalink_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	// We register ETHER_TYPE_IP as most datalink protocols use it
	// to identify IP datagrams. In the future we may limit this.

	status_t status = sStackModule->register_domain_device_handler(
		interface->device, kIPv4FrameType, interface->domain);
	if (status < B_OK)
		delete protocol;
	else
		*_protocol = protocol;

	return status;
}


static status_t
ipv4_datalink_uninit(net_datalink_protocol *protocol)
{
	sStackModule->unregister_device_handler(protocol->interface->device,
		kIPv4FrameType);
	delete protocol;
	return B_OK;
}


static status_t
ipv4_datalink_send_data(net_datalink_protocol *protocol, net_buffer *buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


static status_t
ipv4_datalink_up(net_datalink_protocol *protocol)
{
	return protocol->next->module->interface_up(protocol->next);
}


static void
ipv4_datalink_down(net_datalink_protocol *protocol)
{
	// TODO Clear routes here instead?
	protocol->next->module->interface_down(protocol->next);
}


static status_t
ipv4_datalink_control(net_datalink_protocol *protocol, int32 op,
	void *argument, size_t length)
{
	return protocol->next->module->control(protocol->next, op, argument, length);
}


static status_t
ipv4_datalink_std_ops(int32 op, ...)
{
	switch (op) {
	case B_MODULE_INIT:
		return get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);

	case B_MODULE_UNINIT:
		return put_module(NET_STACK_MODULE_NAME);
	}

	return B_ERROR;
}

net_datalink_protocol_module_info gIPv4DataLinkModule = {
	{
		"network/datalink_protocols/ipv4_datagram/v1",
		0,
		ipv4_datalink_std_ops
	},
	ipv4_datalink_init,
	ipv4_datalink_uninit,
	ipv4_datalink_send_data,
	ipv4_datalink_up,
	ipv4_datalink_down,
	ipv4_datalink_control,
};

module_info *modules[] = {
	(module_info *)&gIPv4DataLinkModule,
	NULL
};

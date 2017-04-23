/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Lin Longzhou, linlongzhou@163.com
 */


#include <ethernet.h>
#include <net/if_types.h>
#include <net_datalink_protocol.h>
#include <net_datalink.h>

#include <KernelExport.h>

#include <new>

#include <PPPControl.h>
#include <ppp_device.h>

struct ppp_frame_protocol : net_datalink_protocol {
};


static net_stack_module_info *sStackModule;

status_t
ppp_deframe(net_device* device, net_buffer* buffer)
{
	TRACE("%s: buffer type:0x%" B_PRIx32 "\n", __func__, buffer->type);
	return B_OK;
}


//	#pragma mark -

status_t
ppp_frame_init(struct net_interface* interface, net_domain* domain,
	net_datalink_protocol** _protocol)
{
	// We only support a single type!
	dprintf("in function: %s::%s\n", __FILE__, __func__);
	if (interface->device->type != IFT_PPP)
		return B_BAD_TYPE;

	ppp_frame_protocol* protocol;

	status_t status = sStackModule->register_device_deframer(interface->device,
		&ppp_deframe);
	if (status != B_OK)
		return status;

	status = sStackModule->register_domain_device_handler(
		interface->device, B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_PPPOE), domain);
	if (status != B_OK)
		return status;

	status = sStackModule->register_domain_device_handler(
		interface->device, B_NET_FRAME_TYPE_IPV4, domain);
	if (status != B_OK)
		return status;

	// Locally received buffers don't need a domain device handler, as the
	// buffer reception is handled internally.

	protocol = new(std::nothrow) ppp_frame_protocol;
	if (protocol == NULL) {
		sStackModule->unregister_device_deframer(interface->device);
		return B_NO_MEMORY;
	}

	*_protocol = protocol;
	return B_OK;
}


status_t
ppp_frame_uninit(net_datalink_protocol* protocol)
{
	sStackModule->unregister_device_deframer(protocol->interface->device);
	sStackModule->unregister_device_handler(protocol->interface->device, 0);

	delete protocol;
	TRACE("%s\n", __func__);
	return B_OK;
}


status_t
ppp_frame_send_data(net_datalink_protocol* protocol, net_buffer* buffer)
{
	TRACE("%s: next module is %s\n", __func__, protocol->next->module->info.name);
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
ppp_frame_up(net_datalink_protocol* protocol)
{
	dprintf("in function: %s\n", __func__);

	return protocol->next->module->interface_up(protocol->next);
}


void
ppp_frame_down(net_datalink_protocol* protocol)
{
	dprintf("%s::%s: entry\n", __FILE__, __func__);
	protocol->next->module->interface_down(protocol->next);
	return;
}


status_t
ppp_frame_change_address(net_datalink_protocol* protocol,
	net_interface_address* address, int32 option,
	const struct sockaddr* oldAddress, const struct sockaddr* newAddress)
{
	TRACE("in function: %s::%s\n", __FILE__, __func__);
	return protocol->next->module->change_address(protocol->next, address,
		option, oldAddress, newAddress);
}


static status_t
PPP_module_control(int32 option, void* argument, size_t length)
{
	// control_net_module_args* args=(control_net_module_args*)argument;

	if (option < PPPC_CONTROL_MODULE || option > PPP_CONTROL_OPS_END)
		return B_ERROR;

	// if (args->name != PPP_INTERFACE_MODULE_NAME)
		// return B_ERROR;

	switch(option)
	{
		case PPPC_CREATE_INTERFACE:
			// process_CreateInterface(args->data, args->length);
		case PPPC_CREATE_INTERFACE_WITH_NAME:
		case PPPC_DELETE_INTERFACE:
		case PPPC_BRING_INTERFACE_UP:
		case PPPC_BRING_INTERFACE_DOWN:
		case PPPC_CONTROL_INTERFACE:
		case PPPC_GET_INTERFACES:
		case PPPC_COUNT_INTERFACES:
		case PPPC_FIND_INTERFACE_WITH_SETTINGS:

		// interface control
		case PPPC_GET_INTERFACE_INFO:
		case PPPC_SET_USERNAME:
		case PPPC_SET_PASSWORD:
		case PPPC_SET_ASK_BEFORE_CONNECTING:
			// ppp_up uses this in order to finalize a connection request
		case PPPC_SET_MRU:
		case PPPC_SET_CONNECT_ON_DEMAND:
		case PPPC_SET_AUTO_RECONNECT:
		case PPPC_HAS_INTERFACE_SETTINGS:
		case PPPC_GET_STATISTICS:
		// handler access
		case PPPC_CONTROL_DEVICE:
		case PPPC_CONTROL_PROTOCOL:
		case PPPC_CONTROL_OPTION_HANDLER:
		case PPPC_CONTROL_LCP_EXTENSION:
		case PPPC_CONTROL_CHILD:
		// KPPPDevice
		case PPPC_GET_DEVICE_INFO:
		// KPPPProtocol
		case PPPC_GET_PROTOCOL_INFO:
		// Common/mixed ops
		case PPPC_ENABLE:
		case PPPC_GET_SIMPLE_HANDLER_INFO:

		// these two control ops use the ppp_report_request structure
		case PPPC_ENABLE_REPORTS:
		case PPPC_DISABLE_REPORTS:

		case PPP_CONTROL_OPS_END:
		case PPPC_CONTROL_MODULE:
		default:
			return B_ERROR;
	}
}


static status_t
ppp_frame_control(net_datalink_protocol* protocol, int32 option,
	void* argument, size_t length)
{
	TRACE("%s: option:%" B_PRId32 "\n", __func__, option);

	if (option >= PPPC_CONTROL_MODULE && option <=PPP_CONTROL_OPS_END)
	{
		status_t status =PPP_module_control(option, argument, length);
		// gPPPInterfaceModule->ControlInterface(1, option, argument, length);
		return status;
	}

	return protocol->next->module->control(protocol->next, option, argument,
		length);
}


static status_t
ppp_frame_join_multicast(net_datalink_protocol* protocol,
	const sockaddr* address)
{
	TRACE("in function: %s\n", __func__);
	return protocol->next->module->join_multicast(protocol->next, address);
}


static status_t
ppp_frame_leave_multicast(net_datalink_protocol* protocol,
	const sockaddr* address)
{
	TRACE("in function: %s\n", __func__);
	return protocol->next->module->leave_multicast(protocol->next, address);
}


static status_t
pppoe_frame_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			if (get_module(NET_STACK_MODULE_NAME,
				(module_info **)&sStackModule) != B_OK)
				return B_ERROR;
			return B_OK;

		case B_MODULE_UNINIT:
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;

		default:
			return B_ERROR;
	}
}


static net_datalink_protocol_module_info sPPPFrameModule = {
	{
		"network/datalink_protocols/ppp_frame/v1",
		0,
		pppoe_frame_std_ops
	},
	ppp_frame_init,
	ppp_frame_uninit,
	ppp_frame_send_data,
	ppp_frame_up,
	ppp_frame_down,
	ppp_frame_change_address,
	ppp_frame_control,
	ppp_frame_join_multicast,
	ppp_frame_leave_multicast,
};


module_info* modules[] = {
	(module_info*)&sPPPFrameModule,
	NULL
};

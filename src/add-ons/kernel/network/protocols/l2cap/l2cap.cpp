/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

/*-
 * Copyright (c) Maksim Yevmenkin <m_evmenkin@yahoo.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
*/


#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <KernelExport.h>
#include <util/list.h>
#include <util/DoublyLinkedList.h>

#include <new>
#include <stdlib.h>
#include <string.h>

#include "l2cap_address.h"
#include "l2cap_internal.h"
#include "l2cap_lower.h"
#include "L2capEndpoint.h"

#include <bluetooth/HCI/btHCI_acl.h>
#include <btModules.h>


#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "L2cap"
#define SUBMODULE_COLOR 32
#include <btDebug.h>


typedef NetBufferField<uint16, offsetof(hci_acl_header, alen)> AclLenField;
DoublyLinkedList<L2capEndpoint> EndpointList;

extern net_protocol_module_info gL2CAPModule;

// module references
bluetooth_core_data_module_info* btCoreData;

net_buffer_module_info* gBufferModule;
net_stack_module_info* gStackModule;
net_socket_module_info* gSocketModule;

static struct net_domain* sDomain;

net_protocol*
l2cap_init_protocol(net_socket* socket)
{
	L2capEndpoint* protocol = new(std::nothrow) L2capEndpoint(socket);
	if (protocol == NULL)
		return NULL;

	EndpointList.Add(protocol);
	debugf("Prococol created %p\n", protocol);

	return protocol;
}


status_t
l2cap_uninit_protocol(net_protocol* protocol)
{
	flowf("\n");

	L2capEndpoint* endpoint = static_cast<L2capEndpoint*>(protocol);

	// TODO: Some more checkins	/ uninit
	EndpointList.Remove(endpoint);

	delete endpoint;

	return B_OK;
}


status_t
l2cap_open(net_protocol* protocol)
{
	flowf("\n");

	return B_OK;
}


status_t
l2cap_close(net_protocol* protocol)
{
	L2capEndpoint* endpoint = static_cast<L2capEndpoint*>(protocol);

	flowf("\n");

	endpoint->Close();

	return B_OK;
}


status_t
l2cap_free(net_protocol* protocol)
{
	flowf("\n");

	return B_OK;
}


status_t
l2cap_connect(net_protocol* protocol, const struct sockaddr* address)
{
	debugf("from %p, with %p\n", protocol, address);

	if (address == NULL)
		return EINVAL;

 	if (address->sa_family != AF_BLUETOOTH)
		return EAFNOSUPPORT;


	return ((L2capEndpoint*)protocol)->Connect(address);;
}


status_t
l2cap_accept(net_protocol* protocol, struct net_socket** _acceptedSocket)
{
	return ((L2capEndpoint*)protocol)->Accept(_acceptedSocket);
}


status_t
l2cap_control(net_protocol* protocol, int level, int option, void* value,
	size_t* _length)
{
	flowf("\n");

	return EOPNOTSUPP;
}


status_t
l2cap_getsockopt(net_protocol* protocol, int level, int option,
	void* value, int* length)
{
	flowf("\n");

	return B_OK;
}


status_t
l2cap_setsockopt(net_protocol* protocol, int level, int option,
	const void* value, int length)
{
	flowf("\n");

	((L2capEndpoint*)protocol)->fConfigurationSet = true;

	return B_OK;
}


status_t
l2cap_bind(net_protocol* protocol, const struct sockaddr* address)
{
	debugf("from %p, with %p\n", protocol, address);

	return ((L2capEndpoint*)protocol)->Bind(address);
}


status_t
l2cap_unbind(net_protocol* protocol, struct sockaddr* address)
{
	flowf("\n");

	return B_ERROR;
}


status_t
l2cap_listen(net_protocol* protocol, int count)
{
	return ((L2capEndpoint*)protocol)->Listen(count);
}


status_t
l2cap_shutdown(net_protocol* protocol, int direction)
{
	flowf("\n");

	return EOPNOTSUPP;
}


status_t
l2cap_send_data(net_protocol* protocol, net_buffer* buffer)
{
	flowf("\n");

	if (buffer == NULL)
		return ENOBUFS;

	return ((L2capEndpoint*)protocol)->SendData(buffer);
}


status_t
l2cap_send_routed_data(net_protocol* protocol, struct net_route* route,
	net_buffer* buffer)
{
	flowf("\n");

	return EOPNOTSUPP;
}


ssize_t
l2cap_send_avail(net_protocol* protocol)
{
	flowf("\n");

	return B_ERROR;
}


status_t
l2cap_read_data(net_protocol* protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	flowf("\n");

	return ((L2capEndpoint*)protocol)->ReadData(numBytes, flags, _buffer);
}


ssize_t
l2cap_read_avail(net_protocol* protocol)
{
	flowf("\n");

	return B_ERROR;
}


struct net_domain*
l2cap_get_domain(net_protocol* protocol)
{
	flowf("\n");

	return sDomain;
}


size_t
l2cap_get_mtu(net_protocol* protocol, const struct sockaddr* address)
{
	flowf("\n");

	return protocol->next->module->get_mtu(protocol->next, address);
}


status_t
l2cap_receive_data(net_buffer* buffer)
{
	HciConnection* conn = (HciConnection*)buffer;
	debugf("received some data, buffer length %lu\n",
		conn->currentRxPacket->size);

	l2cap_receive(conn, conn->currentRxPacket);

	return B_OK;
}


status_t
l2cap_error_received(net_error error, net_buffer* data)
{
	flowf("\n");

	return B_ERROR;
}


status_t
l2cap_error_reply(net_protocol* protocol, net_buffer* cause, net_error error,
	net_error_data* errorData)
{
	flowf("\n");

	return B_ERROR;
}


// #pragma mark -


static status_t
l2cap_std_ops(int32 op, ...)
{
	status_t error;

	flowf("\n");

	switch (op) {
		case B_MODULE_INIT:
		{
			error = gStackModule->register_domain_protocols(AF_BLUETOOTH,
				SOCK_STREAM, BLUETOOTH_PROTO_L2CAP,
				"network/protocols/l2cap/v1",
				NULL);
			if (error != B_OK)
				return error;

			error = gStackModule->register_domain_receiving_protocol(
				AF_BLUETOOTH,
				BLUETOOTH_PROTO_L2CAP,
				"network/protocols/l2cap/v1");
			if (error != B_OK)
				return error;

			error = gStackModule->register_domain(AF_BLUETOOTH, "l2cap",
				&gL2CAPModule, &gL2cap4AddressModule, &sDomain);
			if (error != B_OK)
				return error;

			new (&EndpointList) DoublyLinkedList<L2capEndpoint>;

			error = InitializeConnectionPurgeThread();

			return B_OK;
		}

		case B_MODULE_UNINIT:

			error = QuitConnectionPurgeThread();
			gStackModule->unregister_domain(sDomain);

			return B_OK;

		default:
			return B_ERROR;
	}
}


net_protocol_module_info gL2CAPModule = {
	{
		NET_BLUETOOTH_L2CAP_NAME,
		B_KEEP_LOADED,
		l2cap_std_ops
	},
	NET_PROTOCOL_ATOMIC_MESSAGES,

	l2cap_init_protocol,
	l2cap_uninit_protocol,
	l2cap_open,
	l2cap_close,
	l2cap_free,
	l2cap_connect,
	l2cap_accept,
	l2cap_control,
	l2cap_getsockopt,
	l2cap_setsockopt,
	l2cap_bind,
	l2cap_unbind,
	l2cap_listen,
	l2cap_shutdown,
	l2cap_send_data,
	l2cap_send_routed_data,
	l2cap_send_avail,
	l2cap_read_data,
	l2cap_read_avail,
	l2cap_get_domain,
	l2cap_get_mtu,
	l2cap_receive_data,
	NULL,		// deliver_data()
	l2cap_error_received,
	l2cap_error_reply,
	NULL,		// add_ancillary_data()
	NULL,		// process_ancillary_data()
	NULL,		// process_ancillary_data_no_container()
	NULL,		// send_data_no_buffer()
	NULL		// read_data_no_buffer()
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{BT_CORE_DATA_MODULE_NAME, (module_info**)&btCoreData},
	{NET_SOCKET_MODULE_NAME, (module_info**)&gSocketModule},
	{}
};

module_info* modules[] = {
	(module_info*)&gL2CAPModule,
	NULL
};

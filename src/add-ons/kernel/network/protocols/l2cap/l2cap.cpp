/*
 * Copyright 2007, Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <new>

#include "l2cap_address.h"
#include "l2cap_command.h"
#include "l2cap_internal.h"
#include "l2cap_signal.h"
#include "L2capEndpoint.h"
#include "L2capEndpointManager.h"

#include <bluetooth/HCI/btHCI_acl.h>
#include <btModules.h>
#include <btDebug.h>


extern net_protocol_module_info gL2CAPModule;

// module references
bluetooth_core_data_module_info* btCoreData;
bt_hci_module_info* btDevices;

net_buffer_module_info* gBufferModule;
net_stack_module_info* gStackModule;
net_socket_module_info* gSocketModule;

static struct net_domain* sDomain;


net_protocol*
l2cap_init_protocol(net_socket* socket)
{
	CALLED();

	L2capEndpoint* protocol = new(std::nothrow) L2capEndpoint(socket);
	if (protocol == NULL)
		return NULL;

	return protocol;
}


status_t
l2cap_uninit_protocol(net_protocol* protocol)
{
	CALLED();

	L2capEndpoint* endpoint = static_cast<L2capEndpoint*>(protocol);
	delete endpoint;

	return B_OK;
}


status_t
l2cap_open(net_protocol* protocol)
{
	return ((L2capEndpoint*)protocol)->Open();
}


status_t
l2cap_close(net_protocol* protocol)
{
	return ((L2capEndpoint*)protocol)->Close();
}


status_t
l2cap_free(net_protocol* protocol)
{
	return ((L2capEndpoint*)protocol)->Free();
}


status_t
l2cap_connect(net_protocol* protocol, const struct sockaddr* address)
{
	return ((L2capEndpoint*)protocol)->Connect(address);
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
	CALLED();
	return EOPNOTSUPP;
}


status_t
l2cap_getsockopt(net_protocol* protocol, int level, int option,
	void* value, int* _length)
{
	CALLED();
	return gSocketModule->get_option(protocol->socket, level, option, value,
		_length);
}


status_t
l2cap_setsockopt(net_protocol* protocol, int level, int option,
	const void* _value, int length)
{
	CALLED();
	return gSocketModule->set_option(protocol->socket, level, option,
		_value, length);
}


status_t
l2cap_bind(net_protocol* protocol, const struct sockaddr* address)
{
	return ((L2capEndpoint*)protocol)->Bind(address);
}


status_t
l2cap_unbind(net_protocol* protocol, struct sockaddr* address)
{
	return ((L2capEndpoint*)protocol)->Unbind();
}


status_t
l2cap_listen(net_protocol* protocol, int count)
{
	return ((L2capEndpoint*)protocol)->Listen(count);
}


status_t
l2cap_shutdown(net_protocol* protocol, int direction)
{
	if (direction != SHUT_RDWR)
		return EOPNOTSUPP;
	return ((L2capEndpoint*)protocol)->Shutdown();
}


status_t
l2cap_send_data(net_protocol* protocol, net_buffer* buffer)
{
	return ((L2capEndpoint*)protocol)->SendData(buffer);
}


status_t
l2cap_send_routed_data(net_protocol* protocol, struct net_route* route,
	net_buffer* buffer)
{
	CALLED();
	return EOPNOTSUPP;
}


ssize_t
l2cap_send_avail(net_protocol* protocol)
{
	return ((L2capEndpoint*)protocol)->Sendable();
}


status_t
l2cap_read_data(net_protocol* protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	return ((L2capEndpoint*)protocol)->ReadData(numBytes, flags, _buffer);
}


ssize_t
l2cap_read_avail(net_protocol* protocol)
{
	return ((L2capEndpoint*)protocol)->Receivable();
}


struct net_domain*
l2cap_get_domain(net_protocol* protocol)
{
	return sDomain;
}


size_t
l2cap_get_mtu(net_protocol* protocol, const struct sockaddr* address)
{
	CALLED();
	return protocol->next->module->get_mtu(protocol->next, address);
}


static HciConnection*
connection_for(net_buffer* buffer)
{
	const sockaddr_l2cap* l2capAddr = (sockaddr_l2cap*)buffer->source;
	const sockaddr_dl* interfaceAddr = (sockaddr_dl*)buffer->interface_address->local;
	struct HciConnection* connection = btCoreData->ConnectionByDestination(
		l2capAddr->l2cap_bdaddr, interfaceAddr->sdl_index);
	buffer->interface_address = NULL;
		// This isn't a real interface_address; it could confuse the buffer module.
		// FIXME: We probably should have an alternate interface for passing along data.
	return connection;
}


status_t
l2cap_receive_data(net_buffer* buffer)
{
	if (buffer->size < sizeof(l2cap_basic_header)) {
		ERROR("%s: invalid L2CAP packet: too small. len=%" B_PRIu32 "\n",
			__func__, buffer->size);
		gBufferModule->free(buffer);
		return EMSGSIZE;
	}

	NetBufferHeaderReader<l2cap_basic_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return ENOBUFS;

	uint16 length = le16toh(bufferHeader->length);
	uint16 dcid = le16toh(bufferHeader->dcid);

	TRACE("%s: len=%d cid=%x\n", __func__, length, dcid);

	bufferHeader.Remove();

	if (length != buffer->size) {
		ERROR("l2cap: payload length mismatch, packetlen=%d, bufferlen=%" B_PRIu32 "\n",
			length, buffer->size);
		return EMSGSIZE;
	}

	status_t status = B_ERROR;
	switch (dcid) {
		case L2CAP_SIGNALING_CID:
		{
			// We need to find the connection this packet is associated with.
			struct HciConnection* connection = connection_for(buffer);
			if (connection == NULL) {
				panic("no connection for received L2CAP command");
				return ENOTCONN;
			}

			status = l2cap_handle_signaling_command(connection, buffer);
			break;
		}

		case L2CAP_CONNECTIONLESS_CID:
		{
			NetBufferHeaderReader<l2cap_connectionless_header> connlessHeader(buffer);
			const uint16 psm = le16toh(connlessHeader->psm);
			L2capEndpoint* endpoint = gL2capEndpointManager.ForPSM(psm);
			if (endpoint == NULL)
				return ECONNRESET;

			connlessHeader.Remove();
			buffer->interface_address = NULL;

			status = endpoint->ReceiveData(buffer);
			break;
		}

		default:
		{
			L2capEndpoint* endpoint = gL2capEndpointManager.ForChannel(dcid);
			if (endpoint == NULL)
				return ECONNRESET;

			buffer->interface_address = NULL;
			status = endpoint->ReceiveData(buffer);
			break;
		}
	}

	return status;
}


status_t
l2cap_error_received(net_error error, net_error_data* errorData, net_buffer* data)
{
	CALLED();

	if (error == B_NET_ERROR_UNREACH_HOST) {
		struct HciConnection* connection = connection_for(data);
		if (connection == NULL)
			return ENOTCONN;

		// Disconnect all connections with this HciConnection.
		gL2capEndpointManager.Disconnected(connection);

		gBufferModule->free(data);
		return B_OK;
	}

	return B_ERROR;
}


status_t
l2cap_error_reply(net_protocol* protocol, net_buffer* cause, net_error error,
	net_error_data* errorData)
{
	CALLED();
	return B_ERROR;
}


// #pragma mark -


static status_t
l2cap_std_ops(int32 op, ...)
{
	status_t error;

	CALLED();

	switch (op) {
		case B_MODULE_INIT:
		{
			new(&gL2capEndpointManager) L2capEndpointManager;

			error = gStackModule->register_domain_protocols(AF_BLUETOOTH,
				SOCK_STREAM, BLUETOOTH_PROTO_L2CAP,
				NET_BLUETOOTH_L2CAP_NAME, NULL);
			if (error != B_OK)
				return error;

			error = gStackModule->register_domain(AF_BLUETOOTH, "l2cap",
				&gL2CAPModule, &gL2capAddressModule, &sDomain);
			if (error != B_OK)
				return error;

			return B_OK;
		}

		case B_MODULE_UNINIT:
			gL2capEndpointManager.~L2capEndpointManager();
			gStackModule->unregister_domain(sDomain);
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_protocol_module_info gL2CAPModule = {
	{
		NET_BLUETOOTH_L2CAP_NAME,
		0,
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
	{NET_SOCKET_MODULE_NAME, (module_info**)&gSocketModule},
	{BT_CORE_DATA_MODULE_NAME, (module_info**)&btCoreData},
	{BT_HCI_MODULE_NAME, (module_info**)&btDevices},
	{}
};

module_info* modules[] = {
	(module_info*)&gL2CAPModule,
	NULL
};

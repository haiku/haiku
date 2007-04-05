/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "datalink.h"
#include "domains.h"
#include "interfaces.h"
#include "link.h"
#include "stack_private.h"
#include "utility.h"

#include <net_device.h>

#include <KernelExport.h>

#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>


struct link_protocol : net_protocol {
	net_fifo	fifo;
	char		registered_interface[IF_NAMESIZE];
	bool		registered_monitor;
};


struct net_domain *sDomain;


static status_t
link_monitor_data(void *cookie, net_buffer *packet)
{
	link_protocol *protocol = (link_protocol *)cookie;

	return fifo_socket_enqueue_buffer(&protocol->fifo, protocol->socket,
						B_SELECT_READ, packet);
}


//	#pragma mark -


net_protocol *
link_init_protocol(net_socket *socket)
{
	link_protocol *protocol = new (std::nothrow) link_protocol;
	if (protocol == NULL)
		return NULL;

	if (init_fifo(&protocol->fifo, "packet monitor socket", 65536) < B_OK) {
		delete protocol;
		return NULL;
	}

	protocol->registered_monitor = false;
	return protocol;
}


status_t
link_uninit_protocol(net_protocol *_protocol)
{
	link_protocol *protocol = (link_protocol *)_protocol;

	if (protocol->registered_monitor) {
		net_device_interface *interface = get_device_interface(protocol->registered_interface);
		if (interface != NULL) {
			unregister_device_monitor(interface->device, link_monitor_data, protocol);
			put_device_interface(interface);
		}
	}

	uninit_fifo(&protocol->fifo);
	delete protocol;
	return B_OK;
}


status_t
link_open(net_protocol *protocol)
{
	return B_OK;
}


status_t
link_close(net_protocol *protocol)
{
	return B_OK;
}


status_t
link_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
link_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return EOPNOTSUPP;
}


status_t
link_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
link_control(net_protocol *_protocol, int level, int option, void *value,
	size_t *_length)
{
	link_protocol *protocol = (link_protocol *)_protocol;

	switch (option) {
		case SIOCGIFINDEX:
		{
			// get index of interface
			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) < B_OK)
				return B_BAD_ADDRESS;

			net_device_interface *interface = get_device_interface(request.ifr_name);
			if (interface != NULL) {
				request.ifr_index = interface->device->index;
				put_device_interface(interface);
			} else
				request.ifr_index = 0;

			return user_memcpy(value, &request, sizeof(struct ifreq));
		}
		case SIOCGIFNAME:
		{
			// get name of interface via index
			struct ifreq request;
			if (user_memcpy(&request, value, sizeof(struct ifreq)) < B_OK)
				return B_BAD_ADDRESS;

			net_device_interface *interface = get_device_interface(request.ifr_index);
			if (interface != NULL) {
				strlcpy(request.ifr_name, interface->name, IF_NAMESIZE);
				put_device_interface(interface);
			} else
				return ENODEV;

			return user_memcpy(value, &request, sizeof(struct ifreq));
		}

		case SIOCGIFCOUNT:
		{
			// count number of interfaces
			struct ifconf config;
			config.ifc_value = count_device_interfaces();

			return user_memcpy(value, &config, sizeof(struct ifconf));
		}

		case SIOCGIFCONF:
		{
			// retrieve available interfaces
			struct ifconf config;
			if (user_memcpy(&config, value, sizeof(struct ifconf)) < B_OK)
				return B_BAD_ADDRESS;

			status_t result = list_device_interfaces(config.ifc_buf,
				(size_t *)&config.ifc_len);
			if (result != B_OK)
				return result;

			return user_memcpy(value, &config, sizeof(struct ifconf));
		}

		case SIOCGIFADDR:
		{
			// get address of interface
			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) < B_OK)
				return B_BAD_ADDRESS;

			net_device_interface *interface = get_device_interface(request.ifr_name);
			if (interface != NULL) {
				get_device_interface_address(interface, &request.ifr_addr);
				put_device_interface(interface);
			} else
				return ENODEV;

			return user_memcpy(&((struct ifreq *)value)->ifr_addr,
				&request.ifr_addr, request.ifr_addr.sa_len);
		}

		case SIOCSPACKETCAP:
		{
			// start packet monitoring

			if (protocol->registered_monitor)
				return B_BUSY;

			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) < B_OK)
				return B_BAD_ADDRESS;

			net_device_interface *interface = get_device_interface(request.ifr_name);
			status_t status;
			if (interface != NULL) {
				status = register_device_monitor(interface->device,
					link_monitor_data, protocol);
				if (status == B_OK) {
					// we're now registered
					strlcpy(protocol->registered_interface, request.ifr_name, IF_NAMESIZE);
					protocol->registered_monitor = true;
				}
				put_device_interface(interface);
			} else
				status = ENODEV;

			return status;
		}

		case SIOCCPACKETCAP:
		{
			// stop packet monitoring

			if (!protocol->registered_monitor)
				return B_BAD_VALUE;

			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) < B_OK)
				return B_BAD_ADDRESS;

			net_device_interface *interface = get_device_interface(request.ifr_name);
			status_t status;
			if (interface != NULL) {
				status = unregister_device_monitor(interface->device,
					link_monitor_data, protocol);
				if (status == B_OK) {
					// we're now no longer registered
					protocol->registered_monitor = false;
				}
				put_device_interface(interface);
			} else
				status = ENODEV;

			return status;
		}
	}

	return datalink_control(sDomain, option, value, _length);
}


status_t
link_bind(net_protocol *protocol, struct sockaddr *address)
{
	// TODO: bind to a specific interface and ethernet type
	return B_ERROR;
}


status_t
link_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
}


status_t
link_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
link_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
link_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return B_NOT_ALLOWED;
}


status_t
link_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return B_NOT_ALLOWED;
}


ssize_t
link_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
link_read_data(net_protocol *_protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	link_protocol *protocol = (link_protocol *)_protocol;

	dprintf("link_read is waiting for data...\n");

	net_buffer *buffer;
	status_t status = fifo_dequeue_buffer(&protocol->fifo,
		flags, protocol->socket->receive.timeout, &buffer);
	if (status < B_OK)
		return status;

	*_buffer = buffer;
	return B_OK;
}


ssize_t
link_read_avail(net_protocol *_protocol)
{
	link_protocol *protocol = (link_protocol *)_protocol;
	return protocol->fifo.current_bytes;
}


struct net_domain *
link_get_domain(net_protocol *protocol)
{
	return sDomain;
}


size_t
link_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	// TODO: for now
	return 0;
}


status_t
link_receive_data(net_buffer *buffer)
{
	return B_ERROR;
}


status_t
link_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
link_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


static status_t
link_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return register_domain(AF_LINK, "link", NULL, NULL, &sDomain);

		case B_MODULE_UNINIT:
			unregister_domain(sDomain);
			return B_OK;

		default:
			return B_ERROR;
	}
}


//	#pragma mark -


void
link_init()
{
	register_domain_protocols(AF_LINK, SOCK_DGRAM, 0, "network/stack/link/v1", NULL);

	register_domain_datalink_protocols(AF_LINK, IFT_ETHER,
		"network/datalink_protocols/ethernet_frame/v1",
		NULL);
}


net_protocol_module_info gLinkModule = {
	{
		"network/stack/link/v1",
		0,
		link_std_ops
	},
	link_init_protocol,
	link_uninit_protocol,
	link_open,
	link_close,
	link_free,
	link_connect,
	link_accept,
	link_control,
	link_bind,
	link_unbind,
	link_listen,
	link_shutdown,
	link_send_data,
	link_send_routed_data,
	link_send_avail,
	link_read_data,
	link_read_avail,
	link_get_domain,
	link_get_mtu,
	link_receive_data,
	link_error,
	link_error_reply,
};

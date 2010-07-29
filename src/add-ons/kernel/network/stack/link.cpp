/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


//! The net_protocol one talks to when using the AF_LINK protocol


#include "datalink.h"
#include "device_interfaces.h"
#include "domains.h"
#include "link.h"
#include "stack_private.h"
#include "utility.h"

#include <net_device.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>
#include <ProtocolUtilities.h>

#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>


class LocalStackBundle {
public:
	static net_stack_module_info* Stack() { return &gNetStackModule; }
	static net_buffer_module_info* Buffer() { return &gNetBufferModule; }
};

typedef DatagramSocket<MutexLocking, LocalStackBundle> LocalDatagramSocket;

class LinkProtocol : public net_protocol, public LocalDatagramSocket {
public:
								LinkProtocol(net_socket* socket);
	virtual						~LinkProtocol();

			status_t			StartMonitoring(const char* deviceName);
			status_t			StopMonitoring();

protected:
			status_t			SocketStatus(bool peek) const;

private:
			status_t			_Unregister();

	static	status_t			_MonitorData(net_device_monitor* monitor,
									net_buffer* buffer);
	static	void				_MonitorEvent(net_device_monitor* monitor,
									int32 event);

private:
			net_device_monitor	fMonitor;
			net_device_interface* fMonitoredDevice;
};


struct net_domain* sDomain;


LinkProtocol::LinkProtocol(net_socket* socket)
	: LocalDatagramSocket("packet capture", socket)
{
	fMonitor.cookie = this;
	fMonitor.receive = _MonitorData;
	fMonitor.event = _MonitorEvent;
	fMonitoredDevice = NULL;
}


LinkProtocol::~LinkProtocol()
{
	if (fMonitoredDevice) {
		unregister_device_monitor(fMonitoredDevice->device, &fMonitor);
		put_device_interface(fMonitoredDevice);
	}
}


status_t
LinkProtocol::StartMonitoring(const char* deviceName)
{
	MutexLocker _(fLock);

	if (fMonitoredDevice)
		return B_BUSY;

	net_device_interface* interface = get_device_interface(deviceName);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	status_t status = register_device_monitor(interface->device, &fMonitor);
	if (status < B_OK) {
		put_device_interface(interface);
		return status;
	}

	fMonitoredDevice = interface;
	return B_OK;
}


status_t
LinkProtocol::StopMonitoring()
{
	MutexLocker _(fLock);

	// TODO compare our device with the supplied device name?
	return _Unregister();
}


status_t
LinkProtocol::SocketStatus(bool peek) const
{
	if (fMonitoredDevice == NULL)
		return B_DEVICE_NOT_FOUND;

	return LocalDatagramSocket::SocketStatus(peek);
}


status_t
LinkProtocol::_Unregister()
{
	if (fMonitoredDevice == NULL)
		return B_BAD_VALUE;

	status_t status = unregister_device_monitor(fMonitoredDevice->device,
		&fMonitor);
	put_device_interface(fMonitoredDevice);
	fMonitoredDevice = NULL;

	return status;
}


status_t
LinkProtocol::_MonitorData(net_device_monitor* monitor, net_buffer* packet)
{
	return ((LinkProtocol*)monitor->cookie)->SocketEnqueue(packet);
}


void
LinkProtocol::_MonitorEvent(net_device_monitor* monitor, int32 event)
{
	LinkProtocol* protocol = (LinkProtocol*)monitor->cookie;

	if (event == B_DEVICE_GOING_DOWN) {
		MutexLocker _(protocol->fLock);

		protocol->_Unregister();
		if (protocol->IsEmpty()) {
			protocol->WakeAll();
			notify_socket(protocol->socket, B_SELECT_READ, B_DEVICE_NOT_FOUND);
		}
	}
}


//	#pragma mark -


static bool
user_request_get_device_interface(void* value, struct ifreq& request,
	net_device_interface*& interface)
{
	if (user_memcpy(&request, value, IF_NAMESIZE) < B_OK)
		return false;

	interface = get_device_interface(request.ifr_name);
	return true;
}


//	#pragma mark -


net_protocol* 
link_init_protocol(net_socket* socket)
{
	LinkProtocol* protocol = new (std::nothrow) LinkProtocol(socket);
	if (protocol != NULL && protocol->InitCheck() < B_OK) {
		delete protocol;
		return NULL;
	}

	return protocol;
}


status_t
link_uninit_protocol(net_protocol* protocol)
{
	delete (LinkProtocol*)protocol;
	return B_OK;
}


status_t
link_open(net_protocol* protocol)
{
	return B_OK;
}


status_t
link_close(net_protocol* protocol)
{
	return B_OK;
}


status_t
link_free(net_protocol* protocol)
{
	return B_OK;
}


status_t
link_connect(net_protocol* protocol, const struct sockaddr* address)
{
	return EOPNOTSUPP;
}


status_t
link_accept(net_protocol* protocol, struct net_socket** _acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
link_control(net_protocol* _protocol, int level, int option, void* value,
	size_t* _length)
{
	LinkProtocol* protocol = (LinkProtocol*)_protocol;

	switch (option) {
		case SIOCGIFINDEX:
		{
			// get index of interface
			net_device_interface* interface;
			struct ifreq request;
			if (!user_request_get_device_interface(value, request, interface))
				return B_BAD_ADDRESS;

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

			net_device_interface* interface
				= get_device_interface(request.ifr_index);
			if (interface == NULL)
				return B_DEVICE_NOT_FOUND;

			strlcpy(request.ifr_name, interface->device->name, IF_NAMESIZE);
			put_device_interface(interface);

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
				(size_t*)&config.ifc_len);
			if (result != B_OK)
				return result;

			return user_memcpy(value, &config, sizeof(struct ifconf));
		}

		case SIOCGIFADDR:
		{
			// get address of interface
			net_device_interface* interface;
			struct ifreq request;
			if (!user_request_get_device_interface(value, request, interface))
				return B_BAD_ADDRESS;

			if (interface == NULL)
				return B_DEVICE_NOT_FOUND;

			get_device_interface_address(interface, &request.ifr_addr);
			put_device_interface(interface);

			return user_memcpy(&((struct ifreq*)value)->ifr_addr,
				&request.ifr_addr, request.ifr_addr.sa_len);
		}

		case SIOCGIFFLAGS:
		{
			// get flags of interface
			net_device_interface* interface;
			struct ifreq request;
			if (!user_request_get_device_interface(value, request, interface))
				return B_BAD_ADDRESS;

			if (interface == NULL)
				return B_DEVICE_NOT_FOUND;

			request.ifr_flags = interface->device->flags;
			put_device_interface(interface);

			return user_memcpy(&((struct ifreq*)value)->ifr_flags,
				&request.ifr_flags, sizeof(request.ifr_flags));
		}

		case SIOCSPACKETCAP:
		{
			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) < B_OK)
				return B_BAD_ADDRESS;

			return protocol->StartMonitoring(request.ifr_name);
		}

		case SIOCCPACKETCAP:
			return protocol->StopMonitoring();
	}

	return datalink_control(sDomain, option, value, _length);
}


status_t
link_getsockopt(net_protocol* protocol, int level, int option, void* value,
	int* length)
{
	if (protocol->next != NULL) {
		return protocol->next->module->getsockopt(protocol, level, option,
			value, length);
	}

	return gNetSocketModule.get_option(protocol->socket, level, option, value,
		length);
}


status_t
link_setsockopt(net_protocol* protocol, int level, int option,
	const void* value, int length)
{
	if (protocol->next != NULL) {
		return protocol->next->module->setsockopt(protocol, level, option,
			value, length);
	}

	return gNetSocketModule.set_option(protocol->socket, level, option,
		value, length);
}


status_t
link_bind(net_protocol* protocol, const struct sockaddr* address)
{
	// TODO: bind to a specific interface and ethernet type
	return B_ERROR;
}


status_t
link_unbind(net_protocol* protocol, struct sockaddr* address)
{
	return B_ERROR;
}


status_t
link_listen(net_protocol* protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
link_shutdown(net_protocol* protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
link_send_data(net_protocol* protocol, net_buffer* buffer)
{
	return B_NOT_ALLOWED;
}


status_t
link_send_routed_data(net_protocol* protocol, struct net_route* route,
	net_buffer* buffer)
{
	return B_NOT_ALLOWED;
}


ssize_t
link_send_avail(net_protocol* protocol)
{
	return B_ERROR;
}


status_t
link_read_data(net_protocol* protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	return ((LinkProtocol*)protocol)->SocketDequeue(flags, _buffer);
}


ssize_t
link_read_avail(net_protocol* protocol)
{
	return ((LinkProtocol*)protocol)->AvailableData();
}


struct net_domain* 
link_get_domain(net_protocol* protocol)
{
	return sDomain;
}


size_t
link_get_mtu(net_protocol* protocol, const struct sockaddr* address)
{
	// TODO: for now
	return 0;
}


status_t
link_receive_data(net_buffer* buffer)
{
	return B_ERROR;
}


status_t
link_error(uint32 code, net_buffer* data)
{
	return B_ERROR;
}


status_t
link_error_reply(net_protocol* protocol, net_buffer* causedError, uint32 code,
	void* errorData)
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
	register_domain_protocols(AF_LINK, SOCK_DGRAM, 0, "network/stack/link/v1",
		NULL);

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
	NET_PROTOCOL_ATOMIC_MESSAGES,

	link_init_protocol,
	link_uninit_protocol,
	link_open,
	link_close,
	link_free,
	link_connect,
	link_accept,
	link_control,
	link_getsockopt,
	link_setsockopt,
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
	NULL,
	link_error,
	link_error_reply,
};

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

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>

#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>


class LinkProtocol : public net_protocol {
public:
	LinkProtocol();
	~LinkProtocol();

	status_t InitCheck() const;

	status_t StartMonitoring(const char *);
	status_t StopMonitoring();

	ssize_t ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer);
	ssize_t ReadAvail() const;

private:
	status_t _Enqueue(net_buffer *buffer);
	status_t _Unregister();

	mutable benaphore	fLock;
	Fifo				fFifo;

	net_device_monitor	fMonitor;
	net_device_interface *fMonitoredDevice;

	static status_t _MonitorData(net_device_monitor *monitor, net_buffer *buffer);
	static void _MonitorEvent(net_device_monitor *monitor, int32 event);
};


struct net_domain *sDomain;


LinkProtocol::LinkProtocol()
	: fFifo("packet monitor fifo", 65536)
{
	benaphore_init(&fLock, "packet monitor lock");

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

	benaphore_destroy(&fLock);
}


status_t
LinkProtocol::InitCheck() const
{
	return fLock.sem >= 0 && fFifo.InitCheck();
}


status_t
LinkProtocol::StartMonitoring(const char *deviceName)
{
	BenaphoreLocker _(fLock);

	if (fMonitoredDevice)
		return B_BUSY;

	net_device_interface *interface = get_device_interface(deviceName);
	if (interface == NULL)
		return ENODEV;

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
	BenaphoreLocker _(fLock);

	// TODO compare our device with the supplied device name?
	return _Unregister();
}


ssize_t
LinkProtocol::ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer)
{
	BenaphoreLocker _(fLock);

	bigtime_t timeout = socket->receive.timeout;

	if (timeout == 0)
		flags |= MSG_DONTWAIT;
	else if (timeout != B_INFINITE_TIMEOUT)
		timeout += system_time();

	while (fFifo.IsEmpty()) {
		if (fMonitoredDevice == NULL)
			return ENODEV;

		status_t status = B_WOULD_BLOCK;
		if ((flags & MSG_DONTWAIT) == 0)
			status = fFifo.Wait(&fLock, timeout);

		if (status < B_OK)
			return status;
	}

	*_buffer = fFifo.Dequeue(flags & MSG_PEEK);
	return *_buffer ? B_OK : B_NO_MEMORY;
}


ssize_t
LinkProtocol::ReadAvail() const
{
	BenaphoreLocker _(fLock);
	if (fMonitoredDevice == NULL)
		return ENODEV;
	return fFifo.current_bytes;
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
LinkProtocol::_Enqueue(net_buffer *buffer)
{
	BenaphoreLocker _(fLock);
	return fFifo.EnqueueAndNotify(buffer, socket, B_SELECT_READ);
}


status_t
LinkProtocol::_MonitorData(net_device_monitor *monitor, net_buffer *packet)
{
	return ((LinkProtocol *)monitor->cookie)->_Enqueue(packet);
}


void
LinkProtocol::_MonitorEvent(net_device_monitor *monitor, int32 event)
{
	LinkProtocol *protocol = (LinkProtocol *)monitor->cookie;

	if (event == B_DEVICE_GOING_DOWN) {
		BenaphoreLocker _(protocol->fLock);

		protocol->_Unregister();
		if (protocol->fFifo.IsEmpty()) {
			protocol->fFifo.WakeAll();
			notify_socket(protocol->socket, B_SELECT_READ, ENODEV);
		}
	}
}


//	#pragma mark -


net_protocol *
link_init_protocol(net_socket *socket)
{
	LinkProtocol *protocol = new (std::nothrow) LinkProtocol();
	if (protocol && protocol->InitCheck() < B_OK) {
		delete protocol;
		return NULL;
	}

	return protocol;
}


status_t
link_uninit_protocol(net_protocol *protocol)
{
	delete (LinkProtocol *)protocol;
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
	LinkProtocol *protocol = (LinkProtocol *)_protocol;

	// TODO All of this common functionality should be elsewhere

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
link_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return ((LinkProtocol *)protocol)->ReadData(numBytes, flags, _buffer);
}


ssize_t
link_read_avail(net_protocol *protocol)
{
	return ((LinkProtocol *)protocol)->ReadAvail();
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

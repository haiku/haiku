/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


//! The net_protocol one talks to when using the AF_LINK protocol


#include "link.h"

#include <net/if_dl.h>
#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>

#include <KernelExport.h>

#include <lock.h>
#include <net_datalink.h>
#include <net_device.h>
#include <ProtocolUtilities.h>
#include <util/AutoLock.h>

#include "device_interfaces.h"
#include "domains.h"
#include "interfaces.h"
#include "stack_private.h"
#include "utility.h"


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
			status_t			StopMonitoring(const char* deviceName);

			status_t			Bind(const sockaddr* address);
			status_t			Unbind();
			bool				IsBound() const
									{ return fBoundToDevice != NULL; }

			size_t				MTU();

protected:
			status_t			SocketStatus(bool peek) const;

private:
			status_t			_Unregister();

	static	status_t			_MonitorData(net_device_monitor* monitor,
									net_buffer* buffer);
	static	void				_MonitorEvent(net_device_monitor* monitor,
									int32 event);
	static	status_t			_ReceiveData(void* cookie, net_device* device,
									net_buffer* buffer);

private:
			net_device_monitor	fMonitor;
			net_device_interface* fMonitoredDevice;
			net_device_interface* fBoundToDevice;
			uint32				fBoundType;
};


struct net_domain* sDomain;


LinkProtocol::LinkProtocol(net_socket* socket)
	:
	LocalDatagramSocket("packet capture", socket),
	fMonitoredDevice(NULL),
	fBoundToDevice(NULL)
{
	fMonitor.cookie = this;
	fMonitor.receive = _MonitorData;
	fMonitor.event = _MonitorEvent;
}


LinkProtocol::~LinkProtocol()
{
	if (fMonitoredDevice != NULL) {
		unregister_device_monitor(fMonitoredDevice->device, &fMonitor);
		put_device_interface(fMonitoredDevice);
	} else
		Unbind();
}


status_t
LinkProtocol::StartMonitoring(const char* deviceName)
{
	MutexLocker locker(fLock);

	if (fMonitoredDevice != NULL)
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
LinkProtocol::StopMonitoring(const char* deviceName)
{
	MutexLocker locker(fLock);

	if (fMonitoredDevice == NULL
		|| strcmp(fMonitoredDevice->device->name, deviceName) != 0)
		return B_BAD_VALUE;

	return _Unregister();
}


status_t
LinkProtocol::Bind(const sockaddr* address)
{
	// Only root is allowed to bind to a link layer interface
	if (address == NULL || geteuid() != 0)
		return B_NOT_ALLOWED;

	MutexLocker locker(fLock);

	if (fMonitoredDevice != NULL)
		return B_BUSY;

	Interface* interface = get_interface_for_link(sDomain, address);
	if (interface == NULL)
		return B_BAD_VALUE;

	net_device_interface* boundTo
		= acquire_device_interface(interface->DeviceInterface());

	interface->ReleaseReference();

	if (boundTo == NULL)
		return B_BAD_VALUE;

	sockaddr_dl& linkAddress = *(sockaddr_dl*)address;

	if (linkAddress.sdl_type != 0) {
		fBoundType = B_NET_FRAME_TYPE(linkAddress.sdl_type,
			ntohs(linkAddress.sdl_e_type));
		// Bind to the type requested - this is needed in order to
		// receive any buffers
		// TODO: this could be easily changed by introducing catch all or rule
		// based handlers!
		status_t status = register_device_handler(boundTo->device, fBoundType,
			&LinkProtocol::_ReceiveData, this);
		if (status != B_OK)
			return status;
	} else
		fBoundType = 0;

	fBoundToDevice = boundTo;
	socket->bound_to_device = boundTo->device->index;

	memcpy(&socket->address, address, sizeof(struct sockaddr_storage));
	socket->address.ss_len = sizeof(struct sockaddr_storage);

	return B_OK;
}


status_t
LinkProtocol::Unbind()
{
	MutexLocker locker(fLock);

	if (fBoundToDevice == NULL)
		return B_BAD_VALUE;

	unregister_device_handler(fBoundToDevice->device, fBoundType);
	put_device_interface(fBoundToDevice);

	socket->bound_to_device = 0;
	socket->address.ss_len = 0;
	return B_OK;
}


size_t
LinkProtocol::MTU()
{
	MutexLocker locker(fLock);

	if (!IsBound())
		return 0;

	return fBoundToDevice->device->mtu;
}


status_t
LinkProtocol::SocketStatus(bool peek) const
{
	if (fMonitoredDevice == NULL && !IsBound())
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


/*static*/ status_t
LinkProtocol::_MonitorData(net_device_monitor* monitor, net_buffer* packet)
{
	return ((LinkProtocol*)monitor->cookie)->EnqueueClone(packet);
}


/*static*/ void
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


/*static*/ status_t
LinkProtocol::_ReceiveData(void* cookie, net_device* device, net_buffer* buffer)
{
	LinkProtocol* protocol = (LinkProtocol*)cookie;

	return protocol->Enqueue(buffer);
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


//	#pragma mark - net_protocol module


static net_protocol*
link_init_protocol(net_socket* socket)
{
	LinkProtocol* protocol = new (std::nothrow) LinkProtocol(socket);
	if (protocol != NULL && protocol->InitCheck() < B_OK) {
		delete protocol;
		return NULL;
	}

	return protocol;
}


static status_t
link_uninit_protocol(net_protocol* protocol)
{
	delete (LinkProtocol*)protocol;
	return B_OK;
}


static status_t
link_open(net_protocol* protocol)
{
	return B_OK;
}


static status_t
link_close(net_protocol* protocol)
{
	return B_OK;
}


static status_t
link_free(net_protocol* protocol)
{
	return B_OK;
}


static status_t
link_connect(net_protocol* protocol, const struct sockaddr* address)
{
	return B_NOT_SUPPORTED;
}


static status_t
link_accept(net_protocol* protocol, struct net_socket** _acceptedSocket)
{
	return B_NOT_SUPPORTED;
}


static status_t
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

			sockaddr_storage address;
			get_device_interface_address(interface, (sockaddr*)&address);
			put_device_interface(interface);

			return user_memcpy(&((struct ifreq*)value)->ifr_addr,
				&address, address.ss_len);
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

		case SIOCGIFMEDIA:
		{
			// get media
			if (*_length < sizeof(ifmediareq))
				return B_BAD_VALUE;

			net_device_interface* interface;
			struct ifmediareq request;
			if (!user_request_get_device_interface(value, (ifreq&)request,
					interface))
				return B_BAD_ADDRESS;

			if (interface == NULL)
				return B_DEVICE_NOT_FOUND;

			if (user_memcpy(&request, value, sizeof(ifmediareq)) != B_OK)
				return B_BAD_ADDRESS;

			// TODO: see above.
			if (interface->device->module->control(interface->device,
					SIOCGIFMEDIA, &request,
					sizeof(struct ifmediareq)) != B_OK) {
				memset(&request, 0, sizeof(struct ifmediareq));
				request.ifm_active = request.ifm_current
					= interface->device->media;
			}

			return user_memcpy(value, &request, sizeof(struct ifmediareq));
		}

		case SIOCSPACKETCAP:
		{
			// Only root is allowed to capture packets
			if (geteuid() != 0)
				return B_NOT_ALLOWED;

			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) != B_OK)
				return B_BAD_ADDRESS;

			return protocol->StartMonitoring(request.ifr_name);
		}

		case SIOCCPACKETCAP:
		{
			struct ifreq request;
			if (user_memcpy(&request, value, IF_NAMESIZE) != B_OK)
				return B_BAD_ADDRESS;

			return protocol->StopMonitoring(request.ifr_name);
		}
	}

	return gNetDatalinkModule.control(sDomain, option, value, _length);
}


static status_t
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


static status_t
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


static status_t
link_bind(net_protocol* _protocol, const struct sockaddr* address)
{
	LinkProtocol* protocol = (LinkProtocol*)_protocol;
	return protocol->Bind(address);
}


static status_t
link_unbind(net_protocol* _protocol, struct sockaddr* address)
{
	LinkProtocol* protocol = (LinkProtocol*)_protocol;
	return protocol->Unbind();
}


static status_t
link_listen(net_protocol* protocol, int count)
{
	return B_NOT_SUPPORTED;
}


static status_t
link_shutdown(net_protocol* protocol, int direction)
{
	return B_NOT_SUPPORTED;
}


static status_t
link_send_data(net_protocol* protocol, net_buffer* buffer)
{
	return gNetDatalinkModule.send_data(protocol, sDomain, buffer);
}


static status_t
link_send_routed_data(net_protocol* protocol, struct net_route* route,
	net_buffer* buffer)
{
	if (buffer->destination->sa_family != buffer->source->sa_family
		|| buffer->destination->sa_family != AF_LINK)
		return B_BAD_VALUE;

	// The datalink layer will take care of the framing

	return gNetDatalinkModule.send_routed_data(route, buffer);
}


static ssize_t
link_send_avail(net_protocol* _protocol)
{
	LinkProtocol* protocol = (LinkProtocol*)_protocol;
	if (!protocol->IsBound())
		return B_ERROR;

	return protocol->socket->send.buffer_size;
}


static status_t
link_read_data(net_protocol* protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	return ((LinkProtocol*)protocol)->Dequeue(flags, _buffer);
}


static ssize_t
link_read_avail(net_protocol* protocol)
{
	return ((LinkProtocol*)protocol)->AvailableData();
}


static struct net_domain*
link_get_domain(net_protocol* protocol)
{
	return sDomain;
}


static size_t
link_get_mtu(net_protocol* _protocol, const struct sockaddr* address)
{
	LinkProtocol* protocol = (LinkProtocol*)_protocol;
	return protocol->MTU();
}


static status_t
link_receive_data(net_buffer* buffer)
{
	// We never receive any data this way
	return B_ERROR;
}


static status_t
link_error_received(net_error error, net_buffer* data)
{
	// We don't do any error processing
	return B_ERROR;
}


static status_t
link_error_reply(net_protocol* protocol, net_buffer* cause, net_error error,
	net_error_data* errorData)
{
	// We don't do any error processing
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

	// TODO: this should actually be registered for all types (besides local)
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
	NULL,		// deliver_data
	link_error_received,
	link_error_reply,
	NULL,		// add_ancillary_data()
	NULL,		// process_ancillary_data()
	NULL,		// process_ancillary_data_no_container()
	NULL,		// send_data_no_buffer()
	NULL		// read_data_no_buffer()
};

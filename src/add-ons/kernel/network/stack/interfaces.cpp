/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "domains.h"
#include "interfaces.h"
#include "stack_private.h"
#include "utility.h"

#include <net_device.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>

#include <net/if_dl.h>
#include <new>
#include <stdlib.h>
#include <string.h>


#define TRACE_INTERFACES
#ifdef TRACE_INTERFACES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static benaphore sInterfaceLock;
static DeviceInterfaceList sInterfaces;
static uint32 sInterfaceIndex;
static uint32 sDeviceIndex;


static net_device_interface *
find_device_interface(const char *name)
{
	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();

	while (iterator.HasNext()) {
		net_device_interface *interface = iterator.Next();

		if (!strcmp(interface->name, name))
			return interface;
	}

	return NULL;
}


static status_t
domain_receive_adapter(void *cookie, net_buffer *buffer)
{
	net_domain_private *domain = (net_domain_private *)cookie;
	return domain->module->receive_data(buffer);
}


net_device_interface *
grab_device_interface(net_device_interface *interface)
{
	if (interface == NULL || atomic_add(&interface->ref_count, 1) == 0)
		return NULL;

	return interface;
}


//	#pragma mark - interfaces


/*!
	Searches for a specific interface in a domain by name.
	You need to have the domain's lock hold when calling this function.
*/
struct net_interface_private *
find_interface(struct net_domain *domain, const char *name)
{
	net_interface_private *interface = NULL;

	while (true) {
		interface = (net_interface_private *)list_get_next_item(
			&domain->interfaces, interface);
		if (interface == NULL)
			break;

		if (!strcmp(interface->name, name))
			return interface;
	}

	return NULL;
}


/*!
	Searches for a specific interface in a domain by index.
	You need to have the domain's lock hold when calling this function.
*/
struct net_interface_private *
find_interface(struct net_domain *domain, uint32 index)
{
	net_interface_private *interface = NULL;

	while (true) {
		interface = (net_interface_private *)list_get_next_item(
			&domain->interfaces, interface);
		if (interface == NULL)
			break;

		if (interface->index == index)
			return interface;
	}

	return NULL;
}


status_t
create_interface(net_domain *domain, const char *name, const char *baseName,
	net_device_interface *deviceInterface, net_interface_private **_interface)
{
	net_interface_private *interface =
		new (std::nothrow) net_interface_private;
	if (interface == NULL)
		return B_NO_MEMORY;

	strlcpy(interface->name, name, IF_NAMESIZE);
	strlcpy(interface->base_name, baseName, IF_NAMESIZE);
	interface->domain = domain;
	interface->device = deviceInterface->device;

	interface->address = NULL;
	interface->destination = NULL;
	interface->mask = NULL;

	interface->index = ++sInterfaceIndex;
	interface->flags = deviceInterface->device->flags & ~IFF_UP;
	interface->type = 0;
	interface->mtu = deviceInterface->device->mtu;
	interface->metric = 0;
	interface->device_interface = grab_device_interface(deviceInterface);

	status_t status = get_domain_datalink_protocols(interface);
	if (status < B_OK) {
		delete interface;
		return status;
	}

	// Grab a reference to the networking stack, to make sure it won't be
	// unloaded as long as an interface exists
	module_info *module;
	get_module(NET_STARTER_MODULE_NAME, &module);

	*_interface = interface;
	return B_OK;
}


void
interface_set_down(net_interface *interface)
{
	if ((interface->flags & IFF_UP) == 0)
		return;

	// TODO: IFF_LINK should belong in device only
	interface->flags &= ~(IFF_UP | IFF_LINK);
	interface->first_info->interface_down(interface->first_protocol);
}


void
delete_interface(net_interface_private *interface)
{
	// deleting an interface is fairly complex as we need
	// to clear all references to it throughout the stack

	// this will possibly call (if IFF_UP):
	//  interface_protocol_down()
	//   domain_interface_went_down()
	//    invalidate_routes()
	//     remove_route()
	//      update_route_infos()
	//       get_route_internal()
	//   down_device_interface() -- if upcount reaches 0
	interface_set_down(interface);

	put_device_interface(interface->device_interface);

	free(interface->address);
	free(interface->destination);
	free(interface->mask);

	delete interface;

	// Release reference of the stack - at this point, our stack may be unloaded
	// if no other interfaces or sockets are left
	put_module(NET_STARTER_MODULE_NAME);
}


void
put_interface(struct net_interface_private *interface)
{
	// TODO: reference counting
	// TODO: better locking scheme
	benaphore_unlock(&((net_domain_private *)interface->domain)->lock);
}


struct net_interface_private *
get_interface(net_domain *_domain, const char *name)
{
	net_domain_private *domain = (net_domain_private *)_domain;
	benaphore_lock(&domain->lock);

	net_interface_private *interface = NULL;
	while (true) {
		interface = (net_interface_private *)list_get_next_item(
			&domain->interfaces, interface);
		if (interface == NULL)
			break;

		if (!strcmp(interface->name, name))
			return interface;
	}

	benaphore_unlock(&domain->lock);
	return NULL;
}


//	#pragma mark - device interfaces


void
get_device_interface_address(net_device_interface *interface, sockaddr *_address)
{
	sockaddr_dl &address = *(sockaddr_dl *)_address;

	address.sdl_family = AF_LINK;
	address.sdl_index = interface->device->index;
	address.sdl_type = interface->device->type;
	address.sdl_nlen = strlen(interface->name);
	address.sdl_slen = 0;
	memcpy(address.sdl_data, interface->name, address.sdl_nlen);

	address.sdl_alen = interface->device->address.length;
	memcpy(LLADDR(&address), interface->device->address.data, address.sdl_alen);

	address.sdl_len = sizeof(sockaddr_dl) - sizeof(address.sdl_data)
		+ address.sdl_nlen + address.sdl_alen;
}


uint32
count_device_interfaces()
{
	BenaphoreLocker locker(sInterfaceLock);

	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();
	uint32 count = 0;

	while (iterator.HasNext()) {
		iterator.Next();
		count++;
	}

	return count;
}


/*!
	Dumps a list of all interfaces into the supplied userland buffer.
	If the interfaces don't fit into the buffer, an error (\c ENOBUFS) is
	returned.
*/
status_t
list_device_interfaces(void *_buffer, size_t *bufferSize)
{
	BenaphoreLocker locker(sInterfaceLock);

	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();
	UserBuffer buffer(_buffer, *bufferSize);

	while (iterator.HasNext()) {
		net_device_interface *interface = iterator.Next();

		ifreq request;
		strlcpy(request.ifr_name, interface->name, IF_NAMESIZE);
		get_device_interface_address(interface, &request.ifr_addr);

		if (buffer.Copy(&request, IF_NAMESIZE
				+ request.ifr_addr.sa_len) == NULL)
			return buffer.Status();
	}

	*bufferSize = buffer.ConsumedAmount();
	return B_OK;
}


/*!
	Releases the reference for the interface. When all references are
	released, the interface is removed.
*/
void
put_device_interface(struct net_device_interface *interface)
{
	if (atomic_add(&interface->ref_count, -1) != 1)
		return;

	// we need to remove this interface!

	{
		BenaphoreLocker locker(sInterfaceLock);
		sInterfaces.Remove(interface);
	}

	interface->module->uninit_device(interface->device);
	put_module(interface->module->info.name);
}


/*!
	Finds an interface by the specified index and grabs a reference to it.
*/
struct net_device_interface *
get_device_interface(uint32 index)
{
	BenaphoreLocker locker(sInterfaceLock);
	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();

	while (iterator.HasNext()) {
		net_device_interface *interface = iterator.Next();

		if (interface->device->index == index) {
			if (atomic_add(&interface->ref_count, 1) != 0)
				return interface;
		}
	}

	return NULL;
}


/*!
	Finds an interface by the specified name and grabs a reference to it.
	If the interface does not yet exist, a new one is created.
*/
struct net_device_interface *
get_device_interface(const char *name)
{
	BenaphoreLocker locker(sInterfaceLock);

	net_device_interface *interface = find_device_interface(name);
	if (interface != NULL) {
		if (atomic_add(&interface->ref_count, 1) != 0)
			return interface;

		// try to recreate interface - it just got removed
	}

	void *cookie = open_module_list("network/devices");
	if (cookie == NULL)
		return NULL;

	while (true) {
		char moduleName[B_FILE_NAME_LENGTH];
		size_t length = sizeof(moduleName);
		if (read_next_module_name(cookie, moduleName, &length) != B_OK)
			break;

		TRACE(("get_device_interface: ask \"%s\" for %s\n", moduleName, name));

		net_device_module_info *module;
		if (get_module(moduleName, (module_info **)&module) == B_OK) {
			net_device *device;
			status_t status = module->init_device(name, &device);
			if (status == B_OK) {
				// create new module interface for this
				interface = new (std::nothrow) net_device_interface;
				if (interface != NULL) {
					interface->name = device->name;
					interface->module = module;
					interface->device = device;
					interface->up_count = 0;
					interface->ref_count = 1;
					interface->deframe_func = NULL;
					interface->deframe_ref_count = 0;

					device->index = ++sDeviceIndex;
					device->module = module;

					sInterfaces.Add(interface);
					return interface;
				} else
					module->uninit_device(device);
			}

			put_module(moduleName);
		}
	}

	return NULL;
}


void
down_device_interface(net_device_interface *interface)
{
	net_device *device = interface->device;

	dprintf("down_device_interface(%s)\n", interface->name);

	device->flags &= ~IFF_UP;
	interface->module->down(device);

	// TODO: there is a race condition between the previous
	//       ->down and device->module->receive_data which
	//       locks us here waiting for the reader_thread

	// make sure the reader thread is gone before shutting down the interface
	status_t status;
	wait_for_thread(interface->reader_thread, &status);
}


//	#pragma mark - devices


/*!
	Unregisters a previously registered deframer function.
	This function is part of the net_manager_module_info API.
*/
status_t
unregister_device_deframer(net_device *device)
{
	BenaphoreLocker locker(sInterfaceLock);

	// find device interface for this device
	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	if (--interface->deframe_ref_count == 0)
		interface->deframe_func = NULL;

	return B_OK;
}


/*!
	Registers the deframer function for the specified \a device.
	Note, however, that right now, you can only register one single
	deframer function per device.

	If the need arises, we might want to lift that limitation at a
	later time (which would require a slight API change, though).

	This function is part of the net_manager_module_info API.
*/
status_t
register_device_deframer(net_device *device, net_deframe_func deframeFunc)
{
	BenaphoreLocker locker(sInterfaceLock);

	// find device interface for this device
	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	if (interface->deframe_func != NULL && interface->deframe_func != deframeFunc)
		return B_ERROR;

	interface->deframe_func = deframeFunc;
	interface->deframe_ref_count++;
	return B_OK;
}


status_t
register_domain_device_handler(struct net_device *device, int32 type,
	struct net_domain *_domain)
{
	net_domain_private *domain = (net_domain_private *)_domain;
	if (domain->module == NULL || domain->module->receive_data == NULL)
		return B_BAD_VALUE;

	return register_device_handler(device, type, &domain_receive_adapter, domain);
}


status_t
register_device_handler(struct net_device *device, int32 type,
	net_receive_func receiveFunc, void *cookie)
{
	BenaphoreLocker locker(sInterfaceLock);

	// find device interface for this device
	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	// see if such a handler already for this device

	DeviceHandlerList::Iterator iterator = interface->receive_funcs.GetIterator();
	while (iterator.HasNext()) {
		net_device_handler *handler = iterator.Next();

		if (handler->type == type)
			return B_ERROR;
	}

	// Add new handler

	net_device_handler *handler = new (std::nothrow) net_device_handler;
	if (handler == NULL)
		return B_NO_MEMORY;

	handler->func = receiveFunc;
	handler->type = type;
	handler->cookie = cookie;
	interface->receive_funcs.Add(handler);
	return B_OK;
}


status_t
unregister_device_handler(struct net_device *device, int32 type)
{
	BenaphoreLocker locker(sInterfaceLock);

	// find device interface for this device
	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	// search for the handler

	DeviceHandlerList::Iterator iterator = interface->receive_funcs.GetIterator();
	while (iterator.HasNext()) {
		net_device_handler *handler = iterator.Next();

		if (handler->type == type) {
			// found it
			iterator.Remove();
			delete handler;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


status_t
register_device_monitor(struct net_device *device,
	net_receive_func receiveFunc, void *cookie)
{
	BenaphoreLocker locker(sInterfaceLock);

	// find device interface for this device
	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	// Add new monitor

	net_device_monitor *monitor = new (std::nothrow) net_device_monitor;
	if (monitor == NULL)
		return B_NO_MEMORY;

	monitor->func = receiveFunc;
	monitor->cookie = cookie;
	interface->monitor_funcs.Add(monitor);
	return B_OK;
}


status_t
unregister_device_monitor(struct net_device *device,
	net_receive_func receiveFunc, void *cookie)
{
	BenaphoreLocker locker(sInterfaceLock);

	// find device interface for this device
	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	// search for the monitor

	DeviceMonitorList::Iterator iterator = interface->monitor_funcs.GetIterator();
	while (iterator.HasNext()) {
		net_device_monitor *monitor = iterator.Next();

		if (monitor->cookie == cookie && monitor->func == receiveFunc) {
			// found it
			iterator.Remove();
			delete monitor;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


/*!
	This function is called by device modules in case their link
	state changed, ie. if an ethernet cable was plugged in or
	removed.
*/
status_t
device_link_changed(net_device *device)
{
	domain_interfaces_link_changed(device);
	return B_OK;
}


/*!
	This function is called by device modules once their device got
	physically removed, ie. a USB networking card is unplugged.
	It is part of the net_manager_module_info API.
*/
status_t
device_removed(net_device *device)
{
	BenaphoreLocker locker(sInterfaceLock);

	net_device_interface *interface = find_device_interface(device->name);
	if (interface == NULL)
		return ENODEV;

	// Propagate the loss of the device throughout the stack.
	// This is very complex, refer to delete_interface() for
	// further details.

	// this will possibly call:
	//  remove_interface_from_domain() [domain gets locked]
	//   delete_interface()
	//    ... [see delete_interface()]
	domain_removed_device_interface(interface);

	// TODO: make sure all readers are gone
	//       make sure all watchers are gone
	//       remove device interface

	return B_OK;
}


//	#pragma mark -


status_t
init_interfaces()
{
	if (benaphore_init(&sInterfaceLock, "net interfaces") < B_OK)
		return B_ERROR;

	new (&sInterfaces) DeviceInterfaceList;
		// static C++ objects are not initialized in the module startup
	return B_OK;
}


status_t
uninit_interfaces()
{
	benaphore_destroy(&sInterfaceLock);
	return B_OK;
}


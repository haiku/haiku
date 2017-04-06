/*
 * Copyright 2006-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "device_interfaces.h"
#include "domains.h"
#include "interfaces.h"
#include "stack_private.h"
#include "utility.h"

#include <net_device.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>

#include <net/if_dl.h>
#include <netinet/in.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_DEVICE_INTERFACES
#ifdef TRACE_DEVICE_INTERFACES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static mutex sLock;
static DeviceInterfaceList sInterfaces;
static uint32 sDeviceIndex;


/*!	A service thread for each device interface. It just reads as many packets
	as availabe, deframes them, and puts them into the receive queue of the
	device interface.
*/
static status_t
device_reader_thread(void* _interface)
{
	net_device_interface* interface = (net_device_interface*)_interface;
	net_device* device = interface->device;
	status_t status = B_OK;

	while ((device->flags & IFF_UP) != 0) {
		net_buffer* buffer;
		status = device->module->receive_data(device, &buffer);
		if (status == B_OK) {
			// feed device monitors
			if (atomic_get(&interface->monitor_count) > 0)
				device_interface_monitor_receive(interface, buffer);

			ASSERT(buffer->interface_address == NULL);

			if (interface->deframe_func(interface->device, buffer) != B_OK) {
				gNetBufferModule.free(buffer);
				continue;
			}

			fifo_enqueue_buffer(&interface->receive_queue, buffer);
		} else if (status == B_DEVICE_NOT_FOUND) {
				device_removed(device);
		} else {
			// In case of error, give the other threads some
			// time to run since this is a high priority time thread.
			snooze(10000);
		}
	}

	return status;
}


static status_t
device_consumer_thread(void* _interface)
{
	net_device_interface* interface = (net_device_interface*)_interface;
	net_device* device = interface->device;
	net_buffer* buffer;

	while (true) {
		ssize_t status = fifo_dequeue_buffer(&interface->receive_queue, 0,
			B_INFINITE_TIMEOUT, &buffer);
		if (status != B_OK) {
			if (status == B_INTERRUPTED)
				continue;
			break;
		}

		if (buffer->interface_address != NULL) {
			// If the interface is already specified, this buffer was
			// delivered locally.
			if (buffer->interface_address->domain->module->receive_data(buffer)
					== B_OK)
				buffer = NULL;
		} else {
			sockaddr_dl& linkAddress = *(sockaddr_dl*)buffer->source;
			int32 genericType = buffer->type;
			int32 specificType = B_NET_FRAME_TYPE(linkAddress.sdl_type,
				ntohs(linkAddress.sdl_e_type));

			buffer->index = interface->device->index;

			// Find handler for this packet

			RecursiveLocker locker(interface->receive_lock);

			DeviceHandlerList::Iterator iterator
				= interface->receive_funcs.GetIterator();
			while (buffer != NULL && iterator.HasNext()) {
				net_device_handler* handler = iterator.Next();

				// If the handler returns B_OK, it consumed the buffer - first
				// handler wins.
				if ((handler->type == genericType
						|| handler->type == specificType)
					&& handler->func(handler->cookie, device, buffer) == B_OK)
					buffer = NULL;
			}
		}

		if (buffer != NULL)
			gNetBufferModule.free(buffer);
	}

	return B_OK;
}


/*!	The domain's device receive handler - this will inject the net_buffers into
	the protocol layer (the domain's registered receive handler).
*/
static status_t
domain_receive_adapter(void* cookie, net_device* device, net_buffer* buffer)
{
	net_domain_private* domain = (net_domain_private*)cookie;

	return domain->module->receive_data(buffer);
}


static net_device_interface*
find_device_interface(const char* name)
{
	ASSERT_LOCKED_MUTEX(&sLock);
	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();

	while (net_device_interface* interface = iterator.Next()) {
		if (!strcmp(interface->device->name, name))
			return interface;
	}

	return NULL;
}


static net_device_interface*
allocate_device_interface(net_device* device, net_device_module_info* module)
{
	net_device_interface* interface = new(std::nothrow) net_device_interface;
	if (interface == NULL)
		return NULL;

	recursive_lock_init(&interface->receive_lock, "device interface receive");
	recursive_lock_init(&interface->monitor_lock, "device interface monitors");

	char name[128];
	snprintf(name, sizeof(name), "%s receive queue", device->name);

	if (init_fifo(&interface->receive_queue, name, 16 * 1024 * 1024) < B_OK)
		goto error1;

	interface->device = device;
	interface->up_count = 0;
	interface->ref_count = 1;
	interface->busy = false;
	interface->monitor_count = 0;
	interface->deframe_func = NULL;
	interface->deframe_ref_count = 0;

	snprintf(name, sizeof(name), "%s consumer", device->name);

	interface->reader_thread   = -1;
	interface->consumer_thread = spawn_kernel_thread(device_consumer_thread,
		name, B_DISPLAY_PRIORITY, interface);
	if (interface->consumer_thread < B_OK)
		goto error2;
	resume_thread(interface->consumer_thread);

	// TODO: proper interface index allocation
	device->index = ++sDeviceIndex;
	device->module = module;

	sInterfaces.Add(interface);
	return interface;

error2:
	uninit_fifo(&interface->receive_queue);
error1:
	recursive_lock_destroy(&interface->receive_lock);
	recursive_lock_destroy(&interface->monitor_lock);
	delete interface;

	return NULL;
}


static void
notify_device_monitors(net_device_interface* interface, int32 event)
{
	RecursiveLocker locker(interface->monitor_lock);

	DeviceMonitorList::Iterator iterator
		= interface->monitor_funcs.GetIterator();
	while (net_device_monitor* monitor = iterator.Next()) {
		// it's safe for the "current" item to remove itself.
		monitor->event(monitor, event);
	}
}


#if ENABLE_DEBUGGER_COMMANDS


static int
dump_device_interface(int argc, char** argv)
{
	if (argc != 2) {
		kprintf("usage: %s [address]\n", argv[0]);
		return 0;
	}

	net_device_interface* interface
		= (net_device_interface*)parse_expression(argv[1]);

	kprintf("device:            %p\n", interface->device);
	kprintf("reader_thread:     %" B_PRId32 "\n", interface->reader_thread);
	kprintf("up_count:          %" B_PRIu32 "\n", interface->up_count);
	kprintf("ref_count:         %" B_PRId32 "\n", interface->ref_count);
	kprintf("deframe_func:      %p\n", interface->deframe_func);
	kprintf("deframe_ref_count: %" B_PRId32 "\n", interface->ref_count);
	kprintf("consumer_thread:   %" B_PRId32 "\n", interface->consumer_thread);

	kprintf("monitor_count:     %" B_PRId32 "\n", interface->monitor_count);
	kprintf("monitor_lock:      %p\n", &interface->monitor_lock);
	kprintf("monitor_funcs:\n");
	DeviceMonitorList::Iterator monitorIterator
		= interface->monitor_funcs.GetIterator();
	while (monitorIterator.HasNext())
		kprintf("  %p\n", monitorIterator.Next());

	kprintf("receive_lock:      %p\n", &interface->receive_lock);
	kprintf("receive_queue:     %p\n", &interface->receive_queue);
	kprintf("receive_funcs:\n");
	DeviceHandlerList::Iterator handlerIterator
		= interface->receive_funcs.GetIterator();
	while (handlerIterator.HasNext())
		kprintf("  %p\n", handlerIterator.Next());

	return 0;
}


static int
dump_device_interfaces(int argc, char** argv)
{
	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (net_device_interface* interface = iterator.Next()) {
		kprintf("  %p  %s\n", interface, interface->device->name);
	}

	return 0;
}


#endif	// ENABLE_DEBUGGER_COMMANDS


//	#pragma mark - device interfaces


net_device_interface*
acquire_device_interface(net_device_interface* interface)
{
	if (interface == NULL || atomic_add(&interface->ref_count, 1) == 0)
		return NULL;

	return interface;
}


void
get_device_interface_address(net_device_interface* interface,
	sockaddr* _address)
{
	sockaddr_dl &address = *(sockaddr_dl*)_address;

	address.sdl_family = AF_LINK;
	address.sdl_index = interface->device->index;
	address.sdl_type = interface->device->type;
	address.sdl_nlen = strlen(interface->device->name);
	address.sdl_slen = 0;
	memcpy(address.sdl_data, interface->device->name, address.sdl_nlen);

	address.sdl_alen = interface->device->address.length;
	memcpy(LLADDR(&address), interface->device->address.data, address.sdl_alen);

	address.sdl_len = sizeof(sockaddr_dl) - sizeof(address.sdl_data)
		+ address.sdl_nlen + address.sdl_alen;
}


uint32
count_device_interfaces()
{
	MutexLocker locker(sLock);

	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();
	uint32 count = 0;

	while (iterator.HasNext()) {
		iterator.Next();
		count++;
	}

	return count;
}


/*!	Dumps a list of all interfaces into the supplied userland buffer.
	If the interfaces don't fit into the buffer, an error (\c ENOBUFS) is
	returned.
*/
status_t
list_device_interfaces(void* _buffer, size_t* bufferSize)
{
	MutexLocker locker(sLock);

	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();
	UserBuffer buffer(_buffer, *bufferSize);

	while (net_device_interface* interface = iterator.Next()) {
		buffer.Push(interface->device->name, IF_NAMESIZE);

		sockaddr_storage address;
		get_device_interface_address(interface, (sockaddr*)&address);

		buffer.Push(&address, address.ss_len);
		if (IF_NAMESIZE + address.ss_len < (int)sizeof(ifreq))
			buffer.Pad(sizeof(ifreq) - IF_NAMESIZE - address.ss_len);

		if (buffer.Status() != B_OK)
			return buffer.Status();
	}

	*bufferSize = buffer.BytesConsumed();
	return B_OK;
}


/*!	Releases the reference for the interface. When all references are
	released, the interface is removed.
*/
void
put_device_interface(struct net_device_interface* interface)
{
	if (interface == NULL)
		return;

	if (atomic_add(&interface->ref_count, -1) != 1)
		return;

	MutexLocker locker(sLock);
	sInterfaces.Remove(interface);
	locker.Unlock();

	uninit_fifo(&interface->receive_queue);
	status_t status;
	wait_for_thread(interface->consumer_thread, &status);

	net_device* device = interface->device;
	const char* moduleName = device->module->info.name;

	device->module->uninit_device(device);
	put_module(moduleName);

	recursive_lock_destroy(&interface->monitor_lock);
	recursive_lock_destroy(&interface->receive_lock);
	delete interface;
}


/*!	Finds an interface by the specified index and acquires a reference to it.
*/
struct net_device_interface*
get_device_interface(uint32 index)
{
	MutexLocker locker(sLock);

	// TODO: maintain an array of all device interfaces instead
	DeviceInterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (net_device_interface* interface = iterator.Next()) {
		if (interface->device->index == index) {
			if (interface->busy)
				break;

			if (atomic_add(&interface->ref_count, 1) != 0)
				return interface;
		}
	}

	return NULL;
}


/*!	Finds an interface by the specified name and grabs a reference to it.
	If the interface does not yet exist, a new one is created.
*/
struct net_device_interface*
get_device_interface(const char* name, bool create)
{
	MutexLocker locker(sLock);

	net_device_interface* interface = find_device_interface(name);
	if (interface != NULL) {
		if (interface->busy)
			return NULL;

		if (atomic_add(&interface->ref_count, 1) != 0)
			return interface;

		// try to recreate interface - it just got removed
	}

	if (!create)
		return NULL;

	void* cookie = open_module_list("network/devices");
	if (cookie == NULL)
		return NULL;

	while (true) {
		char moduleName[B_FILE_NAME_LENGTH];
		size_t length = sizeof(moduleName);
		if (read_next_module_name(cookie, moduleName, &length) != B_OK)
			break;

		TRACE(("get_device_interface: ask \"%s\" for %s\n", moduleName, name));

		net_device_module_info* module;
		if (get_module(moduleName, (module_info**)&module) == B_OK) {
			net_device* device;
			status_t status = module->init_device(name, &device);
			if (status == B_OK) {
				interface = allocate_device_interface(device, module);
				if (interface != NULL)
					return interface;

				module->uninit_device(device);
			}
			put_module(moduleName);
		}
	}

	close_module_list(cookie);

	return NULL;
}


/*!	Feeds the device monitors of the \a interface with the specified \a buffer.
	You might want to check interface::monitor_count before calling this
	function for optimization.
*/
void
device_interface_monitor_receive(net_device_interface* interface,
	net_buffer* buffer)
{
	RecursiveLocker locker(interface->monitor_lock);

	DeviceMonitorList::Iterator iterator
		= interface->monitor_funcs.GetIterator();
	while (iterator.HasNext()) {
		net_device_monitor* monitor = iterator.Next();
		monitor->receive(monitor, buffer);
	}
}


status_t
up_device_interface(net_device_interface* interface)
{
	net_device* device = interface->device;

	RecursiveLocker locker(interface->receive_lock);

	if (interface->up_count != 0) {
		interface->up_count++;
		return B_OK;
	}

	status_t status = device->module->up(device);
	if (status != B_OK)
		return status;

	if (device->module->receive_data != NULL) {
		// give the thread a nice name
		char name[B_OS_NAME_LENGTH];
		snprintf(name, sizeof(name), "%s reader", device->name);

		interface->reader_thread = spawn_kernel_thread(device_reader_thread,
			name, B_REAL_TIME_DISPLAY_PRIORITY - 10, interface);
		if (interface->reader_thread < B_OK)
			return interface->reader_thread;
	}

	device->flags |= IFF_UP;

	if (device->module->receive_data != NULL)
		resume_thread(interface->reader_thread);

	interface->up_count = 1;
	return B_OK;
}


void
down_device_interface(net_device_interface* interface)
{
	// Receive lock must be held when calling down_device_interface.
	// Known callers are `interface_protocol_down' which gets
	// here via one of the following paths:
	//
	// - domain_interface_control() [rx lock held, domain lock held]
	//    interface_set_down()
	//     interface_protocol_down()
	//
	// - domain_interface_control() [rx lock held, domain lock held]
	//    remove_interface_from_domain()
	//     delete_interface()
	//      interface_set_down()

	net_device* device = interface->device;

	device->flags &= ~IFF_UP;
	device->module->down(device);

	notify_device_monitors(interface, B_DEVICE_GOING_DOWN);

	if (device->module->receive_data != NULL) {
		thread_id readerThread = interface->reader_thread;

		// make sure the reader thread is gone before shutting down the interface
		status_t status;
		wait_for_thread(readerThread, &status);
	}
}


//	#pragma mark - devices stack API


/*!	Unregisters a previously registered deframer function. */
status_t
unregister_device_deframer(net_device* device)
{
	MutexLocker locker(sLock);

	// find device interface for this device
	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	RecursiveLocker _(interface->receive_lock);

	if (--interface->deframe_ref_count == 0)
		interface->deframe_func = NULL;

	return B_OK;
}


/*!	Registers the deframer function for the specified \a device.
	Note, however, that right now, you can only register one single
	deframer function per device.

	If the need arises, we might want to lift that limitation at a
	later time (which would require a slight API change, though).
*/
status_t
register_device_deframer(net_device* device, net_deframe_func deframeFunc)
{
	MutexLocker locker(sLock);

	// find device interface for this device
	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	RecursiveLocker _(interface->receive_lock);

	if (interface->deframe_func != NULL
		&& interface->deframe_func != deframeFunc)
		return B_ERROR;

	interface->deframe_func = deframeFunc;
	interface->deframe_ref_count++;
	return B_OK;
}


/*!	Registers a domain to receive net_buffers from the specified \a device. */
status_t
register_domain_device_handler(struct net_device* device, int32 type,
	struct net_domain* _domain)
{
	net_domain_private* domain = (net_domain_private*)_domain;
	if (domain->module == NULL || domain->module->receive_data == NULL)
		return B_BAD_VALUE;

	return register_device_handler(device, type, &domain_receive_adapter,
		domain);
}


/*!	Registers a receiving function callback for the specified \a device. */
status_t
register_device_handler(struct net_device* device, int32 type,
	net_receive_func receiveFunc, void* cookie)
{
	MutexLocker locker(sLock);

	// find device interface for this device
	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	RecursiveLocker _(interface->receive_lock);

	// see if such a handler already for this device

	DeviceHandlerList::Iterator iterator
		= interface->receive_funcs.GetIterator();
	while (net_device_handler* handler = iterator.Next()) {
		if (handler->type == type)
			return B_ERROR;
	}

	// Add new handler

	net_device_handler* handler = new(std::nothrow) net_device_handler;
	if (handler == NULL)
		return B_NO_MEMORY;

	handler->func = receiveFunc;
	handler->type = type;
	handler->cookie = cookie;
	interface->receive_funcs.Add(handler);
	return B_OK;
}


/*!	Unregisters a previously registered device handler. */
status_t
unregister_device_handler(struct net_device* device, int32 type)
{
	MutexLocker locker(sLock);

	// find device interface for this device
	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	RecursiveLocker _(interface->receive_lock);

	// search for the handler

	DeviceHandlerList::Iterator iterator
		= interface->receive_funcs.GetIterator();
	while (net_device_handler* handler = iterator.Next()) {
		if (handler->type == type) {
			// found it
			iterator.Remove();
			delete handler;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


/*!	Registers a device monitor for the specified device. */
status_t
register_device_monitor(net_device* device, net_device_monitor* monitor)
{
	if (monitor->receive == NULL || monitor->event == NULL)
		return B_BAD_VALUE;

	MutexLocker locker(sLock);

	// find device interface for this device
	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	RecursiveLocker monitorLocker(interface->monitor_lock);
	interface->monitor_funcs.Add(monitor);
	atomic_add(&interface->monitor_count, 1);

	return B_OK;
}


/*!	Unregisters a previously registered device monitor. */
status_t
unregister_device_monitor(net_device* device, net_device_monitor* monitor)
{
	MutexLocker locker(sLock);

	// find device interface for this device
	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	RecursiveLocker monitorLocker(interface->monitor_lock);

	// search for the monitor

	DeviceMonitorList::Iterator iterator
		= interface->monitor_funcs.GetIterator();
	while (iterator.HasNext()) {
		if (iterator.Next() == monitor) {
			iterator.Remove();
			atomic_add(&interface->monitor_count, -1);
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


/*!	This function is called by device modules in case their link
	state changed, ie. if an ethernet cable was plugged in or
	removed.
*/
status_t
device_link_changed(net_device* device)
{
	notify_link_changed(device);
	return B_OK;
}


/*!	This function is called by device modules once their device got
	physically removed, ie. a USB networking card is unplugged.
*/
status_t
device_removed(net_device* device)
{
	MutexLocker locker(sLock);

	net_device_interface* interface = find_device_interface(device->name);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;
	if (interface->busy)
		return B_BUSY;

	// Acquire a reference to the device interface being removed
	// so our put_() will (eventually) do the final cleanup
	atomic_add(&interface->ref_count, 1);
	interface->busy = true;
	locker.Unlock();

	// Propagate the loss of the device throughout the stack.

	interface_removed_device_interface(interface);
	notify_device_monitors(interface, B_DEVICE_BEING_REMOVED);

	// By now all of the monitors must have removed themselves. If they
	// didn't, they'll probably wait forever to be callback'ed again.
	RecursiveLocker monitorLocker(interface->monitor_lock);
	interface->monitor_funcs.RemoveAll();
	monitorLocker.Unlock();

	// All of the readers should be gone as well since we are out of
	// interfaces and put_domain_datalink_protocols() is called for
	// each delete_interface().

	put_device_interface(interface);

	return B_OK;
}


status_t
device_enqueue_buffer(net_device* device, net_buffer* buffer)
{
	net_device_interface* interface = get_device_interface(device->index);
	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	status_t status = fifo_enqueue_buffer(&interface->receive_queue, buffer);

	put_device_interface(interface);
	return status;
}


//	#pragma mark -


status_t
init_device_interfaces()
{
	mutex_init(&sLock, "net device interfaces");

	new (&sInterfaces) DeviceInterfaceList;
		// static C++ objects are not initialized in the module startup

#if ENABLE_DEBUGGER_COMMANDS
	add_debugger_command("net_device_interface", &dump_device_interface,
		"Dump the given network device interface");
	add_debugger_command("net_device_interfaces", &dump_device_interfaces,
		"Dump network device interfaces");
#endif
	return B_OK;
}


status_t
uninit_device_interfaces()
{
#if ENABLE_DEBUGGER_COMMANDS
	remove_debugger_command("net_device_interface", &dump_device_interface);
	remove_debugger_command("net_device_interfaces", &dump_device_interfaces);
#endif

	mutex_destroy(&sLock);
	return B_OK;
}


/*
 * Copyright 2006-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DEVICE_INTERFACES_H
#define DEVICE_INTERFACES_H


#include <net_datalink.h>
#include <net_stack.h>

#include <util/DoublyLinkedList.h>


struct net_device_handler : DoublyLinkedListLinkImpl<net_device_handler> {
	net_receive_func	func;
	int32				type;
	void*				cookie;
};

typedef DoublyLinkedList<net_device_handler> DeviceHandlerList;

typedef DoublyLinkedList<net_device_monitor,
	DoublyLinkedListCLink<net_device_monitor> > DeviceMonitorList;

struct net_device_interface : DoublyLinkedListLinkImpl<net_device_interface> {
	struct net_device*	device;
	thread_id			reader_thread;
	uint32				up_count;
		// a device can be brought up by more than one interface
	int32				ref_count;
	bool				busy;

	net_deframe_func	deframe_func;
	int32				deframe_ref_count;

	int32				monitor_count;
	recursive_lock		monitor_lock;
	DeviceMonitorList	monitor_funcs;

	DeviceHandlerList	receive_funcs;
	recursive_lock		receive_lock;

	thread_id			consumer_thread;
	net_fifo			receive_queue;
};

typedef DoublyLinkedList<net_device_interface> DeviceInterfaceList;


// device interfaces
net_device_interface* acquire_device_interface(net_device_interface* interface);
void get_device_interface_address(net_device_interface* interface,
	sockaddr* address);
uint32 count_device_interfaces();
status_t list_device_interfaces(void* buffer, size_t* _bufferSize);
void put_device_interface(struct net_device_interface* interface);
struct net_device_interface* get_device_interface(uint32 index);
struct net_device_interface* get_device_interface(const char* name,
	bool create = true);
void device_interface_monitor_receive(net_device_interface* interface,
	net_buffer* buffer);
status_t up_device_interface(net_device_interface* interface);
void down_device_interface(net_device_interface* interface);

// devices
status_t unregister_device_deframer(net_device* device);
status_t register_device_deframer(net_device* device,
	net_deframe_func deframeFunc);
status_t register_domain_device_handler(struct net_device* device, int32 type,
	struct net_domain* domain);
status_t register_device_handler(struct net_device* device, int32 type,
	net_receive_func receiveFunc, void* cookie);
status_t unregister_device_handler(struct net_device* device, int32 type);
status_t register_device_monitor(struct net_device* device,
	struct net_device_monitor* monitor);
status_t unregister_device_monitor(struct net_device* device,
	struct net_device_monitor* monitor);
status_t device_link_changed(net_device* device);
status_t device_removed(net_device* device);
status_t device_enqueue_buffer(net_device* device, net_buffer* buffer);

status_t init_device_interfaces();
status_t uninit_device_interfaces();


#endif	// DEVICE_INTERFACES_H

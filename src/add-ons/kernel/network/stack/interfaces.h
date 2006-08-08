/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTERFACES_H
#define INTERFACES_H


#include <net_datalink.h>
#include <net_stack.h>

#include <util/DoublyLinkedList.h>


struct net_device_handler : public DoublyLinkedListLinkImpl<net_device_handler> {
	net_receive_func	func;
	int32				type;
	void				*cookie;
};

struct net_device_monitor : public DoublyLinkedListLinkImpl<net_device_monitor> {
	net_receive_func	func;
	void				*cookie;
};

typedef DoublyLinkedList<net_device_handler> DeviceHandlerList;
typedef DoublyLinkedList<net_device_monitor> DeviceMonitorList;

struct net_device_interface {
	struct list_link	link;
	const char			*name;
	struct net_device_module_info *module;
	struct net_device	*device;
	uint32				up_count;
		// a device can be brought up by more than one interface
	int32				ref_count;

	net_deframe_func	deframe_func;
	int32				deframe_ref_count;

	DeviceMonitorList	monitor_funcs;
	DeviceHandlerList	receive_funcs;
};

struct net_interface_private : net_interface {
	char				base_name[IF_NAMESIZE];
	net_device_interface *device_interface;
};


status_t init_interfaces();
status_t uninit_interfaces();

// interfaces
struct net_interface_private *find_interface(struct net_domain *domain,
	const char *name);
struct net_interface_private *find_interface(struct net_domain *domain,
	uint32 index);
void put_interface(struct net_interface_private *interface);
struct net_interface_private *get_interface(net_domain *domain,
	const char *name);
status_t create_interface(net_domain *domain, const char *name,
	const char *baseName, net_device_interface *deviceInterface,
	struct net_interface_private **_interface);
void delete_interface(net_interface_private *interface);

// device interfaces
void get_device_interface_address(net_device_interface *interface,
	sockaddr *address);
uint32 count_device_interfaces();
status_t list_device_interfaces(void *buffer, size_t size);
void put_device_interface(struct net_device_interface *interface);
struct net_device_interface *get_device_interface(uint32 index);
struct net_device_interface *get_device_interface(const char *name);

// devices
status_t unregister_device_deframer(net_device *device);
status_t register_device_deframer(net_device *device, net_deframe_func deframeFunc);
status_t register_domain_device_handler(struct net_device *device, int32 type,
	struct net_domain *domain);
status_t register_device_handler(struct net_device *device, int32 type,
	net_receive_func receiveFunc, void *cookie);
status_t unregister_device_handler(struct net_device *device, int32 type);
status_t register_device_monitor(struct net_device *device,
	net_receive_func receiveFunc, void *cookie);
status_t unregister_device_monitor(struct net_device *device,
	net_receive_func receiveFunc, void *cookie);
status_t device_removed(net_device *device);

#endif	// INTERFACES_H

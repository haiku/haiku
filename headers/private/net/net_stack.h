/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_STACK_H
#define NET_STACK_H


#include <lock.h>
#include <util/list.h>

#include <module.h>


#define NET_STACK_MODULE_NAME "network/stack/v1"

struct net_fifo {
	benaphore	lock;
	sem_id		notify;
	int32		waiting;

	size_t		max_bytes;
	size_t		current_bytes;

	struct list	buffers;
};

typedef void (*net_timer_func)(struct net_timer *timer, void *data);

struct net_timer {
	struct list_link link;
	net_timer_func	hook;
	void			*data;
	bigtime_t		due;
};

typedef int32 (*net_deframe_func)(struct net_device *device, struct net_buffer *buffer);
typedef status_t (*net_receive_func)(void *cookie, struct net_buffer *buffer);

struct net_stack_module_info {
	module_info info;

	status_t (*register_domain)(int family, const char *name,
					struct net_protocol_module_info *module,
					struct net_address_module_info *addressModule,
					struct net_domain **_domain);
	status_t (*unregister_domain)(struct net_domain *domain);
	struct net_domain *(*get_domain)(int family);

	status_t (*register_domain_protocols)(int family, int type, int protocol, ...);
	status_t (*register_domain_datalink_protocols)(int family, int type, ...);
	status_t (*register_domain_receiving_protocol)(int family, int type,
					const char *moduleName);

	status_t (*get_domain_receiving_protocol)(struct net_domain *domain, uint32 type,
					struct net_protocol_module_info **_module);
	status_t (*put_domain_receiving_protocol)(struct net_domain *domain, uint32 type);

	// devices
	status_t (*register_device_deframer)(struct net_device *device,
					net_deframe_func deframeFunc);
	status_t (*unregister_device_deframer)(struct net_device *device);

	status_t (*register_domain_device_handler)(struct net_device *device,
					int32 type, struct net_domain *domain);
	status_t (*register_device_handler)(struct net_device *device, int32 type,
					net_receive_func receiveFunc, void *cookie);
	status_t (*unregister_device_handler)(struct net_device *device, int32 type);

	status_t (*register_device_monitor)(struct net_device *device,
					net_receive_func receiveFunc, void *cookie);
	status_t (*unregister_device_monitor)(struct net_device *device,
					net_receive_func receiveFunc, void *cookie);

	status_t (*device_removed)(struct net_device *device);

	// Utility Functions

	// checksum
	uint16 (*checksum)(uint8 *buffer, size_t length);

	// fifo
	status_t (*init_fifo)(struct net_fifo *fifo, const char *name, size_t maxBytes);
	void (*uninit_fifo)(struct net_fifo *fifo);
	status_t (*fifo_enqueue_buffer)(struct net_fifo *fifo, struct net_buffer *buffer);
	ssize_t (*fifo_dequeue_buffer)(struct net_fifo *fifo, uint32 flags,
					bigtime_t timeout, struct net_buffer **_buffer);
	status_t (*clear_fifo)(struct net_fifo *fifo);

	// timer
	void (*init_timer)(struct net_timer *timer, net_timer_func hook, void *data);
	void (*set_timer)(struct net_timer *timer, bigtime_t delay);
};

#endif	// NET_STACK_H

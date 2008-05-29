/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_STACK_H
#define NET_STACK_H


#include <lock.h>
#include <util/list.h>

#include <module.h>


#define NET_STACK_MODULE_NAME "network/stack/v1"


struct net_address_module_info;
struct net_protocol_module_info;

struct net_buffer;
struct net_device;
struct net_domain;
struct net_socket;
struct net_timer;

typedef struct ancillary_data_container ancillary_data_container;

struct net_fifo {
	mutex		lock;
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

typedef int32 (*net_deframe_func)(struct net_device *device,
	struct net_buffer *buffer);
typedef status_t (*net_receive_func)(void *cookie, struct net_device *device,
	struct net_buffer *buffer);

enum {
	B_DEVICE_GOING_UP = 1,
	B_DEVICE_GOING_DOWN,
	B_DEVICE_BEING_REMOVED,
};

struct net_device_monitor {
	struct list_link link;
	void *cookie;

	status_t (*receive)(struct net_device_monitor *monitor,
		struct net_buffer *buffer);
	void (*event)(struct net_device_monitor *monitor, int32 event);
};

typedef struct ancillary_data_header {
	int		level;
	int		type;
	size_t	len;
} ancillary_data_header;

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

	status_t (*get_domain_receiving_protocol)(struct net_domain *domain,
					uint32 type, struct net_protocol_module_info **_module);
	status_t (*put_domain_receiving_protocol)(struct net_domain *domain,
					uint32 type);

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
					struct net_device_monitor *monitor);
	status_t (*unregister_device_monitor)(struct net_device *device,
					struct net_device_monitor *monitor);

	status_t (*device_link_changed)(struct net_device *device);
	status_t (*device_removed)(struct net_device *device);

	status_t (*device_enqueue_buffer)(struct net_device *device,
					struct net_buffer *buffer);

	// Utility Functions

	// notification
	status_t (*notify_socket)(struct net_socket *socket, uint8 event,
					int32 value);

	// checksum
	uint16 (*checksum)(uint8 *buffer, size_t length);

	// fifo
	status_t (*init_fifo)(struct net_fifo *fifo, const char *name,
					size_t maxBytes);
	void (*uninit_fifo)(struct net_fifo *fifo);
	status_t (*fifo_enqueue_buffer)(struct net_fifo *fifo,
					struct net_buffer *buffer);
	ssize_t (*fifo_dequeue_buffer)(struct net_fifo *fifo, uint32 flags,
					bigtime_t timeout, struct net_buffer **_buffer);
	status_t (*clear_fifo)(struct net_fifo *fifo);
	status_t (*fifo_socket_enqueue_buffer)(struct net_fifo *fifo,
					struct net_socket *socket, uint8 event,
					struct net_buffer *buffer);

	// timer
	void (*init_timer)(struct net_timer *timer, net_timer_func hook, void *data);
	void (*set_timer)(struct net_timer *timer, bigtime_t delay);
	bool (*cancel_timer)(struct net_timer *timer);
	bool (*is_timer_active)(struct net_timer *timer);

	// syscall restart
	bool (*is_syscall)(void);
	bool (*is_restarted_syscall)(void);
	void (*store_syscall_restart_timeout)(bigtime_t timeout);
	bigtime_t (*restore_syscall_restart_timeout)(void);

	// ancillary data
	ancillary_data_container* (*create_ancillary_data_container)();
	void (*delete_ancillary_data_container)(
					ancillary_data_container* container);
	status_t (*add_ancillary_data)(ancillary_data_container *container,
					const ancillary_data_header *header, const void *data,
					void (*destructor)(const ancillary_data_header*, void*),
					void **_allocatedData);
	status_t (*remove_ancillary_data)(ancillary_data_container *container,
					void *data, bool destroy);
	void* (*move_ancillary_data)(ancillary_data_container *from,
					ancillary_data_container *to);
	void* (*next_ancillary_data)(ancillary_data_container *container,
					void *previousData, ancillary_data_header *_header);
};

#endif	// NET_STACK_H

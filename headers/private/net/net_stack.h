/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_STACK_H
#define NET_STACK_H


#include <module.h>

#include <lock.h>
#include <util/list.h>


#define NET_STACK_MODULE_NAME "network/stack/v1"


struct net_address_module_info;
struct net_protocol_module_info;

typedef struct net_buffer net_buffer;
typedef struct net_device net_device;
typedef struct net_domain net_domain;
typedef struct net_socket net_socket;

struct net_timer;


typedef struct net_fifo {
	mutex		lock;
	sem_id		notify;
	int32		waiting;

	size_t		max_bytes;
	size_t		current_bytes;

	struct list	buffers;
} net_fifo;

typedef void (*net_timer_func)(struct net_timer* timer, void* data);

typedef struct net_timer {
	struct list_link link;
	net_timer_func	hook;
	void*			data;
	bigtime_t		due;
	uint32			flags;
} net_timer;

typedef status_t (*net_deframe_func)(net_device* device, net_buffer* buffer);
typedef status_t (*net_receive_func)(void* cookie, net_device* device,
	net_buffer* buffer);

enum {
	B_DEVICE_GOING_UP = 1,
	B_DEVICE_GOING_DOWN,
	B_DEVICE_BEING_REMOVED,
};

typedef struct net_device_monitor {
	struct list_link link;
	void*	cookie;

	status_t (*receive)(struct net_device_monitor* monitor,
		struct net_buffer* buffer);
	void (*event)(struct net_device_monitor* monitor, int32 event);
} net_device_monitor;

typedef struct ancillary_data_header {
	int		level;
	int		type;
	size_t	len;
} ancillary_data_header;

typedef struct ancillary_data_container ancillary_data_container;


#define B_NET_FRAME_TYPE(super, sub)	(((int32)(super) << 16) | (sub))
	// Use this when registering a device handler, see net/if_types.h for
	// the possible "super" values. Input values are in host byte order.


// sub types
enum {
	B_NET_FRAME_TYPE_IPV4				= 0x0001,
	B_NET_FRAME_TYPE_IPV6				= 0x0002,
	B_NET_FRAME_TYPE_IPX				= 0x0003,
	B_NET_FRAME_TYPE_APPLE_TALK			= 0x0004,

	B_NET_FRAME_TYPE_ALL				= 0x0000
};


struct net_stack_module_info {
	module_info info;

	status_t	(*register_domain)(int family, const char* name,
					struct net_protocol_module_info* module,
					struct net_address_module_info* addressModule,
					net_domain** _domain);
	status_t	(*unregister_domain)(net_domain* domain);
	net_domain*	(*get_domain)(int family);

	status_t	(*register_domain_protocols)(int family, int type, int protocol,
					...);
	status_t	(*register_domain_datalink_protocols)(int family, int type,
					...);
	status_t	(*register_domain_receiving_protocol)(int family, int type,
					const char* moduleName);

	status_t	(*get_domain_receiving_protocol)(net_domain* domain,
					uint32 type, struct net_protocol_module_info** _module);
	status_t	(*put_domain_receiving_protocol)(net_domain* domain,
					uint32 type);

	// devices
	status_t	(*register_device_deframer)(net_device* device,
					net_deframe_func deframeFunc);
	status_t	(*unregister_device_deframer)(net_device* device);

	status_t	(*register_domain_device_handler)(net_device* device,
					int32 type, net_domain* domain);
	status_t	(*register_device_handler)(net_device* device,
					int32 type, net_receive_func receiveFunc, void* cookie);
	status_t	(*unregister_device_handler)(net_device* device, int32 type);

	status_t	(*register_device_monitor)(net_device* device,
					struct net_device_monitor* monitor);
	status_t	(*unregister_device_monitor)(net_device* device,
					struct net_device_monitor* monitor);

	status_t	(*device_link_changed)(net_device* device);
	status_t	(*device_removed)(net_device* device);

	status_t	(*device_enqueue_buffer)(net_device* device,
					net_buffer* buffer);

	// Utility Functions

	// notification
	status_t	(*notify_socket)(net_socket* socket, uint8 event, int32 value);

	// checksum
	uint16		(*checksum)(uint8* buffer, size_t length);

	// fifo
	status_t	(*init_fifo)(net_fifo* fifo, const char* name, size_t maxBytes);
	void		(*uninit_fifo)(net_fifo* fifo);
	status_t	(*fifo_enqueue_buffer)(net_fifo* fifo, net_buffer* buffer);
	ssize_t		(*fifo_dequeue_buffer)(net_fifo* fifo, uint32 flags,
					bigtime_t timeout, net_buffer** _buffer);
	status_t	(*clear_fifo)(net_fifo* fifo);
	status_t	(*fifo_socket_enqueue_buffer)(net_fifo* fifo,
					net_socket* socket, uint8 event, net_buffer* buffer);

	// timer
	void		(*init_timer)(net_timer* timer, net_timer_func hook,
					void* data);
	void		(*set_timer)(net_timer* timer, bigtime_t delay);
	bool		(*cancel_timer)(net_timer* timer);
	status_t	(*wait_for_timer)(net_timer* timer);
	bool		(*is_timer_active)(net_timer* timer);
	bool		(*is_timer_running)(net_timer* timer);

	// syscall restart
	bool		(*is_syscall)(void);
	bool		(*is_restarted_syscall)(void);
	void		(*store_syscall_restart_timeout)(bigtime_t timeout);
	bigtime_t	(*restore_syscall_restart_timeout)(void);

	// ancillary data
	ancillary_data_container* (*create_ancillary_data_container)();
	void		(*delete_ancillary_data_container)(
					ancillary_data_container* container);
	status_t	(*add_ancillary_data)(ancillary_data_container* container,
					const ancillary_data_header* header, const void* data,
					void (*destructor)(const ancillary_data_header*, void*),
					void** _allocatedData);
	status_t	(*remove_ancillary_data)(ancillary_data_container* container,
					void* data, bool destroy);
	void*		(*move_ancillary_data)(ancillary_data_container* from,
					ancillary_data_container* to);
	void*		(*next_ancillary_data)(ancillary_data_container* container,
					void* previousData, ancillary_data_header* _header);
};


#endif	// NET_STACK_H

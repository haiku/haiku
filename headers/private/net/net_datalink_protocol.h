/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_DATALINK_PROTOCOL_H
#define NET_DATALINK_PROTOCOL_H


#include <net_buffer.h>


typedef struct net_datalink_protocol {
	struct net_datalink_protocol*				next;
	struct net_datalink_protocol_module_info*	module;
	struct net_interface*						interface;
	struct net_domain*							domain;
} net_datalink_protocol;

struct net_datalink_protocol_module_info {
	module_info info;

	status_t	(*init_protocol)(net_interface* interface, net_domain* domain,
					net_datalink_protocol** _protocol);
	status_t	(*uninit_protocol)(net_datalink_protocol* self);

	status_t	(*send_data)(net_datalink_protocol* self, net_buffer* buffer);

	status_t	(*interface_up)(net_datalink_protocol* self);
	void		(*interface_down)(net_datalink_protocol* self);

	status_t	(*change_address)(net_datalink_protocol* self,
					net_interface_address* address, int32 option,
					const struct sockaddr* oldAddress,
					const struct sockaddr* newAddress);

	status_t	(*control)(net_datalink_protocol* self, int32 option,
					void* argument, size_t length);

	status_t	(*join_multicast)(net_datalink_protocol* self,
					const struct sockaddr* address);
	status_t	(*leave_multicast)(net_datalink_protocol* self,
					const struct sockaddr* address);
};


#endif	// NET_DATALINK_PROTOCOL_H

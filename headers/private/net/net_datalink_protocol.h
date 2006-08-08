/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_DATALINK_PROTOCOL_H
#define NET_DATALINK_PROTOCOL_H


#include <net_buffer.h>


typedef struct net_datalink_protocol {
	struct net_datalink_protocol				*next;
	struct net_datalink_protocol_module_info	*module;
	struct net_interface						*interface;
} net_datalink_protocol;

struct net_datalink_protocol_module_info {
	module_info info;

	status_t	(*init_protocol)(struct net_interface *interface,
					net_datalink_protocol **_protocol);
	status_t	(*uninit_protocol)(net_datalink_protocol *self);

	status_t	(*send_data)(net_datalink_protocol *self,
					net_buffer *buffer);

	status_t	(*interface_up)(net_datalink_protocol *self);
	void		(*interface_down)(net_datalink_protocol *self);

	status_t	(*control)(net_datalink_protocol *self,
					int32 op, void *argument, size_t length);
};

#endif	// NET_DATALINK_PROTOCOL_H

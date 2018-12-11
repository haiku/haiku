/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef ROUTES_H
#define ROUTES_H


#include <net_datalink.h>
#include <net_stack.h>

#include <util/DoublyLinkedList.h>


struct InterfaceAddress;


struct net_route_private
	: net_route, DoublyLinkedListLinkImpl<net_route_private> {
	int32	ref_count;

	net_route_private();
	~net_route_private();
};

typedef DoublyLinkedList<net_route_private> RouteList;
typedef DoublyLinkedList<net_route_info,
	DoublyLinkedListCLink<net_route_info> > RouteInfoList;


uint32 route_table_size(struct net_domain_private* domain);
status_t list_routes(struct net_domain_private* domain, void* buffer,
				size_t size);
status_t control_routes(struct net_interface* interface, net_domain* domain,
				int32 option, void* argument, size_t length);

status_t add_route(struct net_domain* domain,
				const struct net_route* route);
status_t remove_route(struct net_domain* domain,
				const struct net_route* route);
status_t get_route_information(struct net_domain* domain, void* buffer,
				size_t length);
void invalidate_routes(net_domain* domain, net_interface* interface);
void invalidate_routes(InterfaceAddress* address);
struct net_route* get_route(struct net_domain* domain,
				const struct sockaddr* address);
status_t get_device_route(struct net_domain* domain, uint32 index,
				struct net_route** _route);
status_t get_buffer_route(struct net_domain* domain,
				struct net_buffer* buffer, struct net_route** _route);
void put_route(struct net_domain* domain, struct net_route* route);

status_t register_route_info(struct net_domain* domain,
				struct net_route_info* info);
status_t unregister_route_info(struct net_domain* domain,
				struct net_route_info* info);
status_t update_route_info(struct net_domain* domain,
				struct net_route_info* info);

#endif	// ROUTES_H

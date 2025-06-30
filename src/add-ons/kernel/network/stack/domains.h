/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DOMAINS_H
#define DOMAINS_H


#include <lock.h>
#include <util/list.h>
#include <util/DoublyLinkedList.h>
#include "radix.h" // For struct radix_node_head

#include "routes.h"


struct net_device_interface;


struct net_domain_private : net_domain,
		DoublyLinkedListLinkImpl<net_domain_private> {
	recursive_lock		lock;

	// RouteList			routes; // Replaced by radix tree
	struct radix_node_head* rnh_head; // Radix tree head for this domain's routes
	RouteInfoList		route_infos;
};


net_domain* get_domain(int family);
status_t register_domain(int family, const char* name,
	struct net_protocol_module_info* module,
	struct net_address_module_info* addressModule, net_domain* *_domain);
status_t unregister_domain(net_domain* domain);

status_t init_domains();
status_t uninit_domains();


#endif	// DOMAINS_H

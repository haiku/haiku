/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DOMAINS_H
#define DOMAINS_H


#include "routes.h"

#include <lock.h>
#include <util/list.h>
#include <util/DoublyLinkedList.h>


struct net_domain_private : net_domain {
	struct list_link	link;

	benaphore			lock;

	RouteList			routes;
	RouteInfoList		route_infos;
};


status_t init_domains();
status_t uninit_domains();

uint32 count_domain_interfaces();
status_t list_domain_interfaces(void *buffer, size_t *_bufferSize);
status_t add_interface_to_domain(net_domain *domain, struct ifreq& request);
status_t remove_interface_from_domain(net_interface *interface);

net_domain *get_domain(int family);
status_t register_domain(int family, const char *name,
			struct net_protocol_module_info *module, 
			struct net_address_module_info *addressModule,
			net_domain **_domain);
status_t unregister_domain(net_domain *domain);

#endif	// DOMAINS_H

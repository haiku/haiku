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

// #include <util/DoublyLinkedList.h> // No longer using DoublyLinkedList for routes
#include <stddef.h> // For offsetof
#include "radix.h" // For struct radix_node

struct InterfaceAddress;


struct net_route_private : net_route {
	int32	ref_count;
	struct radix_node rn_nodes[2]; // For radix tree integration

	net_route_private();
	~net_route_private();

	// Helper to get the containing net_route_private from its leaf radix_node
	// The leaf node is typically rn_nodes[0] as returned by rn_addroute/rn_match.
	static net_route_private* FromRadixNode(struct radix_node* node) {
		if (node == NULL || (node->rn_flags & RNF_ROOT) != 0)
			return NULL;
		// Check if the node pointer is within a valid range of a potential rn_nodes array.
		// This is a basic sanity check, not foolproof.
		// A more robust way might involve a magic number if we were really paranoid.
		// For now, rely on the fact that 'node' is expected to be rn_nodes[0].
		return reinterpret_cast<net_route_private*>(
			reinterpret_cast<char*>(node) - offsetof(net_route_private, rn_nodes[0]));
	}
};

// typedef DoublyLinkedList<net_route_private> RouteList; // No longer used
typedef DoublyLinkedList<net_route_info,
	DoublyLinkedListCLink<net_route_info> > RouteInfoList;


// Forward declaration for functions below
struct net_domain_private;

status_t init_routing_domain_radix(struct net_domain_private* domain);
void deinit_routing_domain_radix(struct net_domain_private* domain);

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

/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_DATALINK_H
#define NET_DATALINK_H


#include <net_buffer.h>
#include <net_routing_info.h>

#include <util/list.h>

#include <net/if.h>


#define NET_DATALINK_MODULE_NAME "network/stack/datalink/v1"

typedef struct net_datalink_protocol net_datalink_protocol;

typedef struct net_domain {
	const char			*name;
	int					family;
	struct list			interfaces;

	struct net_protocol_module_info *module;
	struct net_address_module_info *address_module;
} net_domain;

struct net_interface {
	struct list_link	link;
	struct net_domain	*domain;
	struct net_device	*device;
	struct net_datalink_protocol *first_protocol;
	struct net_datalink_protocol_module_info *first_info;

	char				name[IF_NAMESIZE];
	struct sockaddr		*address;
	struct sockaddr		*destination;
	struct sockaddr		*mask;
	uint32				index;
	uint32				flags;
	uint8				type;
	uint32				mtu;
	uint32				metric;
};

struct net_route {
	struct sockaddr		*destination;
	struct sockaddr		*mask;
	struct sockaddr		*gateway;
	uint32				flags;
	uint32				mtu;
	struct net_interface *interface;
};

struct net_route_info {
	struct list_link	link;
	struct net_route	*route;
	struct sockaddr		address;
};

struct net_datalink_module_info {
	module_info info;

	status_t (*control)(struct net_domain *domain, int32 option, void *value,
					size_t *_length);
	status_t (*send_data)(struct net_route *route, struct net_buffer *buffer);

	bool (*is_local_address)(struct net_domain *domain, 
					const struct sockaddr *address,
					net_interface **_interface, 
					uint32 *_matchedType);
	net_interface *(*get_interface_with_address)(struct net_domain *domain,
		const struct sockaddr *address);

	// routes
	status_t (*add_route)(struct net_domain *domain,
					const struct net_route *route);
	status_t (*remove_route)(struct net_domain *domain,
					const struct net_route *route);
	struct net_route *(*get_route)(struct net_domain *domain,
					const struct sockaddr *address);
	status_t (*get_buffer_route)(struct net_domain *domain,
		struct net_buffer *buffer, struct net_route **_route);
	void (*put_route)(struct net_domain *domain, struct net_route *route);

	status_t (*register_route_info)(struct net_domain *domain,
					struct net_route_info *info);
	status_t (*unregister_route_info)(struct net_domain *domain,
					struct net_route_info *info);
	status_t (*update_route_info)(struct net_domain *domain,
					struct net_route_info *info);
};

struct net_address_module_info {
	module_info info;

	status_t (*copy_address)(const sockaddr *from, sockaddr **to,
					bool replaceWithZeros /* = false */, const sockaddr *mask /* = NULL*/);

	status_t (*mask_address)(const sockaddr *address, const sockaddr *mask, 
					sockaddr *result);
	
	bool (*equal_addresses)(const sockaddr *a, const sockaddr *b);
	bool (*equal_ports)(const sockaddr *a, const sockaddr *b);
	bool (*equal_addresses_and_ports)(const sockaddr *a, const sockaddr *b);
	bool (*equal_masked_addresses)(const sockaddr *a, const sockaddr *b, 
					const sockaddr *mask);
	bool (*is_empty_address)(const sockaddr *address, bool checkPort /* = true */);

	int32 (*first_mask_bit)(const sockaddr *mask);

	bool (*check_mask)(const sockaddr *address);
	
	status_t (*print_address)(const sockaddr *address, char **buffer, 
					bool printPort);

	uint16 (*get_port)(const sockaddr *address);
	status_t (*set_port)(sockaddr *address, uint16 port);

	status_t (*set_to)(sockaddr *address, const sockaddr *from);
	status_t (*set_to_empty_address)(sockaddr *address);

	status_t (*update_to)(sockaddr *address, const sockaddr *from);

	uint32 (*hash_address_pair)(const sockaddr *ourAddress, 
					const sockaddr *peerAddress);

	status_t (*checksum_address)(struct Checksum *checksum, 
					const sockaddr *address);

	bool (*matches_broadcast_address)(const sockaddr *address, 
					const sockaddr *mask, const sockaddr *broadcastAddr);
};

#endif	// NET_DATALINK_H

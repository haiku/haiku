/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_DATALINK_H
#define NET_DATALINK_H


#include <net/if.h>

#include <net_buffer.h>
#include <net_routing_info.h>

#include <util/list.h>


#define NET_DATALINK_MODULE_NAME "network/stack/datalink/v1"


class Checksum;
struct net_protocol;


typedef struct net_datalink_protocol net_datalink_protocol;


typedef struct net_domain {
	const char*			name;
	int					family;

	struct net_protocol_module_info* module;
	struct net_address_module_info* address_module;
} net_domain;

typedef struct net_interface_address {
	struct net_domain*	domain;
	struct net_interface* interface;
	struct sockaddr*	local;
	struct sockaddr*	destination;
	struct sockaddr*	mask;
	uint32_t			flags;
} net_interface_address;

typedef struct net_interface {
	struct net_device*	device;

	char				name[IF_NAMESIZE];
	uint32				index;
	uint32				flags;
	uint8				type;
	uint32				mtu;
	uint32				metric;
} net_interface;

typedef struct net_route {
	struct sockaddr*	destination;
	struct sockaddr*	mask;
	struct sockaddr*	gateway;
	uint32				flags;
	uint32				mtu;
	struct net_interface_address* interface_address;
} net_route;

typedef struct net_route_info {
	struct list_link	link;
	struct net_route*	route;
	struct sockaddr		address;
} net_route_info;


struct net_datalink_module_info {
	module_info info;

	status_t		(*control)(net_domain* domain, int32 option, void* value,
						size_t* _length);
	status_t		(*send_routed_data)(net_route* route, net_buffer* buffer);
	status_t		(*send_data)(struct net_protocol* protocol,
						net_domain* domain, net_buffer* buffer);

	bool			(*is_local_address)(net_domain* domain,
						const struct sockaddr* address,
						net_interface_address** _interfaceAddress,
						uint32* _matchedType);
	bool			(*is_local_link_address)(net_domain* domain,
						bool unconfigured, const struct sockaddr* address,
						net_interface_address** _interfaceAddress);

	net_interface*	(*get_interface)(net_domain* domain, uint32 index);
	net_interface*	(*get_interface_with_address)(
						const struct sockaddr* address);
	void			(*put_interface)(net_interface* interface);

	net_interface_address* (*get_interface_address)(
						const struct sockaddr* address);
	bool			(*get_next_interface_address)(net_interface* interface,
						net_interface_address** _address);
	void			(*put_interface_address)(net_interface_address* address);

	status_t		(*join_multicast)(net_interface* interface,
						net_domain* domain, const struct sockaddr* address);
	status_t		(*leave_multicast)(net_interface* interface,
						net_domain* domain, const struct sockaddr* address);

	// routes
	status_t		(*add_route)(net_domain* domain, const net_route* route);
	status_t		(*remove_route)(net_domain* domain, const net_route* route);
	net_route*		(*get_route)(net_domain* domain,
						const struct sockaddr* address);
	status_t		(*get_buffer_route)(net_domain* domain,
						struct net_buffer* buffer, net_route** _route);
	void			(*put_route)(net_domain* domain, net_route* route);

	status_t		(*register_route_info)(net_domain* domain,
						net_route_info* info);
	status_t		(*unregister_route_info)(net_domain* domain,
						net_route_info* info);
	status_t		(*update_route_info)(net_domain* domain,
						net_route_info* info);
};

#define NET_ADDRESS_MODULE_FLAG_BROADCAST_ADDRESS		0x01

struct net_address_module_info {
	module_info info;
	uint32 flags;

	status_t		(*copy_address)(const struct sockaddr* from,
						struct sockaddr** to, bool replaceWithZeros,
						const struct sockaddr* mask);

	status_t		(*mask_address)(const struct sockaddr* address,
						const struct sockaddr* mask, struct sockaddr* result);

	bool			(*equal_addresses)(const struct sockaddr* a,
						const struct sockaddr* b);
	bool			(*equal_ports)(const struct sockaddr* a,
						const struct sockaddr* b);
	bool			(*equal_addresses_and_ports)(const struct sockaddr* a,
						const struct sockaddr* b);
	bool			(*equal_masked_addresses)(const struct sockaddr* a,
						const struct sockaddr* b, const struct sockaddr* mask);
	bool			(*is_empty_address)(const struct sockaddr* address,
						bool checkPort);
	bool			(*is_same_family)(const struct sockaddr* address);

	int32			(*first_mask_bit)(const struct sockaddr* mask);

	bool			(*check_mask)(const struct sockaddr* address);

	status_t		(*print_address)(const struct sockaddr* address,
						char** buffer, bool printPort);
	status_t		(*print_address_buffer)(const struct sockaddr* address,
						char* buffer, size_t bufferSize, bool printPort);

	uint16			(*get_port)(const struct sockaddr* address);
	status_t		(*set_port)(struct sockaddr* address, uint16 port);

	status_t		(*set_to)(struct sockaddr* address,
						const struct sockaddr* from);
	status_t		(*set_to_empty_address)(struct sockaddr* address);
	status_t		(*set_to_defaults)(struct sockaddr* defaultMask,
						struct sockaddr* defaultBroadcast,
						const struct sockaddr* address,
						const struct sockaddr* netmask);

	status_t		(*update_to)(struct sockaddr* address,
						const struct sockaddr* from);

	uint32			(*hash_address)(const struct sockaddr* address,
						bool includePort);
	uint32			(*hash_address_pair)(const struct sockaddr* ourAddress,
						const struct sockaddr* peerAddress);

	status_t		(*checksum_address)(Checksum* checksum,
						const struct sockaddr* address);

	void			(*get_loopback_address)(struct sockaddr* result);
};


#endif	// NET_DATALINK_H

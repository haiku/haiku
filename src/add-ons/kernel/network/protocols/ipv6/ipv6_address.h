/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef IPV6_ADDRESS_H
#define IPV6_ADDRESS_H


#include <netinet6/in6.h>
#include <string.h>


extern struct net_address_module_info gIPv6AddressModule;


#define NET_IPV6_MODULE_NAME "network/protocols/ipv6/v1"


static inline bool
operator==(const in6_addr &a1, const in6_addr &a2)
{
	// TODO: optimize
	return !memcmp(&a1, &a2, sizeof(in6_addr));
}


#endif	// IPV6_ADDRESS_H

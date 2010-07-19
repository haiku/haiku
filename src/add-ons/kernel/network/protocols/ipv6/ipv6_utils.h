/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Atis Elsts, the.kfx@gmail.com
 */
#ifndef IPV6_UTILS_H
#define IPV6_UTILS_H


#include <netinet6/in6.h>
#include <netinet/in.h>
#include <net_stack.h>


const char *ip6_sprintf(const in6_addr *addr, char *dst,
	size_t size = INET6_ADDRSTRLEN);


static inline uint16
compute_checksum(uint8* _buffer, size_t length)
{
	uint16* buffer = (uint16*)_buffer;
	uint32 sum = 0;

	while (length >= 2) {
		sum += *buffer++;
		length -= 2;
	}

	return sum;
}


static inline uint16
ipv6_checksum(const struct in6_addr* source,
	const struct in6_addr* destination,
	uint16 length, uint16 protocol,
	uint16 checksum)
{
	uint32 sum = checksum;

	length = htons(length);
	protocol = htons(protocol);

	sum += compute_checksum((uint8*)source, sizeof(in6_addr));
	sum += compute_checksum((uint8*)destination, sizeof(in6_addr));
	sum += compute_checksum((uint8*)&length, sizeof(uint16));
	sum += compute_checksum((uint8*)&protocol, sizeof(uint16));

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~(uint16)sum;
}


#endif	// IPV6_UTILS_H

/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef IPV4_H
#define IPV4_H


#include <netinet/in.h>

#include <ByteOrder.h>


#define IPV4_VERSION			4

// fragment flags
#define IP_RESERVED_FLAG		0x8000
#define IP_DONT_FRAGMENT		0x4000
#define IP_MORE_FRAGMENTS		0x2000
#define IP_FRAGMENT_OFFSET_MASK	0x1fff


struct ipv4_header {
#if B_HOST_IS_LENDIAN == 1
	uint8		header_length : 4;	// header length in 32-bit words
	uint8		version : 4;
#else
	uint8		version : 4;
	uint8		header_length : 4;
#endif
	uint8		service_type;
	uint16		total_length;
	uint16		id;
	uint16		fragment_offset;
	uint8		time_to_live;
	uint8		protocol;
	uint16		checksum;
	in_addr_t	source;
	in_addr_t	destination;

	uint16 HeaderLength() const { return header_length << 2; }
	uint16 TotalLength() const { return ntohs(total_length); }
	uint16 FragmentOffset() const { return ntohs(fragment_offset); }
} _PACKED;


#endif	// IPV4_H

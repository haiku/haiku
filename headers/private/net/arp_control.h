/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARP_CONTROL_H
#define ARP_CONTROL_H


#include <ethernet.h>

#include <netinet/in.h>


// ARP flags
#define ARP_FLAG_LOCAL		0x01
#define ARP_FLAG_REJECT		0x02
#define ARP_FLAG_PERMANENT	0x04
#define ARP_FLAG_PUBLISH	0x08
#define ARP_FLAG_VALID		0x10


// generic syscall interface
#define ARP_SYSCALLS "network/arp"

#define ARP_SET_ENTRY		1
#define ARP_GET_ENTRY		2
#define ARP_GET_ENTRIES		3
#define ARP_DELETE_ENTRY	4
#define ARP_FLUSH_ENTRIES	5
#define ARP_IGNORE_REPLIES	6

struct arp_control {
	in_addr_t	address;
	uint8		ethernet_address[ETHER_ADDRESS_LENGTH];
	uint32		flags;
	uint32		cookie;
};

#endif	// ARP_CONTROL_H

/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ETHERNET_H
#define ETHERNET_H


#include <SupportDefs.h>


#define ETHER_ADDRESS_LENGTH	6
#define ETHER_CRC_LENGTH		4
#define ETHER_HEADER_LENGTH		14

#define ETHER_MIN_FRAME_SIZE	64
#define ETHER_MAX_FRAME_SIZE	1514

struct ether_header {
	uint8	destination[ETHER_ADDRESS_LENGTH];
	uint8	source[ETHER_ADDRESS_LENGTH];
	uint16	type;
} _PACKED;


// ethernet types
#define ETHER_TYPE_IP				0x0800
#define ETHER_TYPE_ARP				0x0806
#define ETHER_TYPE_IPX				0x8137
#define	ETHER_TYPE_IPV6				0x86dd
#define	ETHER_TYPE_PPPOE_DISCOVERY	0x8863	// PPPoE discovery stage
#define	ETHER_TYPE_PPPOE			0x8864	// PPPoE session stage


#endif	// ETHERNET_H

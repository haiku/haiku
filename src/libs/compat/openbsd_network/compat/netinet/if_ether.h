/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_NETINET_IF_ETHER_H_
#define _OBSD_COMPAT_NETINET_IF_ETHER_H_


#include_next <netinet/if_ether.h>
#include <net/if_arp.h>
#include <net/ethernet.h>

#include "if_ethersubr.h"


static const u_int8_t etheranyaddr[ETHER_ADDR_LEN] =
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


#define	ETHER_IS_EQ(a1, a2)	(memcmp((a1), (a2), ETHER_ADDR_LEN) == 0)

#define ETHERTYPE_EAPOL		ETHERTYPE_PAE


#endif	/* _OBSD_COMPAT_NETINET_IF_ETHER_H_ */

/*
 * Copyright 2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETINET6_IN6_H_
#define _NETINET6_IN6_H_


#include <sys/types.h>
#include <stdint.h>


struct in6_addr {
	uint8_t		s6_addr[16];
};

/*
 * IP Version 6 socket address.
 */

struct sockaddr_in6 {
	uint8_t		sin6_len;
	uint8_t		sin6_family;
	uint16_t	sin6_port;
	uint32_t	sin6_flowinfo;
	struct in6_addr	sin6_addr;
	uint32_t	sin6_scope_id;
};

#endif	/* _NETINET6_IN6_H_ */

/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_R5_COMPATIBILITY_H
#define NET_R5_COMPATIBILITY_H


#include <SupportDefs.h>


#define R5_SOCK_DGRAM	1
#define R5_SOCK_STREAM	2

#define R5_AF_INET		1

#define R5_IPPROTO_UDP	1
#define R5_IPPROTO_TCP	2
#define R5_IPPROTO_ICMP	3

// setsockopt() defines

#define R5_SOL_SOCKET	1

#define R5_SO_DEBUG		1
#define R5_SO_REUSEADDR	2
#define R5_SO_NONBLOCK	3
#define R5_SO_REUSEPORT	4
#define R5_SO_FIONREAD	5

#define R5_MSG_OOB		0x01


struct r5_sockaddr_in {
	uint16	sin_family;
	uint16	sin_port;
	uint32	sin_addr;
	char	sin_zero[4];
};

extern bool __gR5Compatibility;

#endif	// NET_R5_COMPATIBILITY_H

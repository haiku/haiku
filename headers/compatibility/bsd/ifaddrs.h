/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _IFADDRS_H
#define _IFADDRS_H


struct ifaddrs {
	struct ifaddrs  *ifa_next;    /* Next item in list */
	const char      *ifa_name;    /* Name of interface */
	unsigned int     ifa_flags;   /* Flags from SIOCGIFFLAGS */
	struct sockaddr *ifa_addr;    /* Address of interface */
	struct sockaddr *ifa_netmask; /* Netmask of interface */
	struct sockaddr *ifa_dstaddr;
	#define         ifa_broadaddr ifa_dstaddr
	void            *ifa_data;    /* Address-specific data */
};


int getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);


#endif

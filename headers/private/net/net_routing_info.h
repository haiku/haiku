/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_ROUTING_INFO_H
#define NET_ROUTING_INFO_H


struct net_routing_info {
	void *root;
	void *default_route;

	int addr_start;
	int addr_end;
};

#endif	// NET_ROUTING_INFO_H

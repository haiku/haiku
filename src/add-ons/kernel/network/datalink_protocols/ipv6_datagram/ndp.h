/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef IPV6_NDP_H
#define IPV6_NDP_H


#include <net_buffer.h>


struct net_ndp_module_info {
	module_info info;

	status_t	(*receive_data)(net_buffer* buffer);
};


#endif	// IPV6_NDP_H

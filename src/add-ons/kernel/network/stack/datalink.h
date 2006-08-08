/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DATALINK_H
#define DATALINK_H


#include <net_datalink.h>


status_t datalink_control(struct net_domain *domain, int32 option,
			void *value, size_t *_length);
status_t datalink_send_data(struct net_route *route, net_buffer *buffer);

#endif	// DATALINK_H

/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include <fs/node_monitor.h>


status_t
remove_node_listener(dev_t device, ino_t node, NotificationListener& listener)
{
	return B_NOT_SUPPORTED;
}


status_t
add_node_listener(dev_t device, ino_t node, uint32 flags,
	NotificationListener& listener)
{
	return B_NOT_SUPPORTED;
}

/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_NODE_MONITOR_H
#define _KERNEL_NODE_MONITOR_H


#include <fs_interface.h>


struct io_context;

#ifdef __cplusplus

// C++ only part

class NotificationListener;

extern status_t remove_node_listener(dev_t device, ino_t node,
	NotificationListener& listener);
extern status_t add_node_listener(dev_t device, ino_t node, uint32 flags,
	NotificationListener& listener);

extern "C" {
#endif

// private kernel API
extern status_t remove_node_monitors(struct io_context *context);
extern status_t node_monitor_init(void);
extern status_t notify_unmount(dev_t device);
extern status_t notify_mount(dev_t device, dev_t parentDevice,
					ino_t parentDirectory);

// user-space exported calls
extern status_t _user_stop_notifying(port_id port, uint32 token);
extern status_t _user_start_watching(dev_t device, ino_t node, uint32 flags,
					port_id port, uint32 token);
extern status_t _user_stop_watching(dev_t device, ino_t node, port_id port,
					uint32 token);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_NODE_MONITOR_H */

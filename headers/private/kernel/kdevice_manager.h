/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEVICE_MANAGER_H
#define _KERNEL_DEVICE_MANAGER_H


#include <SupportDefs.h>
#include <device_manager.h>


typedef struct device_node_info device_node_info;

// info about a device node
struct device_node_info {
	device_node_info	*prev, *next;
	device_node_info	*siblings_prev, *siblings_next;
	device_node_info	*notify_prev, *notify_next;
	device_node_info	*parent;
	device_node_info	*children;
	int32				ref_count;			// reference count; see registration.c
	driver_module_info	*driver;
	void				*cookie;
	bool				registered;			// true, if device is officially existent
	bool				init_finished;		// true, if publish_device has been completed
											// (sync with loader_lock)
	bool				blocked_by_rescan;	// true, if device is blocked because of rescan
	bool				verifying;			// true, if driver is being verified by rescan
	bool				redetected;			// true, if driver was redetected during rescan
	int32				num_waiting_hooks;	// number of waiting hook calls
	sem_id				hook_sem;			// sem for waiting hook calls
	int32				load_block_count;	// load-block nest count
	int32				num_blocked_loads;	// number of waiting load calls
	sem_id				load_block_sem;		// sem for waiting load calls
	int32				load_count;			// # of finished load_driver() calls
	int32				loading;			// # of active (un)load_driver() calls
	uint32				rescan_depth;		// > 0 if scanning is active
	struct device_attr_info *attributes;	// list of attributes
	uint32				num_io_resources;	// number of I/O resources
	io_resource_handle	*io_resources;		// array of I/O resource (NULL-terminated)
	int32				defer_probing; 		// > 0 defer probing for consumers of children
	bool				automatically_loaded;	// loaded automatically because PNP_DRIVER_ALWAYS_LOADED
	device_node_info	*unprobed_children; // list of un-probed children
	device_node_info	*unprobed_prev, *unprobed_next; // link for unprobed_children
};


struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

extern status_t probe_for_device_type(const char *type);
extern status_t device_manager_init(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_DEVICE_MANAGER_H */

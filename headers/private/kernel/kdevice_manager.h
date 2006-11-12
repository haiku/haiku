/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEVICE_MANAGER_H
#define _KERNEL_DEVICE_MANAGER_H


#include <SupportDefs.h>
#include <device_manager.h>

#include <util/list.h>


typedef struct device_node_info device_node_info;

// info about a device node
struct device_node_info {
	struct list_link	siblings;
	device_node_info	*parent;
	struct list			children;
	int32				ref_count;

	driver_module_info	*driver;
	void				*cookie;

	bool				registered;			// true, if device is officially existent
	bool				blocked_by_rescan;	// true, if device is blocked because of rescan
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
	bool				automatically_loaded;	// loaded automatically because PNP_DRIVER_ALWAYS_LOADED
	uint32				internal_id;	// internal identifier
};


struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

extern status_t probe_for_device_type(const char *type);
extern status_t device_manager_init(struct kernel_args *args);

// temporary/optional device manager syscall API
#define DEVICE_MANAGER_SYSCALLS "device_manager"

#define DM_GET_ROOT			1
#define DM_GET_CHILD			2
#define DM_GET_NEXT_CHILD		3
#define DM_GET_NEXT_ATTRIBUTE		4

struct dev_attr {
	uint32 		node_cookie;
	uint32		cookie;
	char		name[255];
	type_code	type;
	union {
		uint8   ui8;
		uint16  ui16;
		uint32  ui32;
		uint64  ui64;
		char    string[255];
		struct {
			void    *data;
			size_t  length;
		} raw;
	} value;
};

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_DEVICE_MANAGER_H */

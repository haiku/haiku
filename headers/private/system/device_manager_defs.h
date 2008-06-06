/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_DEVICE_MANAGER_DEFS_H
#define _SYSTEM_DEVICE_MANAGER_DEFS_H


#include <device_manager.h>


// temporary/optional device manager syscall API
#define DEVICE_MANAGER_SYSCALLS "device_manager"

#define DM_GET_ROOT				1
#define DM_GET_CHILD			2
#define DM_GET_NEXT_CHILD		3
#define DM_GET_NEXT_ATTRIBUTE	4

typedef addr_t device_node_cookie;

struct device_attr_info {
	device_node_cookie 		node_cookie;
	device_node_cookie		cookie;
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

#endif	/* _SYSTEM_DEVICE_MANAGER_DEFS_H */

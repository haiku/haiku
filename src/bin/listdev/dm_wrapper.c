/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <drivers/device_manager.h>
#include <device_manager_defs.h>
#include <generic_syscall_defs.h>
#include <string.h>
#include <syscalls.h>

#include "dm_wrapper.h"


status_t init_dm_wrapper(void)
{
	uint32 version = 0;
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, B_SYSCALL_INFO,
		&version, sizeof(version));
}


status_t uninit_dm_wrapper(void)
{
	return B_OK;
}


status_t
get_root(device_node_cookie *cookie)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_ROOT,
		cookie, sizeof(device_node_cookie));
}


status_t
get_child(device_node_cookie *device)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_CHILD,
		device, sizeof(device_node_cookie));
}


status_t
get_next_child(device_node_cookie *device)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_NEXT_CHILD,
		device, sizeof(device_node_cookie));
}


status_t
dm_get_next_attr(struct device_attr_info *attr)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_NEXT_ATTRIBUTE,
		attr, sizeof(struct device_attr_info));
}

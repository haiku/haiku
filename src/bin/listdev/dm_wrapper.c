#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <drivers/device_manager.h>
#include <generic_syscall.h>
#include <string.h>
#include <syscalls.h>

#include "dm_wrapper.h"


status_t init_dm_wrapper(void)
{
	uint32 version = 0;
        return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, B_SYSCALL_INFO, &version, sizeof(version));
}

status_t uninit_dm_wrapper(void)
{
	return B_OK;
}

status_t
get_root(uint32 *cookie)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_ROOT, cookie, sizeof(uint32));
}

status_t 
get_child(uint32 *device)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_CHILD, device, sizeof(uint32));
}

status_t 
get_next_child(uint32 *device)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_NEXT_CHILD, device, sizeof(uint32));
}

status_t 
dm_get_next_attr(struct dev_attr *attr)
{
	return _kern_generic_syscall(DEVICE_MANAGER_SYSCALLS, DM_GET_NEXT_ATTRIBUTE, attr, sizeof(struct dev_attr));
}


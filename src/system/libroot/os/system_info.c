/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <syscalls.h>
#include <system_info.h>


status_t
_get_system_info(system_info *info, size_t size)
{
	if (info == NULL || size != sizeof(system_info))
		return B_BAD_VALUE;

	return _kern_get_system_info(info, size);
}


status_t
__get_system_info_etc(int32 id, void *info, size_t size)
{
	if (info == NULL || size == 0 || id < 0)
		return B_BAD_VALUE;

	return _kern_get_system_info_etc(id, info, size);
}


status_t
__start_watching_system(int32 object, uint32 flags, port_id port, int32 token)
{
	return _kern_start_watching_system(object, flags, port, token);
}


status_t
__stop_watching_system(int32 object, uint32 flags, port_id port, int32 token)
{
	return _kern_stop_watching_system(object, flags, port, token);
}


int32
is_computer_on(void)
{
	return _kern_is_computer_on();
}


double
is_computer_on_fire(void)
{
	return 0.63739;
}


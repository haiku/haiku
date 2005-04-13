/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


status_t
_get_system_info(system_info *info, size_t size)
{
	if (info == NULL || size != sizeof(system_info))
		return B_BAD_VALUE;

	return _kern_get_system_info(info, size);
}


int32
is_computer_on(void)
{
	return 1;
}


double
is_computer_on_fire(void)
{
	return 0;
}


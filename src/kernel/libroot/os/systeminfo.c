/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


status_t
get_cpuid(cpuid_info *info, uint32 eax_register, uint32 cpu_num)
{
	// ToDo: get_cpuid()
	return B_ERROR;
}


status_t
_get_system_info(system_info *returned_info, size_t size)
{
	// ToDo: _get_system_info()
	return B_ERROR;
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


/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <real_time_data.h>
#include <arch_cpu.h>


void
__arch_init_time(struct real_time_data *data)
{
	// ToDo: this should only store a pointer to that value
	// ToDo: this function should not clobber the global name space
	setup_system_time(data->system_time_conversion_factor);
}


/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <real_time_data.h>
#include <arch_cpu.h>


void
__arch_init_time(struct real_time_data *data, bool setDefaults)
{
	if (setDefaults) {
		data->arch_data.system_time_offset = 0;
		data->arch_data.system_time_conversion_factor = 100000;
	}

	// ToDo: this should only store a pointer to that value
	__x86_setup_system_time(data->arch_data.system_time_conversion_factor);
}


bigtime_t
__arch_get_system_time_offset(struct real_time_data *data)
{
	return atomic_get64(&data->arch_data.system_time_offset);
}


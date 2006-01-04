/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>


void
__arch_init_time(struct real_time_data *data, bool setDefaults)
{
	if (setDefaults) {
		data->arch_data.data[0].system_time_offset = 0;
		data->arch_data.system_time_conversion_factor = 1000000000LL;
		data->arch_data.version = 0;
	}

	__ppc_setup_system_time(&data->arch_data.system_time_conversion_factor);
}

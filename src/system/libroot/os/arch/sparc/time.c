/* 
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>


static struct arch_real_time_data *sRealTimeData;

void
__arch_init_time(struct real_time_data *data, bool setDefaults)
{
	sRealTimeData = &data->arch_data;

	if (setDefaults) {
		sRealTimeData->system_time_conversion_factor = 1000000000LL;
	}

	// __sparc_setup_system_time(&sRealTimeData->system_time_conversion_factor);
}


bigtime_t
__arch_get_system_time_offset(struct real_time_data *data)
{
	return 0;
}

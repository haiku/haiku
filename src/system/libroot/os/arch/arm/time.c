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
		sRealTimeData->data[0].system_time_offset = 0;
		sRealTimeData->system_time_conversion_factor = 1000000000LL;
		sRealTimeData->version = 0;
	}

	__arm_setup_system_time(&sRealTimeData->system_time_conversion_factor);
}


bigtime_t
__arch_get_system_time_offset(struct real_time_data *data)
{
	int32 version;
	bigtime_t offset;
	do {
		version = sRealTimeData->version;
		offset = sRealTimeData->data[version % 2].system_time_offset;
	} while (version != sRealTimeData->version);

	return offset;
}


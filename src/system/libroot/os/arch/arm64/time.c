/* 
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>


void
__arch_init_time(struct real_time_data *data, bool setDefaults)
{
}


bigtime_t
__arch_get_system_time_offset(struct real_time_data *data)
{
	return 0;
}

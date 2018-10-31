/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <compat/sys/kernel.h>


int32_t
get_ticks()
{
	return usecs_to_ticks(system_time());
}

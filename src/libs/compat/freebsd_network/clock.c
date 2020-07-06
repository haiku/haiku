/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <compat/sys/kernel.h>


int32_t
_get_ticks()
{
	return USEC_2_TICKS(system_time());
}

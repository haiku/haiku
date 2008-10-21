/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>


static vint32 *sConversionFactor;

void
__m68k_setup_system_time(vint32 *cvFactor)
{
	sConversionFactor = cvFactor;
}


//XXX: this is a hack
// remove me when platform code works
int64
__m68k_get_time_base(void)
{
	static uint64 time_dilation_field = 0;
	return time_dilation_field++;
}

bigtime_t
system_time(void)
{
	uint64 timeBase = __m68k_get_time_base();

	uint32 cv = *sConversionFactor;
	return (timeBase >> 32) * cv + (((timeBase & 0xffffffff) * cv) >> 32);
}

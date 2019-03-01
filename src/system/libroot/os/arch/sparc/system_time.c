/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */

#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>

static vint32 *sConversionFactor;

//XXX: this is a hack
// remove me when platform code works
static int64
__sparc_get_time_base(void)
{
	static uint64 time_dilation_field = 0;
	return time_dilation_field++;
}

bigtime_t
system_time(void)
{
	uint64 timeBase = __sparc_get_time_base();

	uint32 cv = sConversionFactor ? *sConversionFactor : 0;
	return (timeBase >> 32) * cv + (((timeBase & 0xffffffff) * cv) >> 32);
}

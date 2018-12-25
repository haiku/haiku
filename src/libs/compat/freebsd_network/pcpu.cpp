/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <smp.h>

extern "C" {
#include <compat/sys/pcpu.h>
};


int32_t
get_curcpu()
{
	return smp_get_current_cpu();
}

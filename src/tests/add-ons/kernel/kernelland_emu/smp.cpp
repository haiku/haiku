/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//!	This is only needed for the debug build.


#include <cpu.h>
#include <smp.h>


#ifdef acquire_spinlock
#	undef acquire_spinlock
#endif


cpu_ent gCPU[8];


extern "C" void
acquire_spinlock(spinlock* lock)
{
}


extern "C" int32
smp_get_current_cpu()
{
	return 0;
}

/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//!	This is only needed for the debug build.


#if __GNUC__ == 2
typedef struct list_link list_link;
	// for x86_gcc2h, this typedef must be visible when DoublyLinkedListCLink is defined,
	// in order to define struct cpu_ent in cpu.h
#endif // __GNUC__ == 2

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

/*
 * Copyright 2014-2016 Haiku, Inc. All rights reserved.
 * Copyright 2013-2014, Fredrik Holmqvist, fredrik.holmqvist@gmail.com.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <arch/x86/descriptors.h>


extern uint64 gLongGDT;
extern uint64 gLongGDTR;
segment_descriptor gBootGDT[BOOT_GDT_SEGMENT_COUNT];


static void
long_gdt_init()
{
	clear_segment_descriptor(&gBootGDT[0]);

	// Set up code/data segments (TSS segments set up later in the kernel).
	set_segment_descriptor(&gBootGDT[KERNEL_CODE_SEGMENT], DT_CODE_EXECUTE_ONLY,
		DPL_KERNEL);
	set_segment_descriptor(&gBootGDT[KERNEL_DATA_SEGMENT], DT_DATA_WRITEABLE,
		DPL_KERNEL);
	set_segment_descriptor(&gBootGDT[USER_CODE_SEGMENT], DT_CODE_EXECUTE_ONLY,
		DPL_USER);
	set_segment_descriptor(&gBootGDT[USER_DATA_SEGMENT], DT_DATA_WRITEABLE,
		DPL_USER);

	// Used by long_enter_kernel().
	gLongGDT = (addr_t)gBootGDT + 0xFFFFFF0000000000;
	dprintf("GDT at 0x%lx\n", gLongGDT);
}


void
arch_mmu_init()
{
	long_gdt_init();
}

/*
 * Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/x86/apic.h>
#include <arch/x86/msi.h>

#include <debug.h>
#include <vm/vm.h>

#include "timers/apic_timer.h"


static void *sLocalAPIC = NULL;


bool
apic_available()
{
	return sLocalAPIC != NULL;
}


uint32
apic_read(uint32 offset)
{
	return *(volatile uint32 *)((char *)sLocalAPIC + offset);
}


void
apic_write(uint32 offset, uint32 data)
{
	*(volatile uint32 *)((char *)sLocalAPIC + offset) = data;
}


uint32
apic_local_id()
{
	return (apic_read(APIC_ID) & 0xffffffff) >> 24;
}


void
apic_end_of_interrupt()
{
	apic_write(APIC_EOI, 0);
}


void
apic_disable_local_ints()
{
	// just clear them out completely
	apic_write(APIC_LVT_LINT0, APIC_LVT_MASKED);
	apic_write(APIC_LVT_LINT1, APIC_LVT_MASKED);
}


status_t
apic_init(kernel_args *args)
{
	if (args->arch_args.apic == NULL)
		return B_NO_INIT;

	sLocalAPIC = args->arch_args.apic;
	dprintf("mapping local apic at %p\n", sLocalAPIC);
	if (vm_map_physical_memory(B_SYSTEM_TEAM, "local apic", &sLocalAPIC,
			B_EXACT_ADDRESS, B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			args->arch_args.apic_phys, true) < 0) {
		panic("mapping the local apic failed");
		return B_ERROR;
	}

	return B_OK;
}


status_t
apic_per_cpu_init(kernel_args *args, int32 cpu)
{
	dprintf("setting up apic for CPU %" B_PRId32 ": apic id %" B_PRIu32 ", "
		"version %" B_PRIu32 "\n", cpu, apic_local_id(),
		apic_read(APIC_VERSION));

	/* set spurious interrupt vector to 0xff */
	uint32 config = apic_read(APIC_SPURIOUS_INTR_VECTOR) & 0xffffff00;
	config |= APIC_ENABLE | 0xff;
	apic_write(APIC_SPURIOUS_INTR_VECTOR, config);

	// don't touch the LINT0/1 configuration in virtual wire mode
	// ToDo: implement support for other modes...
#if 0
	if (cpu == 0) {
		/* setup LINT0 as ExtINT */
		config = (apic_read(APIC_LINT0) & 0xffff00ff);
		config |= APIC_LVT_DM_ExtINT | APIC_LVT_IIPP | APIC_LVT_TM;
		apic_write(APIC_LINT0, config);

		/* setup LINT1 as NMI */
		config = (apic_read(APIC_LINT1) & 0xffff00ff);
		config |= APIC_LVT_DM_NMI | APIC_LVT_IIPP;
		apic_write(APIC_LINT1, config);
	}
	if (cpu > 0) {
		dprintf("LINT0: %p\n", (void *)apic_read(APIC_LINT0));
		dprintf("LINT1: %p\n", (void *)apic_read(APIC_LINT1));

		/* disable LINT0/1 */
		config = apic_read(APIC_LINT0);
		apic_write(APIC_LINT0, config | APIC_LVT_MASKED);

		config = apic_read(APIC_LINT1);
		apic_write(APIC_LINT1, config | APIC_LVT_MASKED);
	} else {
		dprintf("0: LINT0: %p\n", (void *)apic_read(APIC_LINT0));
		dprintf("0: LINT1: %p\n", (void *)apic_read(APIC_LINT1));
	}
#endif

	apic_timer_per_cpu_init(args, cpu);

	/* setup error vector to 0xfe */
	config = (apic_read(APIC_LVT_ERROR) & 0xffffff00) | 0xfe;
	apic_write(APIC_LVT_ERROR, config);

	/* accept all interrupts */
	config = apic_read(APIC_TASK_PRIORITY) & 0xffffff00;
	apic_write(APIC_TASK_PRIORITY, config);

	config = apic_read(APIC_SPURIOUS_INTR_VECTOR);
	apic_end_of_interrupt();

	return B_OK;
}

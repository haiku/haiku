/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/kernel_args.h>
#include <vm.h>
#include <int.h>
#include <smp.h>
#include <smp_priv.h>

#include <arch/cpu.h>
#include <arch/vm.h>
#include <arch/smp.h>

#include <timer.h>

#include <arch/x86/smp_priv.h>
#include <arch/x86/arch_apic.h>
#include <arch/x86/timer.h>

#include <string.h>
#include <stdio.h>


//#define TRACE_ARCH_SMP
#ifdef TRACE_ARCH_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static void *sLocalAPIC = NULL;
static uint32 sCPUAPICIds[B_MAX_CPU_COUNT];
static uint32 sCPUOSIds[B_MAX_CPU_COUNT];
static uint32 sAPICVersions[B_MAX_CPU_COUNT];

extern bool gUsingIOAPIC;

static uint32
apic_read(uint32 offset)
{
	return *(volatile uint32 *)((char *)sLocalAPIC + offset);
}


static void
apic_write(uint32 offset, uint32 data)
{
	*(volatile uint32 *)((char *)sLocalAPIC + offset) = data;
}


static status_t
setup_apic(kernel_args *args, int32 cpu)
{
	uint32 config;

	TRACE(("setting up the APIC for CPU %ld...\n", cpu));
	TRACE(("	apic id %d, version %d\n", apic_read(APIC_ID), apic_read(APIC_VERSION)));

	/* set spurious interrupt vector to 0xff */
	config = apic_read(APIC_SPURIOUS_INTR_VECTOR) & 0xffffff00;
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

	apic_smp_init_timer(args, cpu);

	/* setup error vector to 0xfe */
	config = (apic_read(APIC_LVT_ERROR) & 0xffffff00) | 0xfe;
	apic_write(APIC_LVT_ERROR, config);

	/* accept all interrupts */
	config = apic_read(APIC_TASK_PRIORITY) & 0xffffff00;
	apic_write(APIC_TASK_PRIORITY, config);

	config = apic_read(APIC_SPURIOUS_INTR_VECTOR);
	apic_write(APIC_EOI, 0);

	return B_OK;
}


static int32
i386_ici_interrupt(void *data)
{
	// genuine inter-cpu interrupt
	TRACE(("inter-cpu interrupt on cpu %ld\n", smp_get_current_cpu()));

	// if we are not using the IO APIC we need to acknowledge the
	// interrupt ourselfs
	if (!gUsingIOAPIC)
		apic_write(APIC_EOI, 0);

	return smp_intercpu_int_handler();
}


static int32
i386_spurious_interrupt(void *data)
{
	// spurious interrupt
	TRACE(("spurious interrupt on cpu %ld\n", smp_get_current_cpu()));

	// spurious interrupts must not be acknowledged as it does not expect
	// a end of interrupt - if we still do it we would loose the next best
	// interrupt
	return B_HANDLED_INTERRUPT;
}


static int32
i386_smp_error_interrupt(void *data)
{
	// smp error interrupt
	TRACE(("smp error interrupt on cpu %ld\n", smp_get_current_cpu()));

	// if we are not using the IO APIC we need to acknowledge the
	// interrupt ourselfs
	if (!gUsingIOAPIC)
		apic_write(APIC_EOI, 0);

	return B_HANDLED_INTERRUPT;
}


status_t
arch_smp_init(kernel_args *args)
{
	TRACE(("arch_smp_init: entry\n"));

	if (args->arch_args.apic == NULL)
		return B_OK;

	sLocalAPIC = args->arch_args.apic;

	// setup some globals
	memcpy(sCPUAPICIds, args->arch_args.cpu_apic_id, sizeof(args->arch_args.cpu_apic_id));
	memcpy(sCPUOSIds, args->arch_args.cpu_os_id, sizeof(args->arch_args.cpu_os_id));
	memcpy(sAPICVersions, args->arch_args.cpu_apic_version, sizeof(args->arch_args.cpu_apic_version));

	// set up the local apic on the boot cpu
	arch_smp_per_cpu_init(args, 0);

	if (args->num_cpus > 1) {
		// I/O interrupts start at ARCH_INTERRUPT_BASE, so all interrupts are shifted
		install_io_interrupt_handler(0xfd - ARCH_INTERRUPT_BASE, &i386_ici_interrupt, NULL, B_NO_LOCK_VECTOR);
		install_io_interrupt_handler(0xfe - ARCH_INTERRUPT_BASE, &i386_smp_error_interrupt, NULL, B_NO_LOCK_VECTOR);
		install_io_interrupt_handler(0xff - ARCH_INTERRUPT_BASE, &i386_spurious_interrupt, NULL, B_NO_LOCK_VECTOR);
	}

	return B_OK;
}


status_t
arch_smp_per_cpu_init(kernel_args *args, int32 cpu)
{
	// set up the local apic on the current cpu
	TRACE(("arch_smp_init_percpu: setting up the apic on cpu %ld\n", cpu));
	setup_apic(args, cpu);

	return B_OK;
}


void
arch_smp_send_broadcast_ici(void)
{
	uint32 config;
	cpu_status state = disable_interrupts();

	config = apic_read(APIC_INTR_COMMAND_1) & APIC_INTR_COMMAND_1_MASK;
	apic_write(APIC_INTR_COMMAND_1, config | 0xfd | APIC_DELIVERY_MODE_FIXED
		| APIC_INTR_COMMAND_1_ASSERT
		| APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL
		| APIC_INTR_COMMAND_1_DEST_ALL_BUT_SELF);

	restore_interrupts(state);
}


void
arch_smp_send_ici(int32 target_cpu)
{
	uint32 config;
	uint32 timeout;
	cpu_status state;

	state = disable_interrupts();

	config = apic_read(APIC_INTR_COMMAND_2) & APIC_INTR_COMMAND_2_MASK;
	apic_write(APIC_INTR_COMMAND_2, config | sCPUAPICIds[target_cpu] << 24);

	config = apic_read(APIC_INTR_COMMAND_1) & APIC_INTR_COMMAND_1_MASK;
	apic_write(APIC_INTR_COMMAND_1, config | 0xfd | APIC_DELIVERY_MODE_FIXED
		| APIC_INTR_COMMAND_1_ASSERT
		| APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL
		| APIC_INTR_COMMAND_1_DEST_FIELD);

	timeout = 100000000;
	// wait for message to be sent
	while ((apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) != 0 && --timeout != 0)
		asm volatile ("pause;");

	if (timeout == 0)
		panic("arch_smp_send_ici: timeout, target_cpu %ld", target_cpu);

	restore_interrupts(state);
}

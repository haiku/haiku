/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/kernel_args.h>
#include <vm/vm.h>
#include <int.h>
#include <smp.h>
#include <smp_priv.h>

#include <arch/cpu.h>
#include <arch/vm.h>
#include <arch/smp.h>

#include <arch/x86/apic.h>
#include <arch/x86/arch_smp.h>
#include <arch/x86/smp_priv.h>
#include <arch/x86/timer.h>

#include <string.h>
#include <stdio.h>


//#define TRACE_ARCH_SMP
#ifdef TRACE_ARCH_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static uint32 sCPUAPICIds[B_MAX_CPU_COUNT];
static uint32 sAPICVersions[B_MAX_CPU_COUNT];


static int32
x86_ici_interrupt(void *data)
{
	// genuine inter-cpu interrupt
	int cpu = smp_get_current_cpu();
	TRACE(("inter-cpu interrupt on cpu %d\n", cpu));
	return smp_intercpu_int_handler(cpu);
}


static int32
x86_spurious_interrupt(void *data)
{
	// spurious interrupt
	TRACE(("spurious interrupt on cpu %ld\n", smp_get_current_cpu()));

	// spurious interrupts must not be acknowledged as it does not expect
	// a end of interrupt - if we still do it we would loose the next best
	// interrupt
	return B_HANDLED_INTERRUPT;
}


static int32
x86_smp_error_interrupt(void *data)
{
	// smp error interrupt
	TRACE(("smp error interrupt on cpu %ld\n", smp_get_current_cpu()));
	return B_HANDLED_INTERRUPT;
}


status_t
arch_smp_init(kernel_args *args)
{
	TRACE(("%s: entry\n", __func__));

	if (!apic_available()) {
		// if we don't have an apic we can't do smp
		TRACE(("%s: apic not available for smp\n", __func__));
		return B_OK;
	}

	// setup some globals
	memcpy(sCPUAPICIds, args->arch_args.cpu_apic_id, sizeof(args->arch_args.cpu_apic_id));
	memcpy(sAPICVersions, args->arch_args.cpu_apic_version, sizeof(args->arch_args.cpu_apic_version));

	// set up the local apic on the boot cpu
	arch_smp_per_cpu_init(args, 0);

	if (args->num_cpus > 1) {
		// I/O interrupts start at ARCH_INTERRUPT_BASE, so all interrupts are shifted
		reserve_io_interrupt_vectors(3, 0xfd - ARCH_INTERRUPT_BASE);
		install_io_interrupt_handler(0xfd - ARCH_INTERRUPT_BASE, &x86_ici_interrupt, NULL, B_NO_LOCK_VECTOR);
		install_io_interrupt_handler(0xfe - ARCH_INTERRUPT_BASE, &x86_smp_error_interrupt, NULL, B_NO_LOCK_VECTOR);
		install_io_interrupt_handler(0xff - ARCH_INTERRUPT_BASE, &x86_spurious_interrupt, NULL, B_NO_LOCK_VECTOR);
	}

	return B_OK;
}


status_t
arch_smp_per_cpu_init(kernel_args *args, int32 cpu)
{
	// set up the local apic on the current cpu
	TRACE(("arch_smp_init_percpu: setting up the apic on cpu %ld\n", cpu));
	apic_per_cpu_init(args, cpu);

	// setup FPU and SSE if supported
	x86_init_fpu();

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

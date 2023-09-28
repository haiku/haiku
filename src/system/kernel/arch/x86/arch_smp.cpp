/*
 * Copyright 2023, Puck Meerburg, puck@puckipedia.com.
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/kernel_args.h>
#include <vm/vm.h>
#include <cpu.h>
#include <int.h>
#include <smp.h>
#include <smp_priv.h>

#include <arch/atomic.h>
#include <arch/cpu.h>
#include <arch/vm.h>
#include <arch/smp.h>

#include <arch/x86/apic.h>
#include <arch/x86/arch_smp.h>
#include <arch/x86/smp_priv.h>
#include <arch/x86/timer.h>

#include <string.h>
#include <stdio.h>

#include <algorithm>


//#define TRACE_ARCH_SMP
#ifdef TRACE_ARCH_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define	ICI_VECTOR		0xfd


static uint32 sCPUAPICIds[SMP_MAX_CPUS];
static uint32 sAPICVersions[SMP_MAX_CPUS];


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
	TRACE(("spurious interrupt on cpu %" B_PRId32 "\n", smp_get_current_cpu()));

	// spurious interrupts must not be acknowledged as it does not expect
	// a end of interrupt - if we still do it we would loose the next best
	// interrupt
	return B_HANDLED_INTERRUPT;
}


static int32
x86_smp_error_interrupt(void *data)
{
	// smp error interrupt
	TRACE(("smp error interrupt on cpu %" B_PRId32 "\n", smp_get_current_cpu()));
	return B_HANDLED_INTERRUPT;
}


uint32
x86_get_cpu_apic_id(int32 cpu)
{
	ASSERT(cpu >= 0 && cpu < SMP_MAX_CPUS);
	return sCPUAPICIds[cpu];
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
		reserve_io_interrupt_vectors(3, 0xfd - ARCH_INTERRUPT_BASE,
			INTERRUPT_TYPE_ICI);
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
	TRACE(("arch_smp_init_percpu: setting up the apic on cpu %" B_PRId32 "\n",
		cpu));
	apic_per_cpu_init(args, cpu);

	// setup FPU and SSE if supported
	x86_init_fpu();

	return B_OK;
}


static void
send_multicast_ici_physical(CPUSet& cpuSet)
{
	int32 cpuCount = smp_get_num_cpus();
	int32 currentCpu = smp_get_current_cpu();

	for (int32 i = 0; i < cpuCount; i++) {
		if (cpuSet.GetBit(i) && i != currentCpu) {
			uint32 destination = sCPUAPICIds[i];
			uint32 mode = ICI_VECTOR | APIC_DELIVERY_MODE_FIXED
					| APIC_INTR_COMMAND_1_ASSERT
					| APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL
					| APIC_INTR_COMMAND_1_DEST_FIELD;

			while (!apic_interrupt_delivered())
				cpu_pause();
			apic_set_interrupt_command(destination, mode);
		}
	}
}


void
arch_smp_send_multicast_ici(CPUSet& cpuSet)
{
#if KDEBUG
	if (are_interrupts_enabled())
		panic("arch_smp_send_multicast_ici: called with interrupts enabled");
#endif

	memory_write_barrier();

	if (!x2apic_available()) {
		send_multicast_ici_physical(cpuSet);
		return;
	}

	// WRMSR on the x2APIC MSRs is neither serializing, nor a load-store
	// operation, requiring both memory serialization *and* a load fence, which is
	// the only way to ensure the MSR doesn't get executed before the write
	// barrier.
	memory_read_barrier();

	int32 cpuCount = smp_get_num_cpus();
	int32 currentCpu = smp_get_current_cpu();

	uint32 mode = ICI_VECTOR | APIC_DELIVERY_MODE_FIXED
			| APIC_INTR_COMMAND_1_ASSERT
			| APIC_INTR_COMMAND_1_DEST_MODE_LOGICAL
			| APIC_INTR_COMMAND_1_DEST_FIELD;

	for (int32 i = 0; i < cpuCount; i++) {
		if (!cpuSet.GetBit(i) || i == currentCpu)
			continue;

		apic_set_interrupt_command(gCPU[i].arch.logical_apic_id, mode);
	}
}


void
arch_smp_send_broadcast_ici(void)
{
#if KDEBUG
	if (are_interrupts_enabled())
		panic("arch_smp_send_broadcast_ici: called with interrupts enabled");
#endif

	memory_write_barrier();

	uint32 mode = ICI_VECTOR | APIC_DELIVERY_MODE_FIXED
			| APIC_INTR_COMMAND_1_ASSERT
			| APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL
			| APIC_INTR_COMMAND_1_DEST_ALL_BUT_SELF;

	while (!apic_interrupt_delivered())
		cpu_pause();
	apic_set_interrupt_command(0, mode);
}


void
arch_smp_send_ici(int32 target_cpu)
{
#if KDEBUG
	if (are_interrupts_enabled())
		panic("arch_smp_send_ici: called with interrupts enabled");
#endif

	memory_write_barrier();

	uint32 destination = sCPUAPICIds[target_cpu];
	uint32 mode = ICI_VECTOR | APIC_DELIVERY_MODE_FIXED
			| APIC_INTR_COMMAND_1_ASSERT
			| APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL
			| APIC_INTR_COMMAND_1_DEST_FIELD;

	while (!apic_interrupt_delivered())
		cpu_pause();
	apic_set_interrupt_command(destination, mode);
}


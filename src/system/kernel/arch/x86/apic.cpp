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
#include <kernel/cpu.h>
#include <safemode.h>
#include <vm/vm.h>
#include <util/AutoLock.h>

#include "timers/apic_timer.h"


static void *sLocalAPIC = NULL;
static bool sX2APIC = false;


bool
apic_available()
{
	return sLocalAPIC != NULL || sX2APIC;
}


bool
x2apic_available()
{
	return sX2APIC;
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
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_ID);
	else
		return (apic_read(APIC_ID) & 0xffffffff) >> 24;
}


uint32
apic_version()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_VERSION);
	else
		return apic_read(APIC_VERSION);
}


uint32
apic_task_priority()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_TASK_PRIORITY);
	else
		return apic_read(APIC_TASK_PRIORITY);
}


void
apic_set_task_priority(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_TASK_PRIORITY, config);
	else
		apic_write(APIC_TASK_PRIORITY, config);
}


void
apic_end_of_interrupt()
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_EOI, 0);
	else
		apic_write(APIC_EOI, 0);
}


uint32
apic_logical_apic_id()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_LOGICAL_DEST);
	else
		return apic_read(APIC_LOGICAL_DEST);
}


void
apic_disable_local_ints()
{
	// just clear them out completely
	if (sX2APIC) {
		x86_write_msr(IA32_MSR_APIC_LVT_LINT0, APIC_LVT_MASKED);
		x86_write_msr(IA32_MSR_APIC_LVT_LINT1, APIC_LVT_MASKED);
	} else {
		apic_write(APIC_LVT_LINT0, APIC_LVT_MASKED);
		apic_write(APIC_LVT_LINT1, APIC_LVT_MASKED);
	}
}


uint32
apic_spurious_intr_vector()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_SPURIOUS_INTR_VECTOR);
	else
		return apic_read(APIC_SPURIOUS_INTR_VECTOR);
}


void
apic_set_spurious_intr_vector(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_SPURIOUS_INTR_VECTOR, config);
	else
		apic_write(APIC_SPURIOUS_INTR_VECTOR, config);
}


void
apic_set_interrupt_command(uint32 destination, uint32 mode)
{
	if (sX2APIC) {
		uint64 command = x86_read_msr(IA32_MSR_APIC_INTR_COMMAND);
		command &= APIC_INTR_COMMAND_1_MASK;
		command |= (uint64)destination << 32;
		command |= mode;
		x86_write_msr(IA32_MSR_APIC_INTR_COMMAND, command);
	} else {
		uint32 command2 = apic_read(APIC_INTR_COMMAND_2)
				& APIC_INTR_COMMAND_2_MASK;
		command2 |= destination << 24;
		apic_write(APIC_INTR_COMMAND_2, command2);

		uint32 command1 = apic_read(APIC_INTR_COMMAND_1)
				& APIC_INTR_COMMAND_1_MASK;
		command1 |= mode;
		apic_write(APIC_INTR_COMMAND_1, command1);
	}
}


bool
apic_interrupt_delivered(void)
{
	if (sX2APIC)
		return true;
	else
		return (apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) == 0;
}


uint32
apic_lvt_timer()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_LVT_TIMER);
	else
		return apic_read(APIC_LVT_TIMER);
}


void
apic_set_lvt_timer(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_LVT_TIMER, config);
	else
		apic_write(APIC_LVT_TIMER, config);
}


uint32
apic_lvt_error()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_LVT_ERROR);
	else
		return apic_read(APIC_LVT_ERROR);
}


void
apic_set_lvt_error(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_LVT_ERROR, config);
	else
		apic_write(APIC_LVT_ERROR, config);
}


uint32
apic_lvt_initial_timer_count()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_INITIAL_TIMER_COUNT);
	else
		return apic_read(APIC_INITIAL_TIMER_COUNT);
}


void
apic_set_lvt_initial_timer_count(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_INITIAL_TIMER_COUNT, config);
	else
		apic_write(APIC_INITIAL_TIMER_COUNT, config);
}


uint32
apic_lvt_timer_divide_config()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_TIMER_DIVIDE_CONFIG);
	else
		return apic_read(APIC_TIMER_DIVIDE_CONFIG);
}


void
apic_set_lvt_timer_divide_config(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_TIMER_DIVIDE_CONFIG, config);
	else
		apic_write(APIC_TIMER_DIVIDE_CONFIG, config);
}


status_t
apic_init(kernel_args *args)
{
	if (args->arch_args.apic == NULL)
		return B_NO_INIT;

	uint64 apic_base = x86_read_msr(IA32_MSR_APIC_BASE);

	if (x86_check_feature(IA32_FEATURE_EXT_X2APIC, FEATURE_EXT)
		&& (x86_check_feature(IA32_FEATURE_EXT_HYPERVISOR, FEATURE_EXT)
			|| ((apic_base & IA32_MSR_APIC_BASE_X2APIC) != 0))) {
		dprintf("found x2apic\n");

		if (get_safemode_boolean(B_SAFEMODE_DISABLE_X2APIC, false)) {
			dprintf("x2apic disabled per safemode setting\n");
		} else {
			sX2APIC = true;
			return B_OK;
		}
	}

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
	if (sX2APIC) {
		uint64 apic_base = x86_read_msr(IA32_MSR_APIC_BASE);
		if ((apic_base & IA32_MSR_APIC_BASE_X2APIC) == 0) {
			x86_write_msr(IA32_MSR_APIC_BASE, apic_base
				| IA32_MSR_APIC_BASE_X2APIC);
		}
	}

	dprintf("setting up %sapic for CPU %" B_PRId32 ": apic id %" B_PRIu32 ", "
		"version %" B_PRIu32 "\n", sX2APIC ? "x2" : "", cpu, apic_local_id(),
		apic_version());

	if (!sX2APIC && cpu < 8) {
		apic_write(APIC_DEST_FORMAT, uint32(-1));

		uint8 logical_apic_id = 1 << cpu;
		uint32 value = apic_read(APIC_LOGICAL_DEST);
		value &= 0xffffff;
		apic_write(APIC_LOGICAL_DEST, value | (logical_apic_id << 24));
	}

	// get logical APIC ID
	gCPU[cpu].arch.logical_apic_id = apic_logical_apic_id();
	if (!sX2APIC)
		gCPU[cpu].arch.logical_apic_id >>= 24;
	dprintf("CPU %" B_PRId32 ": logical apic id: %#" B_PRIx32 "\n", cpu,
		gCPU[cpu].arch.logical_apic_id);

	/* set spurious interrupt vector to 0xff */
	uint32 config = apic_spurious_intr_vector() & 0xffffff00;
	config |= APIC_ENABLE | 0xff;
	apic_set_spurious_intr_vector(config);

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
	config = (apic_lvt_error() & 0xffffff00) | 0xfe;
	apic_set_lvt_error(config);

	/* accept all interrupts */
	config = apic_task_priority() & 0xffffff00;
	apic_set_task_priority(config);

	config = apic_spurious_intr_vector();
	apic_end_of_interrupt();

	return B_OK;
}

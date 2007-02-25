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

#include <arch/x86/smp_priv.h>
#include <arch/x86/smp_apic.h>
#include <arch/x86/timer.h>

#include <string.h>
#include <stdio.h>


//#define TRACE_ARCH_SMP
#ifdef TRACE_ARCH_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

//#define TRACE_ARCH_SMP_TIMER
#ifdef TRACE_ARCH_SMP_TIMER
#	define TRACE_TIMER(x) dprintf x
#else
#	define TRACE_TIMER(x) ;
#endif


static void *apic = NULL;
static uint32 cpu_apic_id[B_MAX_CPU_COUNT] = {0, 0};
static uint32 cpu_os_id[B_MAX_CPU_COUNT] = {0, 0};
static uint32 cpu_apic_version[B_MAX_CPU_COUNT] = {0, 0};
static void *ioapic = NULL;
static uint32 apic_timer_tics_per_sec = 0;


static int32
i386_timer_interrupt(void *data)
{
	arch_smp_ack_interrupt();

	return apic_timer_interrupt();
}


static int32
i386_ici_interrupt(void *data)
{
	// genuine inter-cpu interrupt
	TRACE(("inter-cpu interrupt on cpu %ld\n", smp_get_current_cpu()));
	arch_smp_ack_interrupt();

	return smp_intercpu_int_handler();
}


static int32
i386_spurious_interrupt(void *data)
{
	// spurious interrupt
	TRACE(("spurious interrupt on cpu %ld\n", smp_get_current_cpu()));
	arch_smp_ack_interrupt();

	return B_HANDLED_INTERRUPT;
}


static int32
i386_smp_error_interrupt(void *data)
{
	// smp error interrupt
	TRACE(("smp error interrupt on cpu %ld\n", smp_get_current_cpu()));
	arch_smp_ack_interrupt();

	return B_HANDLED_INTERRUPT;
}


static uint32
apic_read(uint32 offset)
{
	return *(volatile uint32 *)((char *)apic + offset);
}


static void
apic_write(uint32 offset, uint32 data)
{
	*(volatile uint32 *)((char *)apic + offset) = data;
}


static status_t
setup_apic(kernel_args *args, int32 cpu)
{
	uint32 config;

	TRACE(("setting up the APIC for CPU %ld...\n", cpu));

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

	/* setup timer */
	config = apic_read(APIC_LVT_TIMER) & APIC_LVT_TIMER_MASK;
	config |= 0xfb | APIC_LVT_MASKED; // vector 0xfb, timer masked
	apic_write(APIC_LVT_TIMER, config);

	apic_write(APIC_INITIAL_TIMER_COUNT, 0); // zero out the clock

	config = apic_read(APIC_TIMER_DIVIDE_CONFIG) & 0xfffffff0;
	config |= APIC_TIMER_DIVIDE_CONFIG_1; // clock division by 1
	apic_write(APIC_TIMER_DIVIDE_CONFIG, config);

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


status_t
arch_smp_init(kernel_args *args)
{
	TRACE(("arch_smp_init: entry\n"));

	if (args->num_cpus > 1) {
		// setup some globals
		apic = (void *)args->arch_args.apic;
		ioapic = (void *)args->arch_args.ioapic;
		memcpy(cpu_apic_id, args->arch_args.cpu_apic_id, sizeof(args->arch_args.cpu_apic_id));
		memcpy(cpu_os_id, args->arch_args.cpu_os_id, sizeof(args->arch_args.cpu_os_id));
		memcpy(cpu_apic_version, args->arch_args.cpu_apic_version, sizeof(args->arch_args.cpu_apic_version));
		apic_timer_tics_per_sec = args->arch_args.apic_time_cv_factor;

		// setup regions that represent the apic & ioapic
		create_area("local apic", &apic, B_EXACT_ADDRESS, B_PAGE_SIZE,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		create_area("ioapic", &ioapic, B_EXACT_ADDRESS, B_PAGE_SIZE,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

		// set up the local apic on the boot cpu
		arch_smp_per_cpu_init(args, 0);

		// I/O interrupts start at ARCH_INTERRUPT_BASE, so all interrupts are shifted
		install_io_interrupt_handler(0xfb - ARCH_INTERRUPT_BASE, &i386_timer_interrupt, NULL, 0);
		install_io_interrupt_handler(0xfd - ARCH_INTERRUPT_BASE, &i386_ici_interrupt, NULL, 0);
		install_io_interrupt_handler(0xfe - ARCH_INTERRUPT_BASE, &i386_smp_error_interrupt, NULL, 0);
		install_io_interrupt_handler(0xff - ARCH_INTERRUPT_BASE, &i386_spurious_interrupt, NULL, 0);
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
	apic_write(APIC_INTR_COMMAND_2, config | cpu_apic_id[target_cpu] << 24);

	config = apic_read(APIC_INTR_COMMAND_1) & APIC_INTR_COMMAND_1_MASK;
	apic_write(APIC_INTR_COMMAND_1, config | 0xfd | APIC_DELIVERY_MODE_FIXED
		| APIC_INTR_COMMAND_1_ASSERT
		| APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL
		| APIC_INTR_COMMAND_1_DEST_FIELD);

	timeout = 100000000;
	// wait for message to be sent
	while ((apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) != 0 && --timeout != 0)
		;
	if (timeout == 0)
		panic("arch_smp_send_ici: timeout, target_cpu %ld", target_cpu);

	restore_interrupts(state);
}


void
arch_smp_ack_interrupt(void)
{
	apic_write(APIC_EOI, 0);
}

#define MIN_TIMEOUT 1000

status_t
arch_smp_set_apic_timer(bigtime_t relativeTimeout)
{
	cpu_status state;
	uint32 config;
	uint32 ticks;

	if (apic == NULL)
		return B_ERROR;

	if (relativeTimeout < MIN_TIMEOUT)
		relativeTimeout = MIN_TIMEOUT;

	// calculation should be ok, since it's going to be 64-bit
	ticks = ((relativeTimeout * apic_timer_tics_per_sec) / 1000000);

	state = disable_interrupts();

	config = apic_read(APIC_LVT_TIMER) | APIC_LVT_MASKED; // mask the timer
	apic_write(APIC_LVT_TIMER, config);

	apic_write(APIC_INITIAL_TIMER_COUNT, 0); // zero out the timer

	config = apic_read(APIC_LVT_TIMER) & ~APIC_LVT_MASKED; // unmask the timer
	apic_write(APIC_LVT_TIMER, config);

	TRACE_TIMER(("arch_smp_set_apic_timer: config 0x%lx, timeout %Ld, tics/sec %lu, tics %lu\n",
		config, relativeTimeout, apic_timer_tics_per_sec, ticks));

	apic_write(APIC_INITIAL_TIMER_COUNT, ticks); // start it up

	restore_interrupts(state);

	return B_OK;
}


status_t
arch_smp_clear_apic_timer(void)
{
	cpu_status state;
	uint32 config;

	if (apic == NULL)
		return B_ERROR;

	state = disable_interrupts();

	config = apic_read(APIC_LVT_TIMER) | APIC_LVT_MASKED; // mask the timer
	apic_write(APIC_LVT_TIMER, config);

	apic_write(APIC_INITIAL_TIMER_COUNT, 0); // zero out the timer

	restore_interrupts(state);
	return B_OK;
}


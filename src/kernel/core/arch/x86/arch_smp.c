/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>
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


static uint32 *apic = NULL;
static uint32 cpu_apic_id[B_MAX_CPU_COUNT] = {0, 0};
static uint32 cpu_os_id[B_MAX_CPU_COUNT] = {0, 0};
static uint32 cpu_apic_version[B_MAX_CPU_COUNT] = {0, 0};
static uint32 *ioapic = NULL;
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
	// gin-u-wine inter-cpu interrupt
//	dprintf("inter-cpu interrupt on cpu %d\n", arch_smp_get_current_cpu());
	arch_smp_ack_interrupt();

	return smp_intercpu_int_handler();
}


static int32
i386_spurious_interrupt(void *data)
{
	// spurious interrupt
//	dprintf("spurious interrupt on cpu %d\n", arch_smp_get_current_cpu());
	arch_smp_ack_interrupt();

	return B_HANDLED_INTERRUPT;
}


static int32
i386_smp_error_interrupt(void *data)
{
	// smp error interrupt
	TRACE(("smp error interrupt on cpu %d\n", arch_smp_get_current_cpu()));
	arch_smp_ack_interrupt();

	return B_HANDLED_INTERRUPT;
}


static uint32
apic_read(uint32 offset)
{
	return *(uint32 *)((uint32)apic + offset);
}


static void
apic_write(uint32 offset, uint32 data)
{
	uint32 *addr = (uint32 *)((uint32)apic + offset);
	*addr = data;
}


status_t
arch_smp_init(kernel_args *ka)
{
	TRACE(("arch_smp_init: entry\n"));

	if (ka->num_cpus > 1) {
		// setup some globals
		apic = (uint32 *)ka->arch_args.apic;
		ioapic = (uint32 *)ka->arch_args.ioapic;
		memcpy(cpu_apic_id, ka->arch_args.cpu_apic_id, sizeof(ka->arch_args.cpu_apic_id));
		memcpy(cpu_os_id, ka->arch_args.cpu_os_id, sizeof(ka->arch_args.cpu_os_id));
		memcpy(cpu_apic_version, ka->arch_args.cpu_apic_version, sizeof(ka->arch_args.cpu_apic_version));
		apic_timer_tics_per_sec = ka->arch_args.apic_time_cv_factor;

		// setup regions that represent the apic & ioapic
		vm_create_anonymous_region(vm_get_kernel_aspace_id(), "local_apic", (void *)&apic,
			REGION_ADDR_EXACT_ADDRESS, PAGE_SIZE, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);
		vm_create_anonymous_region(vm_get_kernel_aspace_id(), "ioapic", (void *)&ioapic,
			REGION_ADDR_EXACT_ADDRESS, PAGE_SIZE, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

		// set up the local apic on the boot cpu
		arch_smp_per_cpu_init(ka, 0);

		install_interrupt_handler(0xfb, &i386_timer_interrupt, NULL);
		install_interrupt_handler(0xfd, &i386_ici_interrupt, NULL);
		install_interrupt_handler(0xfe, &i386_smp_error_interrupt, NULL);
		install_interrupt_handler(0xff, &i386_spurious_interrupt, NULL);
	}
	return B_OK;
}


static int
smp_setup_apic(kernel_args *ka)
{
	uint32 config;

	TRACE(("setting up the apic..."));

	/* set spurious interrupt vector to 0xff */
	config = apic_read(APIC_SIVR) & 0xfffffc00;
	config |= APIC_ENABLE | 0xff;
	apic_write(APIC_SIVR, config);
#if 0
	/* setup LINT0 as ExtINT */
	config = (apic_read(APIC_LINT0) & 0xffff1c00);
	config |= APIC_LVT_DM_ExtINT | APIC_LVT_IIPP | APIC_LVT_TM;
	apic_write(APIC_LINT0, config);

	/* setup LINT1 as NMI */
	config = (apic_read(APIC_LINT1) & 0xffff1c00);
	config |= APIC_LVT_DM_NMI | APIC_LVT_IIPP;
	apic_write(APIC_LINT1, config);
#endif

	/* setup timer */
	config = apic_read(APIC_LVTT) & ~APIC_LVTT_MASK;
	config |= 0xfb | APIC_LVTT_M; // vector 0xfb, timer masked
	apic_write(APIC_LVTT, config);

	apic_write(APIC_ICRT, 0); // zero out the clock

	config = apic_read(APIC_TDCR) & ~0x0000000f;
	config |= APIC_TDCR_1; // clock division by 1
	apic_write(APIC_TDCR, config);

	/* setup error vector to 0xfe */
	config = (apic_read(APIC_LVT3) & 0xffffff00) | 0xfe;
	apic_write(APIC_LVT3, config);

	/* accept all interrupts */
	config = apic_read(APIC_TPRI) & 0xffffff00;
	apic_write(APIC_TPRI, config);

	config = apic_read(APIC_SIVR);
	apic_write(APIC_EOI, 0);

	TRACE((" done\n"));
	return 0;
}


status_t
arch_smp_per_cpu_init(kernel_args *args, int32 cpu)
{
	// set up the local apic on the current cpu
	TRACE(("arch_smp_init_percpu: setting up the apic on cpu %ld\n", cpu));
	smp_setup_apic(args);

	return B_OK;
}


void
arch_smp_send_broadcast_ici(void)
{
	uint32 config;
	cpu_status state = disable_interrupts();

	config = apic_read(APIC_ICR1) & APIC_ICR1_WRITE_MASK;
	apic_write(APIC_ICR1, config | 0xfd | APIC_ICR1_DELMODE_FIXED | APIC_ICR1_DESTMODE_PHYS | APIC_ICR1_DEST_ALL_BUT_SELF);

	restore_interrupts(state);
}


void
arch_smp_send_ici(int32 target_cpu)
{
	uint32 config;
	int state = disable_interrupts();

	config = apic_read(APIC_ICR2) & APIC_ICR2_MASK;
	apic_write(APIC_ICR2, config | cpu_apic_id[target_cpu] << 24);

	config = apic_read(APIC_ICR1) & APIC_ICR1_WRITE_MASK;
	apic_write(APIC_ICR1, config | 0xfd | APIC_ICR1_DELMODE_FIXED | APIC_ICR1_DESTMODE_PHYS | APIC_ICR1_DEST_FIELD);

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

	config = apic_read(APIC_LVTT) | APIC_LVTT_M; // mask the timer
	apic_write(APIC_LVTT, config);

	apic_write(APIC_ICRT, 0); // zero out the timer

	config = apic_read(APIC_LVTT) & ~APIC_LVTT_M; // unmask the timer
	apic_write(APIC_LVTT, config);

	TRACE(("arch_smp_set_apic_timer: config 0x%x, timeout %Ld, tics/sec %d, tics %d\n",
		config, relative_timeout, apic_timer_tics_per_sec, ticks));

	apic_write(APIC_ICRT, ticks); // start it up

	restore_interrupts(state);

	return B_OK;
}


int
arch_smp_clear_apic_timer(void)
{
	cpu_status state;
	uint32 config;

	if (apic == NULL)
		return -1;

	state = disable_interrupts();

	config = apic_read(APIC_LVTT) | APIC_LVTT_M; // mask the timer
	apic_write(APIC_LVTT, config);

	apic_write(APIC_ICRT, 0); // zero out the timer

	restore_interrupts(state);

	return 0;
}


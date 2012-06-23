/*
 * Copyright 2009-2010, Stefano Ceccherini (stefano.ceccherini@gmail.com)
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <debug.h>
#include <int.h>
#include <smp.h>
#include <timer.h>

#include <arch/int.h>
#include <arch/cpu.h>
#include <arch/x86/timer.h>
#include <arch/x86/arch_hpet.h>

#include <boot/kernel_args.h>
#include <vm/vm.h>


//#define TRACE_HPET
#ifdef TRACE_HPET
	#define TRACE(x) dprintf x
#else
	#define TRACE(x) ;
#endif

#define TEST_HPET

static struct hpet_regs *sHPETRegs;
static volatile struct hpet_timer *sTimer;
static uint64 sHPETPeriod;

static int hpet_get_priority();
static status_t hpet_set_hardware_timer(bigtime_t relativeTimeout);
static status_t hpet_clear_hardware_timer();
static status_t hpet_init(struct kernel_args *args);


struct timer_info gHPETTimer = {
	"HPET",
	&hpet_get_priority,
	&hpet_set_hardware_timer,
	&hpet_clear_hardware_timer,
	&hpet_init
};


static int
hpet_get_priority()
{
	// TODO: Fix HPET in SMP mode.
	if (smp_get_num_cpus() > 1)
		return 0;

	// HPET timers, being off-chip, are more expensive to setup
	// than the LAPIC.
	return 0;
}


static int32
hpet_timer_interrupt(void *arg)
{
	//dprintf_no_syslog("HPET timer_interrupt!!!!\n");
	return timer_interrupt();
}


static inline bigtime_t
hpet_convert_timeout(const bigtime_t &relativeTimeout)
{
	return ((relativeTimeout * 1000000000ULL)
		/ sHPETPeriod) + sHPETRegs->u0.counter64;
}


#define MIN_TIMEOUT 3000

static status_t
hpet_set_hardware_timer(bigtime_t relativeTimeout)
{
	cpu_status state = disable_interrupts();

	// enable timer interrupt
	sTimer->config |= HPET_CONF_TIMER_INT_ENABLE;

	// TODO:
	if (relativeTimeout < MIN_TIMEOUT)
		relativeTimeout = MIN_TIMEOUT;

	bigtime_t timerValue = hpet_convert_timeout(relativeTimeout);

	sTimer->u0.comparator64 = timerValue;

	restore_interrupts(state);

	return B_OK;
}


static status_t
hpet_clear_hardware_timer()
{
	// Disable timer interrupt
	sTimer->config &= ~HPET_CONF_TIMER_INT_ENABLE;
	return B_OK;
}


static status_t
hpet_set_enabled(bool enabled)
{
	if (enabled)
		sHPETRegs->config |= HPET_CONF_MASK_ENABLED;
	else
		sHPETRegs->config &= ~HPET_CONF_MASK_ENABLED;
	return B_OK;
}


static status_t
hpet_set_legacy(bool enabled)
{
	if (!HPET_IS_LEGACY_CAPABLE(sHPETRegs)) {
		dprintf("hpet_init: HPET doesn't support legacy mode. Skipping.\n");
		return B_NOT_SUPPORTED;
	}

	if (enabled)
		sHPETRegs->config |= HPET_CONF_MASK_LEGACY;
	else
		sHPETRegs->config &= ~HPET_CONF_MASK_LEGACY;

	return B_OK;
}


#ifdef TRACE_HPET
static void
hpet_dump_timer(volatile struct hpet_timer *timer)
{
	dprintf("HPET Timer %ld:\n", (timer - sHPETRegs->timer));

	dprintf("\troutable IRQs: ");
	uint32 interrupts = (uint32)HPET_GET_CAP_TIMER_ROUTE(timer);
	for (int i = 0; i < 32; i++) {
		if (interrupts & (1 << i))
			dprintf("%d ", i);
	}
	dprintf("\n");
	dprintf("\tconfiguration: 0x%llx\n", timer->config);
	dprintf("\tFSB Enabled: %s\n",
		timer->config & HPET_CONF_TIMER_FSB_ENABLE ? "Yes" : "No");
	dprintf("\tInterrupt Enabled: %s\n",
		timer->config & HPET_CONF_TIMER_INT_ENABLE ? "Yes" : "No");
	dprintf("\tTimer type: %s\n",
		timer->config & HPET_CONF_TIMER_TYPE ? "Periodic" : "OneShot");
	dprintf("\tInterrupt Type: %s\n",
		timer->config & HPET_CONF_TIMER_INT_TYPE ? "Level" : "Edge");

	dprintf("\tconfigured IRQ: %lld\n",
		HPET_GET_CONF_TIMER_INT_ROUTE(timer));

	if (timer->config & HPET_CONF_TIMER_FSB_ENABLE) {
		dprintf("\tfsb_route[0]: 0x%llx\n", timer->fsb_route[0]);
		dprintf("\tfsb_route[1]: 0x%llx\n", timer->fsb_route[1]);
	}
}
#endif


static void
hpet_init_timer(volatile struct hpet_timer *timer)
{
	sTimer = timer;

	uint32 interrupt = 0;

	sTimer->config |= (interrupt << HPET_CONF_TIMER_INT_ROUTE_SHIFT)
		& HPET_CONF_TIMER_INT_ROUTE_MASK;

	// Non-periodic mode, edge triggered
	sTimer->config &= ~(HPET_CONF_TIMER_TYPE | HPET_CONF_TIMER_INT_TYPE);

	sTimer->config &= ~HPET_CONF_TIMER_FSB_ENABLE;
	sTimer->config &= ~HPET_CONF_TIMER_32MODE;

	// Enable timer
	sTimer->config |= HPET_CONF_TIMER_INT_ENABLE;

#ifdef TRACE_HPET
	hpet_dump_timer(sTimer);
#endif
}


static status_t
hpet_test()
{
	uint64 initialValue = sHPETRegs->u0.counter64;
	spin(10);
	uint64 finalValue = sHPETRegs->u0.counter64;

	if (initialValue == finalValue) {
		dprintf("hpet_test: counter does not increment\n");
		return B_ERROR;
	}

	return B_OK;
}


static status_t
hpet_init(struct kernel_args *args)
{
	/* hpet_acpi_probe() through a similar "scan spots" table
	   to that of smp.cpp.
	   Seems to be the most elegant solution right now. */
	if (args->arch_args.hpet == NULL)
		return B_ERROR;

	if (sHPETRegs == NULL) {
		sHPETRegs = (struct hpet_regs *)args->arch_args.hpet.Pointer();
		if (vm_map_physical_memory(B_SYSTEM_TEAM, "hpet",
			(void **)&sHPETRegs, B_EXACT_ADDRESS, B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(phys_addr_t)args->arch_args.hpet_phys, true) < B_OK) {
			// Would it be better to panic here?
			dprintf("hpet_init: Failed to map memory for the HPET registers.");
			return B_ERROR;
		}
	}

	sHPETPeriod = HPET_GET_PERIOD(sHPETRegs);

	TRACE(("hpet_init: HPET is at %p.\n\tVendor ID: %llx, rev: %llx, period: %lld\n",
		sHPETRegs, HPET_GET_VENDOR_ID(sHPETRegs), HPET_GET_REVID(sHPETRegs),
		sHPETPeriod));

	status_t status = hpet_set_enabled(false);
	if (status != B_OK)
		return status;

	status = hpet_set_legacy(true);
	if (status != B_OK)
		return status;

	uint32 numTimers = HPET_GET_NUM_TIMERS(sHPETRegs) + 1;

	TRACE(("hpet_init: HPET supports %lu timers, and is %s bits wide.\n",
		numTimers, HPET_IS_64BIT(sHPETRegs) ? "64" : "32"));

	TRACE(("hpet_init: configuration: 0x%llx, timer_interrupts: 0x%llx\n",
		sHPETRegs->config, sHPETRegs->interrupt_status));

	if (numTimers < 3) {
		dprintf("hpet_init: HPET does not have at least 3 timers. Skipping.\n");
		return B_ERROR;
	}

#ifdef TRACE_HPET
	for (uint32 c = 0; c < numTimers; c++)
		hpet_dump_timer(&sHPETRegs->timer[c]);
#endif

	hpet_init_timer(&sHPETRegs->timer[0]);

	status = hpet_set_enabled(true);
	if (status != B_OK)
		return status;

#ifdef TEST_HPET
	status = hpet_test();
	if (status != B_OK)
		return status;
#endif

	int32 configuredIRQ = HPET_GET_CONF_TIMER_INT_ROUTE(sTimer);

	install_io_interrupt_handler(configuredIRQ, &hpet_timer_interrupt,
		NULL, B_NO_LOCK_VECTOR);

	return status;
}


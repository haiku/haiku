/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <timer.h>
#include <arch/x86/timer.h>
#include <arch/x86/arch_hpet.h>

#include <boot/kernel_args.h>

#include <arch/int.h>
#include <arch/cpu.h>
#include <int.h>

#define TRACE_HPET
#ifdef TRACE_HPET
	#define TRACE(x) dprintf x
#else
	#define TRACE(x) ;
#endif

static struct hpet_regs *sHPETRegs;
static volatile struct hpet_timer *sTimer;

static int hpet_get_prio();
static status_t hpet_set_hardware_timer(bigtime_t relativeTimeout);
static status_t hpet_clear_hardware_timer();
static status_t hpet_init(struct kernel_args *args);


struct timer_info gHPETTimer = {
	"HPET",
	&hpet_get_prio,
	&hpet_set_hardware_timer,
	&hpet_clear_hardware_timer,
	&hpet_init
};


static int
hpet_get_prio()
{
	return 0; //TODO: Should be 3, so it gets picked first
}


static int32
hpet_timer_interrupt(void *arg)
{
	return timer_interrupt();
}


static inline bigtime_t
hpet_convert_timeout(const bigtime_t &relativeTimeout)
{
	return ((relativeTimeout * 1000000000LL)
		/ HPET_GET_PERIOD(sHPETRegs)) + sHPETRegs->u0.counter64;
}


static status_t
hpet_set_hardware_timer(bigtime_t relativeTimeout)
{
	cpu_status state = disable_interrupts();

	bigtime_t timerValue = hpet_convert_timeout(relativeTimeout);
	
	//dprintf("setting value %lld, cur is %lld\n", timerValue, sHPETRegs->counter64);
	
	sTimer->u0.comparator64 = timerValue;

	restore_interrupts(state);

	return B_OK;
}


static status_t
hpet_clear_hardware_timer()
{
	// Disable timer
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
	dprintf(" routable interrupts:\n");
	uint32 interrupts = (uint32)HPET_GET_CAP_TIMER_ROUTE(timer);
	for (int i = 0; i < 32; i++) {
		if (interrupts & (1 << i))
			dprintf("%d ", i);
	}
		
	dprintf("\n");
	dprintf(" config: 0x%llx\n", timer->config);
	dprintf(" configured interrupt: %lld\n",
		HPET_GET_CONF_TIMER_INT_ROUTE(timer));
	
	dprintf(" fsb_route[0]: 0x%llx\n", timer->fsb_route[0]);
	dprintf(" fsb_route[1]: 0x%llx\n", timer->fsb_route[1]);
}
#endif


static void
hpet_init_timer(volatile struct hpet_timer *timer)
{
	sTimer = timer;
	
	// Configure timer for interrupt 0
	sTimer->config &= (~HPET_CONF_TIMER_INT_ROUTE_MASK
		<< HPET_CONF_TIMER_INT_ROUTE_SHIFT);
	
	// Non-periodic mode, edge triggered
	sTimer->config &= ~(HPET_CONF_TIMER_TYPE | HPET_CONF_TIMER_INT_TYPE);

	sTimer->config &= ~HPET_CONF_TIMER_FSB_ENABLE;

	// Enable timer
	sTimer->config |= HPET_CONF_TIMER_INT_ENABLE;
	
#ifdef TRACE_HPET
	dprintf("hpet_init_timer: timer %p, final configuration:\n", timer);
	hpet_dump_timer(sTimer);
#endif
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
		sHPETRegs = (struct hpet_regs *)args->arch_args.hpet;
		if (map_physical_memory("hpet", (void *)args->arch_args.hpet_phys,
			B_PAGE_SIZE, B_EXACT_ADDRESS, B_KERNEL_READ_AREA |
			B_KERNEL_WRITE_AREA, (void **)&sHPETRegs) < B_OK) {
			// Would it be better to panic here?
			dprintf("hpet_init: Failed to map memory for the HPET registers.");
			return B_ERROR;
		}
	}

	TRACE(("hpet_init: HPET is at %p. Vendor ID: %llx. Period: %lld\n",
		sHPETRegs, HPET_GET_VENDOR_ID(sHPETRegs), HPET_GET_PERIOD(sHPETRegs)));

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
	for (uint32 c = 0; c < numTimers; c++) {
		TRACE(("hpet_init: timer %lu:\n", c));	
		hpet_dump_timer(&sHPETRegs->timer[c]);
	}
#endif

	install_io_interrupt_handler(0xfb - ARCH_INTERRUPT_BASE,
		&hpet_timer_interrupt, NULL, B_NO_LOCK_VECTOR);
	install_io_interrupt_handler(0, &hpet_timer_interrupt, NULL,
		B_NO_LOCK_VECTOR);

	hpet_init_timer(&sHPETRegs->timer[0]);
	
	status = hpet_set_enabled(true);

	return status;
}


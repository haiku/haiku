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
convert_timeout(const bigtime_t &relativeTimeout)
{
	return ((relativeTimeout * 1000000000LL)
		/ sHPETRegs->period) + sHPETRegs->counter;
}


static status_t
hpet_set_hardware_timer(bigtime_t relativeTimeout)
{
	cpu_status state = disable_interrupts();

	//dprintf("setting value %d, cur is %d\n", timerValue, sHPETRegs->counter);
	sHPETRegs->timer[2].comparator = convert_timeout(relativeTimeout);

	// Clear the interrupt (set to 0)
	sHPETRegs->timer[2].config &= (~31 << 9);

	// Non-periodic mode, edge triggered
	sHPETRegs->timer[2].config &= ~(0x8 & 0x2);

	// Enable timer
	sHPETRegs->timer[2].config |= 0x4;

	restore_interrupts(state);

	return B_OK;
}


static status_t
hpet_clear_hardware_timer()
{
	// Disable timer
	sHPETRegs->timer[2].config &= ~0x4;
	return B_OK;
}


static status_t
hpet_set_enabled(struct hpet_regs *regs, bool enabled)
{
	if (enabled)
		regs->config |= HPET_CONF_MASK_ENABLED;
	else
		regs->config &= ~HPET_CONF_MASK_ENABLED;
	return B_OK;
}


static status_t
hpet_set_legacy(struct hpet_regs *regs, bool enabled)
{
	if (!HPET_IS_LEGACY_CAPABLE(regs))
		return B_NOT_SUPPORTED;

	if (enabled)
		regs->config |= HPET_CONF_MASK_LEGACY;
	else
		regs->config &= ~HPET_CONF_MASK_LEGACY;
	
	return B_OK;
}


#ifdef TRACE_HPET
static void
dump_timer(volatile struct hpet_timer *timer)
{
	dprintf(" config: 0x%lx\n", timer->config);
	dprintf(" interrupts: 0x%lx\n", timer->interrupts);
	dprintf(" fsb_value: 0x%lx\n", timer->fsb_value);
	dprintf(" fsb_addr: 0x%lx\n", timer->fsb_addr);	
}
#endif


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

	TRACE(("hpet_init: HPET is at %p. Vendor ID: %lx. Period: %ld\n",
		sHPETRegs, HPET_GET_VENDOR_ID(sHPETRegs), sHPETRegs->period));

	/* There is no hpet legacy support, so error out on init */
	if (!HPET_IS_LEGACY_CAPABLE(sHPETRegs)) {
		dprintf("hpet_init: HPET doesn't support legacy mode. Skipping.\n");
		return B_ERROR;
	}

	status_t status = hpet_set_legacy(sHPETRegs, true);
	if (status != B_OK)
		return status;

	uint32 numTimers = HPET_GET_NUM_TIMERS(sHPETRegs) + 1;
	
	TRACE(("hpet_init: HPET does%s support legacy mode.\n",
		HPET_IS_LEGACY_CAPABLE(sHPETRegs) ? "" : " not"));
	TRACE(("hpet_init: HPET supports %lu timers, and is %s bits wide.\n",
		numTimers, HPET_IS_64BIT(sHPETRegs) ? "64" : "32"));
	
	TRACE(("hpet_init: configuration: 0x%llx, timer_interrupts: 0x%llx\n",
		sHPETRegs->config, sHPETRegs->timer_interrupts));

	if (numTimers < 3) {
		dprintf("hpet_init: HPET does not have at least 3 timers. Skipping.\n");
		return B_ERROR;
	}

#ifdef TRACE_HPET
	for (uint32 c = 0; c < numTimers; c++) {
		TRACE(("hpet_init: timer %lu:\n", c));	
		dump_timer(&sHPETRegs->timer[c]);
	}
#endif	
	install_io_interrupt_handler(0xfb - ARCH_INTERRUPT_BASE,
		&hpet_timer_interrupt, NULL, B_NO_LOCK_VECTOR);
	install_io_interrupt_handler(0, &hpet_timer_interrupt, NULL,
		B_NO_LOCK_VECTOR);

	status = hpet_set_enabled(sHPETRegs, true);

	return status;
}


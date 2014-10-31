#include "soc_omap3.h"

enum {
	INTCPS_REVISION = 0,
	INTCPS_SYSCONFIG = 4,
	INTCPS_SYSSTATUS,
	INTCPS_SIR_IRQ = 16,
	INTCPS_SIR_FIQ = 17,
	INTCPS_CONTROL = 18,
	INTCPS_PROTECTION = 19,
	INTCPS_IDLE = 20,
	INTCPS_IRQ_PRIORITY = 24,
	INTCPS_FIQ_PRIORITY = 25,
	INTCPS_THRESHOLD = 26,
	INTCPS_ITRn = 32,
	INTCPS_MIRn = 33,
	INTCPS_MIR_CLEARn = 34,
	INTCPS_MIR_SETn = 35,
	INTCPS_ISR_SETn = 36,
	INTCPS_ISR_CLEARn = 37,
	INTCPS_PENDING_IRQn = 38,
	INTCPS_PENDING_FIQn = 39,
	INTCPS_ILRm = 40,
};


void
OMAP3InterruptController::EnableInterrupt(int irq)
{
	uint32 bit = irq % 32, bank = irq / 32;
	fRegBase[INTCPS_MIR_CLEARn + (8 * bank)] = 1 << bit;
}


void
OMAP3InterruptController::DisableInterrupt(int irq)
{
	uint32 bit = irq % 32, bank = irq / 32;
	fRegBase[INTCPS_MIR_SETn + (8 * bank)] = 1 << bit;
}


void
OMAP3InterruptController::HandleInterrupt()
{
	bool handledIRQ = false;
	int irqnr = 0;

	do {
		for (uint32 i=0; i < fNumPending; i++) {
			irqnr = fRegBase[INTCPS_PENDING_IRQn + (8 * i)];
			if (irqnr)
				break;
		}

		if (!irqnr)
			break;

		irqnr = fRegBase[INTCPS_SIR_IRQ];
		irqnr &= 0x7f; /* ACTIVEIRQ */

		if (irqnr) {
			int_io_interrupt_handler(irqnr, true);
			handledIRQ = true;
		}
	} while(irqnr);

	// If IRQ got cleared before we could handle it, simply
	// ack it.
	if (!handledIRQ)
		fRegBase[INTCPS_CONTROL] = 1;
}


void
OMAP3InterruptController::SoftReset()
{
	uint32 tmp = fRegBase[INTCPS_REVISION] & 0xff;

	dprintf("OMAP: INTC found at 0x%p (rev %ld.%ld)\n",
		fRegBase, tmp >> 4, tmp & 0xf);

	tmp = fRegBase[INTCPS_SYSCONFIG];
	tmp |= 1 << 1;  /* soft reset */
	fRegBase[INTCPS_SYSCONFIG] = tmp;

        while (!(fRegBase[INTCPS_SYSSTATUS] & 0x1))
                /* Wait for reset to complete */;

        /* Enable autoidle */
        fRegBase[INTCPS_SYSCONFIG] = 1;
}


OMAP3InterruptController::OMAP3InterruptController(fdt_module_info *fdt, fdt_device_node node)
	: InterruptController(fdt, node),
	fNumPending(3)
{
	fRegArea = fFDT->map_reg_range(node, 0, (void**)&fRegBase);
	if (fRegArea < 0)
		panic("OMAP3InterruptController: cannot map registers!");

	SoftReset();

	// Enable protection (MPU registers only available in privileged mode)
	fRegBase[INTCPS_PROTECTION] |= 1;
}


enum {
	TIDR = 0,
	TIOCP_CFG = 4,
	TISTAT,
	TISR,
	TIER,
	TWER,
	TCLR,
	TCRR,
	TLDR,
	TTGR,
	TWPS,
	TMAR,
	TCAR1,
	TSICR,
	TCAR2,
	TPIR,
	TNIR,
	TCVR,
	TOCR,
	TOWR,
};

int32
OMAP3Timer::_InterruptWrapper(void *data)
{
	return ((OMAP3Timer*)data)->HandleInterrupt();
}


int32
OMAP3Timer::HandleInterrupt()
{
	uint32 ints = fRegBase[TISR] & 7;

	if (ints & 1) { // Match?
		dprintf("OMAP3Timer: match!\n");
		timer_interrupt();
	} else if (ints & 2) { // Overflow?
		dprintf("OMAP3Timer: overflow!\n");
		fSystemTime += UINT_MAX +1;
	} else if (ints & 4) { // Capture?
		dprintf("OMAP3Timer: capture!\n");
	}

	// clear interrupt
	fRegBase[TISR] = ints;

	return B_HANDLED_INTERRUPT;
}


void
OMAP3Timer::SetTimeout(bigtime_t timeout)
{
	fRegBase[TMAR] = fRegBase[TCRR] + timeout / 1000ULL;
	fRegBase[TIER] |= 1; // Enable match interrupt
}


bigtime_t
OMAP3Timer::Time()
{
	return fSystemTime + fRegBase[TCRR];
}


void
OMAP3Timer::Clear()
{
	fRegBase[TIER] &= ~1; // Disable match interrupt
}


OMAP3Timer::OMAP3Timer(fdt_module_info *fdtModule, fdt_device_node node)
	: HardwareTimer(fdtModule, node),
	fSystemTime(0)
{
	fRegArea = fFDT->map_reg_range(node, 0, (void**)&fRegBase);
	if (fRegArea < 0)
		panic("Cannot map OMAP3Timer registers!");

	fInterrupt = fFDT->get_interrupt(node, 0);
	if (fInterrupt < 0)
		panic("Cannot get OMAP3Timer interrupt!");

	uint32 rev = fRegBase[TIDR];
	dprintf("OMAP: Found timer @ 0x%p, IRQ %d (rev %ld.%ld)\n", fRegBase, fInterrupt, (rev >> 4) & 0xf, rev & 0xf);

	// Let the timer run (so we can use it as clocksource)
	fRegBase[TCLR] |= 1;
	fRegBase[TIER] = 2; // Enable overflow interrupt

	install_io_interrupt_handler(fInterrupt, &OMAP3Timer::_InterruptWrapper, this, 0);
}

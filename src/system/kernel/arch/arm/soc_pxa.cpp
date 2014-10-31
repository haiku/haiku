#include "soc_pxa.h"

/* PXA Interrupt Controller Registers */
#define PXA_ICIP	0x00
#define PXA_ICMR	0x01
#define PXA_ICFP	0x03
#define PXA_ICMR2	0x28

void
PXAInterruptController::EnableInterrupt(int irq)
{
	if (irq <= 31) {
		fRegBase[PXA_ICMR] |= 1 << irq;
		return;
	}

	fRegBase[PXA_ICMR2] |= 1 << (irq - 32);
}


void
PXAInterruptController::DisableInterrupt(int irq)
{
	if (irq <= 31) {
		fRegBase[PXA_ICMR] &= ~(1 << irq);
		return;
	}

	fRegBase[PXA_ICMR2] &= ~(1 << (irq - 32));
}


void
PXAInterruptController::HandleInterrupt()
{
	for (int i=0; i < 32; i++) {
		if (fRegBase[PXA_ICIP] & (1 << i))
			int_io_interrupt_handler(i, true);
	}
}


PXAInterruptController::PXAInterruptController(fdt_module_info *fdt, fdt_device_node node)
	: InterruptController(fdt, node) {
	fRegArea = fFDT->map_reg_range(node, 0, (void**)&fRegBase);
	if (fRegArea < 0)
		panic("PXAInterruptController: cannot map registers!");

	fRegBase[PXA_ICMR] = 0;
	fRegBase[PXA_ICMR2] = 0;
}

#define PXA_TIMERS_INTERRUPT	7 /* OST_4_11 */

#define PXA_OSSR	0x05
#define PXA_OIER	0x07
#define PXA_OSCR4	0x10
#define PXA_OSCR5	0x11
#define PXA_OSMR4	0x20
#define PXA_OSMR5	0x21
#define PXA_OMCR4	0x30
#define PXA_OMCR5	0x31

#define PXA_RES_S	(3 << 0)
#define PXA_RES_MS	(1 << 1)
#define PXA_RES_US	(1 << 2)

#define US2S(bt)	((bt) / 1000000ULL)
#define US2MS(bt)	((bt) / 1000ULL)

void
PXATimer::SetTimeout(bigtime_t timeout)
{
	uint32 val = timeout & UINT_MAX;
	uint32 res = PXA_RES_US;

	if (timeout & ~UINT_MAX) {
		// Does not fit, so scale resolution down to milliseconds
		if (US2MS(timeout) & ~UINT_MAX) {
			// Still does not fit, scale down to seconds as last ditch attempt
			val = US2S(timeout) & UINT_MAX;
			res = PXA_RES_S;
		} else {
			// Fits in millisecond resolution
			val = US2MS(timeout) & UINT_MAX;
			res = PXA_RES_MS;
		}
	}

	dprintf("arch_timer_set_hardware_timer(val=%lu, res=%lu)\n", val, res);
	fRegBase[PXA_OIER] |= (1 << 4);
	fRegBase[PXA_OMCR4] = res;
	fRegBase[PXA_OSMR4] = val;
	fRegBase[PXA_OSCR4] = 0; // start counting from 0 again
}

void
PXATimer::Clear()
{
	fRegBase[PXA_OMCR4] = 0; // disable our timer
	fRegBase[PXA_OIER] &= ~(1 << 4);
}


bigtime_t
PXATimer::Time()
{
	if (fRegArea < 0)
		return 0;

	return (fRegBase != NULL) ?
		fSystemTime + fRegBase[PXA_OSCR5] :
		0ULL;
}


int32
PXATimer::_InterruptWrapper(void *data)
{
	return ((PXATimer*)data)->HandleInterrupt();
}


int32
PXATimer::HandleInterrupt()
{
	if (fRegBase[PXA_OSSR] & (1 << 4)) {
		fRegBase[PXA_OSSR] |= (1 << 4);
		return timer_interrupt();
	}

	if (fRegBase[PXA_OSSR]  & (1 << 5)) {
		fRegBase[PXA_OSSR] |= (1 << 5);
		fSystemTime += UINT_MAX + 1ULL;
	}

	return B_HANDLED_INTERRUPT;
}


PXATimer::PXATimer(fdt_module_info *fdt, fdt_device_node node)
	: HardwareTimer(fdt, node)
{
	fRegArea = fFDT->map_reg_range(node, 0, (void**)&fRegBase);
	if (fRegArea < 0)
		panic("Cannot map PXATimer registers!");

	fRegBase[PXA_OIER] |= (1 << 5); // enable timekeeping timer
	fRegBase[PXA_OMCR5] = PXA_RES_US | (1 << 7);
	fRegBase[PXA_OSMR5] = UINT_MAX;
	fRegBase[PXA_OSCR5] = 0;

	install_io_interrupt_handler(PXA_TIMERS_INTERRUPT, &PXATimer::_InterruptWrapper, NULL, 0);
}

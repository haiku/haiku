
/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <timer.h>
#include <arch/x86/timer.h>

#include "hpet.h"

static int32 sHPETAddr;
static struct hpet_regs *sHPETRegs;

struct timer_info gHPETTimer = {
	"HPET",
	&hpet_get_prio,
	&hpet_set_hardware_timer,
	&hpet_clear_hardware_timer,
	&hpet_init
};


static int
hpet_get_prio(void)
{
	return 3;
}


/* TODO: Implement something similar to smp_acpi_probe from boot/.../smp.cpp-
	we need to get the hpet base address without talking to the acpi module,
	because there's no guarantee that it's actually present at this early point. */

static int
hpet_set_enabled(struct hpet_regs *regs, bool enabled)
{
	if (enabled)
		regs->config |= HPET_CONF_MASK_ENABLED;
	else
		regs->config &= ~HPET_CONF_MASK_ENABLED;
	return B_OK;
}


static int
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


static int32
hpet_timer_interrupt(void *data)
{
	return timer_interrupt();
}


static status_t
hpet_set_hardware_timer(bigtime_t relative_timeout)
{
	return B_ERROR;
}


static status_t
hpet_clear_hardware_timer(void)
{
	return B_ERROR;
}


static status_t
hpet_init(struct kernel_args *args)
{
	/* hpet_acpi_probe() through a similar "scan spots" table to that of smp.cpp.
	   Seems to be the most elegant solution right now. */

	/* There is no hpet support proper, so error out on init */
	return B_ERROR;
}

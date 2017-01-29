/*
 * Copyright 2007-2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/stage2.h>
#include <kernel.h>
#include <debug.h>

#include <timer.h>
#include <arch/timer.h>
#include <arch/cpu.h>

#include <drivers/bus/FDT.h>
#include "soc.h"

#include "soc_pxa.h"
#include "soc_omap3.h"

//#define TRACE_ARCH_TIMER
#ifdef TRACE_ARCH_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static fdt_module_info *sFdtModule;

static struct fdt_device_info intc_table[] = {
	{
		.compatible = "marvell,pxa-timers",	// XXX not in FDT (also not in upstream!)
		.init = PXATimer::Init,
	}, {
		.compatible = "ti,omap3430-timer",
		.init = OMAP3Timer::Init,
	}
};
static int intc_count = sizeof(intc_table) / sizeof(struct fdt_device_info);


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	HardwareTimer *timer = HardwareTimer::Get();
	if (timer != NULL)
		timer->SetTimeout(timeout);
}


void
arch_timer_clear_hardware_timer()
{
	HardwareTimer *timer = HardwareTimer::Get();
	if (timer != NULL)
		timer->Clear();
}

int
arch_init_timer(kernel_args *args)
{
	TRACE(("%s\n", __func__));

	status_t rc = get_module(B_FDT_MODULE_NAME, (module_info**)&sFdtModule);
	if (rc != B_OK)
		panic("Unable to get FDT module: %08lx!\n", rc);

	rc = sFdtModule->setup_devices(intc_table, intc_count, NULL);
	if (rc != B_OK)
		panic("No interrupt controllers found!\n");

	return B_OK;
}

bigtime_t
system_time(void)
{
	HardwareTimer *timer = HardwareTimer::Get();
	if (timer != NULL)
		return timer->Time();

	return 0;
}

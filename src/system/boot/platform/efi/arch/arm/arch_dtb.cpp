/*
 * Copyright 2019-2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
 */

#include <arch_cpu_defs.h>
#include <arch_dtb.h>
#include <arch_smp.h>
#include <boot/platform.h>
#include <boot/stage2.h>

extern "C" {
#include <libfdt.h>
}

#include "dtb.h"


const struct supported_interrupt_controllers {
	const char*	dtb_compat;
	const char*	kind;
} kSupportedInterruptControllers[] = {
	{ "arm,cortex-a9-gic", INTC_KIND_GICV1 },
	{ "arm,cortex-a15-gic", INTC_KIND_GICV2 },
	{ "arm,gic-400", INTC_KIND_GICV2 },
	{ "ti,omap3-intc", INTC_KIND_OMAP3 },
	{ "marvell,pxa-intc", INTC_KIND_PXA },
	{ "allwinner,sun4i-a10-ic", INTC_KIND_SUN4I },
};


const struct supported_timers {
	const char* dtb_compat;
	const char* kind;
} kSupportedTimers[] = {
	{ "arm,armv7-timer",	TIMER_KIND_ARMV7 },
	{ "ti,omap3430-timer",	TIMER_KIND_OMAP3 },
	{ "marvell,pxa-timers",	TIMER_KIND_PXA },
};


void
arch_handle_fdt(const void* fdt, int node)
{
	const char* deviceType = (const char*)fdt_getprop(fdt, node,
		"device_type", NULL);

	if (deviceType != NULL) {
		if (strcmp(deviceType, "cpu") == 0) {
			platform_cpu_info* info;
			arch_smp_register_cpu(&info);
			if (info == NULL)
				return;
			info->id = fdt32_to_cpu(*(uint32*)fdt_getprop(fdt, node,
				"reg", NULL));
			dprintf("cpu\n");
			dprintf("  id: %" B_PRIu32 "\n", info->id);

		}
	}

	int compatibleLen;
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", &compatibleLen);

	if (compatible == NULL)
		return;

	intc_info &interrupt_controller = gKernelArgs.arch_args.interrupt_controller;
	if (interrupt_controller.kind[0] == 0) {
		for (uint32 i = 0; i < B_COUNT_OF(kSupportedInterruptControllers); i++) {
			if (dtb_has_fdt_string(compatible, compatibleLen,
				kSupportedInterruptControllers[i].dtb_compat)) {

				memcpy(interrupt_controller.kind, kSupportedInterruptControllers[i].kind,
					sizeof(interrupt_controller.kind));

				dtb_get_reg(fdt, node, 0, interrupt_controller.regs1);
				dtb_get_reg(fdt, node, 1, interrupt_controller.regs2);
			}
		}
	}

	boot_timer_info &timer = gKernelArgs.arch_args.timer;
	if (timer.kind[0] == 0) {
		for (uint32 i = 0; i < B_COUNT_OF(kSupportedTimers); i++) {
			if (dtb_has_fdt_string(compatible, compatibleLen,
				kSupportedTimers[i].dtb_compat)) {

				memcpy(timer.kind, kSupportedTimers[i].kind,
					sizeof(timer.kind));

				dtb_get_reg(fdt, node, 0, timer.regs);
				timer.interrupt = dtb_get_interrupt(fdt, node);
			}
		}
	}
}


void
arch_dtb_set_kernel_args(void)
{
	intc_info &interrupt_controller = gKernelArgs.arch_args.interrupt_controller;
	dprintf("Chosen interrupt controller:\n");
	if (interrupt_controller.kind[0] == 0) {
		dprintf("kind: None!\n");
	} else {
		dprintf("  kind: %s\n", interrupt_controller.kind);
		dprintf("  regs: %#" B_PRIx64 ", %#" B_PRIx64 "\n",
			interrupt_controller.regs1.start,
			interrupt_controller.regs1.size);
		dprintf("        %#" B_PRIx64 ", %#" B_PRIx64 "\n",
			interrupt_controller.regs2.start,
			interrupt_controller.regs2.size);
	}

	boot_timer_info &timer = gKernelArgs.arch_args.timer;
	dprintf("Chosen timer:\n");
	if (timer.kind[0] == 0) {
		dprintf("kind: None!\n");
	} else {
		dprintf("  kind: %s\n", timer.kind);
		dprintf("  regs: %#" B_PRIx64 ", %#" B_PRIx64 "\n",
			timer.regs.start,
			timer.regs.size);
		dprintf("  irq: %" B_PRIu32 "\n", timer.interrupt);
	}
}

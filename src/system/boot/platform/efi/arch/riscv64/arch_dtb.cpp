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


uint32 gBootHart = 0;
static uint64 sTimerFrequency = 10000000;

static addr_range sPlic = {0};
static addr_range sClint = {0};


void
arch_handle_fdt(const void* fdt, int node)
{
	const char* deviceType = (const char*)fdt_getprop(fdt, node,
		"device_type", NULL);

	const char* name = fdt_get_name(fdt, node, NULL);
	if (strcmp(name, "chosen") == 0) {
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "boot-hartid", NULL))
			gBootHart = fdt32_to_cpu(*prop);
	} else if (strcmp(name, "cpus") == 0) {
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "timebase-frequency", NULL))
			sTimerFrequency = fdt32_to_cpu(*prop);
	}

	if (deviceType != NULL) {
		if (strcmp(deviceType, "cpu") == 0) {
			// TODO: improve incompatible CPU detection
			if (!(fdt_getprop(fdt, node, "mmu-type", NULL) != NULL))
				return;

			platform_cpu_info* info;
			arch_smp_register_cpu(&info);
			if (info == NULL)
				return;
			info->id = fdt32_to_cpu(*(uint32*)fdt_getprop(fdt, node,
				"reg", NULL));
			dprintf("cpu\n");
			dprintf("  id: %" B_PRIu32 "\n", info->id);

			int subNode = fdt_subnode_offset(fdt, node, "interrupt-controller");
			if (subNode < 0) {
				dprintf("  [!] no interrupt controller\n");
			} else {
				info->phandle = fdt_get_phandle(fdt, subNode);
				dprintf("  phandle: %" B_PRIu32 "\n", info->phandle);
			}
		}
	}
	int compatibleLen;
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", &compatibleLen);

	if (compatible == NULL)
		return;

	if (dtb_has_fdt_string(compatible, compatibleLen, "riscv,clint0")) {
		dtb_get_reg(fdt, node, 0, sClint);
		return;
	}

	if (dtb_has_fdt_string(compatible, compatibleLen, "riscv,plic0")
		|| dtb_has_fdt_string(compatible, compatibleLen, "sifive,plic-1.0.0")) {
		dtb_get_reg(fdt, node, 0, sPlic);
		int propSize;
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts-extended", &propSize)) {
			dprintf("PLIC contexts\n");
			uint32 contextId = 0;
			for (uint32 *it = prop; (uint8_t*)it - (uint8_t*)prop < propSize; it += 2) {
				uint32 phandle = fdt32_to_cpu(*it);
				uint32 interrupt = fdt32_to_cpu(*(it + 1));
				if (interrupt == sExternInt) {
					platform_cpu_info* cpuInfo = arch_smp_find_cpu(phandle);
					dprintf("  context %" B_PRIu32 ": %" B_PRIu32 "\n", contextId, phandle);
					if (cpuInfo != NULL) {
						cpuInfo->plicContext = contextId;
						dprintf("    cpu id: %" B_PRIu32 "\n", cpuInfo->id);
					}
				}
				contextId++;
			}
		}
		return;
	}
}


void
arch_dtb_set_kernel_args(void)
{
	dprintf("bootHart: %" B_PRIu32 "\n", gBootHart);
	dprintf("timerFrequency: %" B_PRIu64 "\n", sTimerFrequency);
	gKernelArgs.arch_args.timerFrequency = sTimerFrequency;

//	gKernelArgs.arch_args.htif  = {.start = 0x40008000, .size = 0x10};
	gKernelArgs.arch_args.htif  = {.start = 0, .size = 0};
	gKernelArgs.arch_args.plic  = sPlic;
	gKernelArgs.arch_args.clint = sClint;
}

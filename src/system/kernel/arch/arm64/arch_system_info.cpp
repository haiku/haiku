/*
 * Copyright 2019-2026 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>

#include <arch/arm64/arch_cpu_type.h>
#include <arch/system_info.h>
#include <arch_cpu.h>
#include <boot/kernel_args.h>

#include "cpu.h"


static cpu_vendor sCPUVendor = B_CPU_VENDOR_UNKNOWN;


void
arch_fill_topology_node(cpu_topology_node_info* node, int32 cpu)
{
	switch (node->type) {
		case B_TOPOLOGY_ROOT:
			node->data.root.platform = B_CPU_ARM_64;
			break;
		case B_TOPOLOGY_PACKAGE:
			node->data.package.vendor = sCPUVendor;
			node->data.package.cache_line_size = CACHE_LINE_SIZE;
			break;
		case B_TOPOLOGY_CORE:
			node->data.core.model = static_cast<uint32>(gCPU[cpu].arch.midr);
			// TODO: This can be taken from SMBIOS Type 4, or the device tree depending on the
			// device
			node->data.core.default_frequency = 0;
			break;
		default:
			break;
	}
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	cpu_ent* cpu = get_cpu_struct();

	// TODO: It *is* possible for the package vendors to be heterogeneous,
	//       (Tegra X2 has NVIDIA and ARM-designed cores on the same package)
	//       but as vendor field is set for the whole package currently,
	//       it would have to be reworked for it to be supported.
	switch (CPU_IMPL(cpu->arch.midr)) {
		case CPU_IMPL_ARM:
			sCPUVendor = B_CPU_VENDOR_ARM;
			break;
		case CPU_IMPL_BROADCOM:
			sCPUVendor = B_CPU_VENDOR_BROADCOM;
			break;
		case CPU_IMPL_CAVIUM:
			sCPUVendor = B_CPU_VENDOR_CAVIUM;
			break;
		case CPU_IMPL_DEC:
			sCPUVendor = B_CPU_VENDOR_DEC;
			break;
		case CPU_IMPL_FUJITSU:
			sCPUVendor = B_CPU_VENDOR_FUJITSU;
			break;
		case CPU_IMPL_HISILICON:
			sCPUVendor = B_CPU_VENDOR_HISILICON;
			break;
		case CPU_IMPL_INFINEON:
			sCPUVendor = B_CPU_VENDOR_INFINEON;
			break;
		case CPU_IMPL_FREESCALE:
			sCPUVendor = B_CPU_VENDOR_FREESCALE;
			break;
		case CPU_IMPL_NVIDIA:
			sCPUVendor = B_CPU_VENDOR_NVIDIA;
			break;
		case CPU_IMPL_APM:
			sCPUVendor = B_CPU_VENDOR_APM;
			break;
		case CPU_IMPL_QUALCOMM:
			sCPUVendor = B_CPU_VENDOR_QUALCOMM;
			break;
		case CPU_IMPL_MARVELL:
			sCPUVendor = B_CPU_VENDOR_MARVELL;
			break;
		case CPU_IMPL_APPLE:
			sCPUVendor = B_CPU_VENDOR_APPLE;
			break;
		case CPU_IMPL_INTEL:
			sCPUVendor = B_CPU_VENDOR_INTEL;
			break;
		case CPU_IMPL_AMPERE:
			sCPUVendor = B_CPU_VENDOR_AMPERE;
			break;
		case CPU_IMPL_MICROSOFT:
			sCPUVendor = B_CPU_VENDOR_MICROSOFT;
			break;
		default:
			break;
	}

	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	// HACKME: This is a tricky one. There are a few options with wildly varying availability:
	// * ARMv8.4 Activity Monitors (AMEVCNTR0 counter)
	// * PMU cycle counter sampling
	// * on Apple M-series, p-state registers
	// * ACPI CPPC feedback counters
	// In QEMU, the only one that "works" is PMU, but it returns constant 1 GHz.

	*frequency = 0;
	return B_OK;
}

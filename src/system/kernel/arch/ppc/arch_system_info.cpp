/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>


enum cpu_vendor sCPUVendor;
uint32 sPVR;

static uint64 sCPUClockFrequency;
static uint64 sBusClockFrequency;

struct cpu_model {
	uint16			version;
	enum cpu_vendor	vendor;
};

// mapping of CPU versions to vendors
struct cpu_model kCPUModels[] = {
	{ MPC601,		B_CPU_VENDOR_MOTOROLA },
	{ MPC603,		B_CPU_VENDOR_MOTOROLA },
	{ MPC604,		B_CPU_VENDOR_MOTOROLA },
	{ MPC602,		B_CPU_VENDOR_MOTOROLA },
	{ MPC603e,		B_CPU_VENDOR_MOTOROLA },
	{ MPC603ev,		B_CPU_VENDOR_MOTOROLA },
	{ MPC750,		B_CPU_VENDOR_MOTOROLA },
	{ MPC604ev,		B_CPU_VENDOR_MOTOROLA },
	{ MPC7400,		B_CPU_VENDOR_MOTOROLA },
	{ MPC620,		B_CPU_VENDOR_MOTOROLA },
	{ IBM403,		B_CPU_VENDOR_IBM },
	{ IBM401A1,		B_CPU_VENDOR_IBM },
	{ IBM401B2,		B_CPU_VENDOR_IBM },
	{ IBM401C2,		B_CPU_VENDOR_IBM },
	{ IBM401D2,		B_CPU_VENDOR_IBM },
	{ IBM401E2,		B_CPU_VENDOR_IBM },
	{ IBM401F2,		B_CPU_VENDOR_IBM },
	{ IBM401G2,		B_CPU_VENDOR_IBM },
	{ IBMPOWER3,	B_CPU_VENDOR_IBM },
	{ MPC860,		B_CPU_VENDOR_MOTOROLA },
	{ MPC8240,		B_CPU_VENDOR_MOTOROLA },
	{ IBM405GP,		B_CPU_VENDOR_IBM },
	{ IBM405L,		B_CPU_VENDOR_IBM },
	{ IBM750FX,		B_CPU_VENDOR_IBM },
	{ MPC7450,		B_CPU_VENDOR_MOTOROLA },
	{ MPC7455,		B_CPU_VENDOR_MOTOROLA },
	{ MPC7457,		B_CPU_VENDOR_MOTOROLA },
	{ MPC7447A,		B_CPU_VENDOR_MOTOROLA },
	{ MPC7448,		B_CPU_VENDOR_MOTOROLA },
	{ MPC7410,		B_CPU_VENDOR_MOTOROLA },
	{ MPC8245,		B_CPU_VENDOR_MOTOROLA },
	{ 0,			B_CPU_VENDOR_UNKNOWN }
};


void
arch_fill_topology_node(cpu_topology_node_info* node, int32 cpu)
{
	switch (node->type) {
		case B_TOPOLOGY_ROOT:
#if  __powerpc64__
			node->data.root.platform = B_CPU_PPC_64;
#else
			node->data.root.platform = B_CPU_PPC;
#endif
			break;

		case B_TOPOLOGY_PACKAGE:
			node->data.package.vendor = sCPUVendor;
			node->data.package.cache_line_size = CACHE_LINE_SIZE;
			break;

		case B_TOPOLOGY_CORE:
			node->data.core.model = sPVR;
			node->data.core.default_frequency = sCPUClockFrequency;
			break;

		default:
			break;
	}
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	int i;

	sCPUClockFrequency = args->arch_args.cpu_frequency;
	sBusClockFrequency = args->arch_args.bus_frequency;

	// The PVR (processor version register) contains processor version and
	// revision.
	sPVR = get_pvr();
	uint16 model = (uint16)(sPVR >> 16);
	//sCPURevision = (uint16)(pvr & 0xffff);

	// Populate vendor
	for (i = 0; kCPUModels[i].vendor != B_CPU_VENDOR_UNKNOWN; i++) {
		if (model == kCPUModels[i].version) {
			sCPUVendor = kCPUModels[i].vendor;
			break;
		}
	}

	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	*frequency = sCPUClockFrequency;
	return B_OK;
}

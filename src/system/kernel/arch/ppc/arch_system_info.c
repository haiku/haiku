/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>


static uint64 sCPUClockFrequency;
static uint64 sBusClockFrequency;
static enum cpu_types sCPUType;
static uint16 sCPURevision;

struct cpu_model {
	uint16			version;
	enum cpu_types	beos_type;
};

// mapping of CPU versions to BeOS enum cpu_types
struct cpu_model kCPUModels[] = {
	{ MPC601,		B_CPU_PPC_601 },
	{ MPC603,		B_CPU_PPC_603 },
	{ MPC604,		B_CPU_PPC_604 },
	{ MPC602,		B_CPU_PPC_603ev },
	{ MPC603e,		B_CPU_PPC_603e },
	{ MPC603ev,		B_CPU_PPC_603ev },
	{ MPC750,		B_CPU_PPC_750 },
	{ MPC604ev,		B_CPU_PPC_604ev },
	{ MPC7400,		B_CPU_PPC_7400 },
	{ MPC620,		B_CPU_PPC_620 },
	{ IBM403,		B_CPU_PPC_IBM_403 },
	{ IBM401A1,		B_CPU_PPC_IBM_401A1 },
	{ IBM401B2,		B_CPU_PPC_IBM_401B2 },
	{ IBM401C2,		B_CPU_PPC_IBM_401C2 },
	{ IBM401D2,		B_CPU_PPC_IBM_401D2 },
	{ IBM401E2,		B_CPU_PPC_IBM_401E2 },
	{ IBM401F2,		B_CPU_PPC_IBM_401F2 },
	{ IBM401G2,		B_CPU_PPC_IBM_401G2 },
	{ IBMPOWER3,	B_CPU_PPC_IBM_POWER3 },
	{ MPC860,		B_CPU_PPC_860 },
	{ MPC8240,		B_CPU_PPC_8240 },
	{ IBM405GP,		B_CPU_PPC_IBM_405GP },
	{ IBM405L,		B_CPU_PPC_IBM_405L },
	{ IBM750FX,		B_CPU_PPC_IBM_750FX },
	{ MPC7450,		B_CPU_PPC_7450 },
	{ MPC7455,		B_CPU_PPC_7455 },
	{ MPC7457,		B_CPU_PPC_7457 },
	{ MPC7447A,		B_CPU_PPC_7447A },
	{ MPC7448,		B_CPU_PPC_7448 },
	{ MPC7410,		B_CPU_PPC_7410 },
	{ MPC8245,		B_CPU_PPC_8245 },
	{ 0,			B_CPU_PPC_UNKNOWN }
};


status_t
arch_get_system_info(system_info *info, size_t size)
{
	info->cpu_type = sCPUType;
	info->cpu_revision = sCPURevision;

	info->cpu_clock_speed = sCPUClockFrequency;
	info->bus_clock_speed = sBusClockFrequency;

	info->platform_type = B_MAC_PLATFORM;

	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	int i;

	sCPUClockFrequency = args->arch_args.cpu_frequency;
	sBusClockFrequency = args->arch_args.bus_frequency;

	// The PVR (processor version register) contains processor version and
	// revision.
	uint32 pvr = get_pvr();
	uint16 version = (uint16)(pvr >> 16);
	sCPURevision = (uint16)(pvr & 0xffff);

	// translate the version to a BeOS cpu_types constant
	sCPUType = B_CPU_PPC_UNKNOWN;
	for (i = 0; kCPUModels[i].beos_type != B_CPU_PPC_UNKNOWN; i++) {
		if (version == kCPUModels[i].version) {
			sCPUType = kCPUModels[i].beos_type;
			break;
		}
	}
	
	return B_OK;
}

/*
 * Copyright 2004-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2026, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_TYPE_H
#define CPU_TYPE_H

#include <stdlib.h>
#include <string.h>

#include <OS.h>

// get_cpu_model_string, implemented per architecture
#if defined(__i386__) || defined(__x86_64__)
#include <arch/x86/arch_cpu_type.h>
#elif defined(__aarch64__)
// TODO: This should be easy to adapt to arm32 as well
#include <arch/arm64/arch_cpu_type.h>
#else


static inline const char*
get_cpu_model_string(enum cpu_platform platform, enum cpu_vendor cpuVendor, uint32 cpuModel)
{
	// TODO: identification not yet implemented for this architecture
	return NULL;
}
#endif


static inline const char*
get_cpu_vendor_string(enum cpu_vendor cpuVendor)
{
	// Should match vendors in OS.h
	static const char* vendorStrings[] = {NULL, "AMD", "Cyrix", "IDT", "Intel",
		"National Semiconductor", "Rise", "Transmeta", "VIA", "IBM", "Motorola", "NEC", "Hygon",
		"Sun", "Fujitsu", "ARM", "Broadcom", "Cavium", "DEC", "HiSilicon", "Infineon", "Freescale",
		"NVIDIA", "Applied Micro", "Qualcomm", "Marvell", "Apple", "Ampere", "Microsoft"};

	if ((size_t)cpuVendor >= sizeof(vendorStrings) / sizeof(const char*))
		return NULL;
	return vendorStrings[cpuVendor];
}


static inline void
get_cpu_type(char* vendorBuffer, size_t vendorSize, char* modelBuffer, size_t modelSize)
{
	const char *vendor, *model;

	uint32 topologyNodeCount = 0;
	cpu_topology_node_info* topology = NULL;
	get_cpu_topology_info(NULL, &topologyNodeCount);
	if (topologyNodeCount != 0) {
		topology
			= (cpu_topology_node_info*)calloc(topologyNodeCount, sizeof(cpu_topology_node_info));
	}
	get_cpu_topology_info(topology, &topologyNodeCount);

	enum cpu_platform platform = B_CPU_UNKNOWN;
	enum cpu_vendor cpuVendor = B_CPU_VENDOR_UNKNOWN;
	uint32 cpuModel = 0;
	for (uint32 i = 0; i < topologyNodeCount; i++) {
		switch (topology[i].type) {
			case B_TOPOLOGY_ROOT:
				platform = topology[i].data.root.platform;
				break;

			case B_TOPOLOGY_PACKAGE:
				cpuVendor = topology[i].data.package.vendor;
				break;

			case B_TOPOLOGY_CORE:
				cpuModel = topology[i].data.core.model;
				break;

			default:
				break;
		}
	}
	free(topology);

	vendor = get_cpu_vendor_string(cpuVendor);
	if (vendor == NULL)
		vendor = "Unknown";

	model = get_cpu_model_string(platform, cpuVendor, cpuModel);
	if (model == NULL)
		model = "Unknown";

	strlcpy(vendorBuffer, vendor, vendorSize);
	strlcpy(modelBuffer, model, modelSize);
}


static inline int32
get_rounded_cpu_speed(void)
{
	uint32 topologyNodeCount = 0;
	cpu_topology_node_info* topology = NULL;
	get_cpu_topology_info(NULL, &topologyNodeCount);
	if (topologyNodeCount != 0) {
		topology
			= (cpu_topology_node_info*)calloc(topologyNodeCount, sizeof(cpu_topology_node_info));
	}
	get_cpu_topology_info(topology, &topologyNodeCount);

	uint64 cpuFrequency = 0;
	for (uint32 i = 0; i < topologyNodeCount; i++) {
		if (topology[i].type == B_TOPOLOGY_CORE) {
			cpuFrequency = topology[i].data.core.default_frequency;
			break;
		}
	}
	free(topology);

	int target, frac, delta;
	int freqs[] = {100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90};
	uint x;

	target = cpuFrequency / 1000000;
	frac = target % 100;
	delta = -frac;

	for (x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
		int ndelta = freqs[x] - frac;
		if (abs(ndelta) < abs(delta))
			delta = ndelta;
	}
	return target + delta;
}

#endif // CPU_TYPE_H

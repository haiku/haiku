/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTEL_EXTREME_PRIVATE_H
#define INTEL_EXTREME_PRIVATE_H


#include <KernelExport.h>
#include <PCI.h>

#include "intel_extreme.h"
#include "lock.h"


struct intel_info {
	uint32			cookie_magic;
	int32			open_count;
	int32			id;
	pci_info		*pci;
	uint8			*registers;
	area_id			registers_area;
	struct intel_shared_info *shared_info;
	area_id			shared_area;

	struct overlay_registers *overlay_registers;
		// update buffer, shares an area with shared_info

	uint8			*graphics_memory;
	area_id			graphics_memory_area;
	area_id			additional_memory_area;
		// allocated memory beyond the "stolen" amount
	mem_info		*memory_manager;

	bool			fake_interrupts;

	const char		*device_identifier;
	uint32			device_type;
};

extern status_t intel_extreme_init(intel_info &info);
extern void intel_extreme_uninit(intel_info &info);

#endif  /* INTEL_EXTREME_PRIVATE_H */

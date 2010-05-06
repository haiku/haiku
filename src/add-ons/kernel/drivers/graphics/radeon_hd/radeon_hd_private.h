/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef RADEON_RD_PRIVATE_H
#define RADEON_RD_PRIVATE_H


#include <AGP.h>
#include <KernelExport.h>
#include <PCI.h>


#include "radeon_hd.h"
#include "lock.h"


struct radeon_info {
	int32			open_count;
	status_t		init_status;
	int32			id;
	pci_info*		pci;
	addr_t			aperture_base;
	aperture_id		aperture;
	uint8*			registers;
	area_id			registers_area;
	area_id			framebuffer_area;

	struct radeon_shared_info* shared_info;
	area_id			shared_area;

	struct overlay_registers* overlay_registers;

	bool			fake_interrupts;

	const char*		device_identifier;
	DeviceType		device_type;
};

extern status_t radeon_free_memory(radeon_info& info, addr_t offset);
extern status_t radeon_allocate_memory(radeon_info& info, size_t size,
	size_t alignment, uint32 flags, addr_t* _offset,
	addr_t* _physicalBase = NULL);
extern status_t radeon_hd_init(radeon_info& info);
extern void radeon_hd_uninit(radeon_info& info);

#endif  /* RADEON_RD_PRIVATE_H */

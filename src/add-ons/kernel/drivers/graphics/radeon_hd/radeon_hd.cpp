/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "radeon_hd.h"

#include "AreaKeeper.h"
#include "driver.h"
#include "utility.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <driver_settings.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>


#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


//	#pragma mark -


#define RHD_FB_BAR   0
#define RHD_MMIO_BAR 2


status_t
radeon_hd_init(radeon_info &info)
{
	// memory mapped I/O
	
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("radeon hd shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(radeon_shared_info)), B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		return info.shared_area;
	}

	memset((void *)info.shared_info, 0, sizeof(radeon_shared_info));

	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void *)info.pci->u.h0.base_registers[RHD_MMIO_BAR],
		info.pci->u.h0.base_register_sizes[RHD_MMIO_BAR],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map memory I/O!\n");
		return info.registers_area;
	}

	AreaKeeper frambufferMapper;
	info.framebuffer_area = frambufferMapper.Map("radeon hd framebuffer",
		(void *)info.pci->u.h0.base_registers[RHD_FB_BAR],
		info.pci->u.h0.base_register_sizes[RHD_FB_BAR],
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void **)&info.shared_info->graphics_memory);
	if (frambufferMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map framebuffer!\n");
		return info.framebuffer_area;
	}
	
	// Turn on write combining for the area
	vm_set_area_memory_type(info.framebuffer_area,
		info.pci->u.h0.base_registers[RHD_FB_BAR], B_MTR_WC);
	
	sharedCreator.Detach();
	mmioMapper.Detach();
	frambufferMapper.Detach();

	info.shared_info->registers_area = info.registers_area;
	info.shared_info->frame_buffer_offset = 0;
	info.shared_info->physical_graphics_memory = info.pci->u.h0.base_registers[RHD_FB_BAR];

	TRACE((DEVICE_NAME "radeon_hd_init() completed successfully!\n"));
	return B_OK;
}


void
radeon_hd_uninit(radeon_info &info)
{
	TRACE((DEVICE_NAME ": radeon_hd_uninit()\n"));

	delete_area(info.shared_area);
	delete_area(info.registers_area);
	delete_area(info.framebuffer_area);
}


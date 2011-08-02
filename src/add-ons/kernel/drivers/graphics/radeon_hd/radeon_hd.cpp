/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "radeon_hd.h"

#include "AreaKeeper.h"
#include "driver.h"
#include "utility.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <boot_item.h>
#include <driver_settings.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>


#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x...) dprintf("radeon_hd: " x)
#else
#	define TRACE(x) ;
#endif


//	#pragma mark -


#define RHD_FB_BAR   0
#define RHD_MMIO_BAR 2


status_t
radeon_hd_init(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);

	// *** Map shared info
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("radeon hd shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(radeon_shared_info)), B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		return info.shared_area;
	}

	memset((void *)info.shared_info, 0, sizeof(radeon_shared_info));

	// *** Map Memory mapped IO
	// R6xx_R7xx_3D.pdf, 5.3.3.1 SET_CONFIG_REG
	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void *)info.pci->u.h0.base_registers[RHD_MMIO_BAR],
		info.pci->u.h0.base_register_sizes[RHD_MMIO_BAR],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": card (%ld): could not map memory I/O!\n",
			info.id);
		return info.registers_area;
	}

	// *** Framebuffer mapping
	AreaKeeper frambufferMapper;
	info.framebuffer_area = frambufferMapper.Map("radeon hd framebuffer",
		(void *)info.pci->u.h0.base_registers[RHD_FB_BAR],
		info.pci->u.h0.base_register_sizes[RHD_FB_BAR],
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void **)&info.shared_info->frame_buffer);
	if (frambufferMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": card(%ld): could not map framebuffer!\n",
			info.id);
		return info.framebuffer_area;
	}

	// *** VGA rom / AtomBIOS mapping
	AreaKeeper romMapper;
	info.rom_area = romMapper.Map("radeon hd rom",
		(void *)info.pci->u.h0.rom_base,
		info.pci->u.h0.rom_size,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void **)&info.shared_info->rom);
	if (romMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": card(%ld): could not map VGA rom!\n",
			info.id);
		return info.rom_area;
	}

	// Turn on write combining for the area
	vm_set_area_memory_type(info.framebuffer_area,
		info.pci->u.h0.base_registers[RHD_FB_BAR], B_MTR_WC);

	sharedCreator.Detach();
	mmioMapper.Detach();
	frambufferMapper.Detach();
	romMapper.Detach();

	// Pass common information to accelerant
	info.shared_info->device_id = info.device_id;
	info.shared_info->device_chipset = info.device_chipset;
	info.shared_info->registers_area = info.registers_area;
	info.shared_info->frame_buffer_area = info.framebuffer_area;
	info.shared_info->frame_buffer_phys
		= info.pci->u.h0.base_registers[RHD_FB_BAR];
	info.shared_info->frame_buffer_int
		= read32(info.registers + R6XX_CONFIG_FB_BASE);
	info.shared_info->rom_area = info.rom_area;
	info.shared_info->rom_phys = info.pci->u.h0.rom_base;
	info.shared_info->rom_size = info.pci->u.h0.rom_size;

	strcpy(info.shared_info->device_identifier, info.device_identifier);

	// Pull active monitor VESA EDID from boot loader
	edid1_info* edidInfo = (edid1_info*)get_boot_item(EDID_BOOT_INFO,
		NULL);
	if (edidInfo != NULL) {
		TRACE("card(%ld): %s found BIOS EDID information.\n", info.id,
			__func__);
		info.shared_info->has_edid = true;
		memcpy(&info.shared_info->edid_info, edidInfo, sizeof(edid1_info));
	} else {
		TRACE("card(%ld): %s didn't find BIOS EDID modes.\n", info.id,
			__func__);
		info.shared_info->has_edid = false;
	}

	// Populate graphics_memory/aperture_size with KB
	if (info.shared_info->device_chipset >= RADEON_R800) {
		// R800+ has memory stored in MB
		info.shared_info->graphics_memory_size
			= read32(info.registers + R6XX_CONFIG_MEMSIZE) * 1024;
		info.shared_info->frame_buffer_size
			= read32(info.registers + R6XX_CONFIG_APER_SIZE) * 1024;
	} else {
		// R600-R700 has memory stored in bytes
		info.shared_info->graphics_memory_size
			= read32(info.registers + R6XX_CONFIG_MEMSIZE) / 1024;
		info.shared_info->frame_buffer_size
			= read32(info.registers + R6XX_CONFIG_APER_SIZE) / 1024;
	}

	uint32 barSize = info.pci->u.h0.base_register_sizes[RHD_FB_BAR] / 1024;

	// if graphics memory is larger then PCI bar, just map bar
	if (info.shared_info->graphics_memory_size > barSize)
		info.shared_info->frame_buffer_size = barSize;
	else
		info.shared_info->frame_buffer_size
			= info.shared_info->graphics_memory_size;

	int32 memory_size = info.shared_info->graphics_memory_size / 1024;
	int32 frame_buffer_size = info.shared_info->frame_buffer_size / 1024;

	TRACE("card(%ld): Found %ld MB memory on card\n", info.id,
		memory_size);

	TRACE("card(%ld): Frame buffer aperture size is %ld MB\n", info.id,
		frame_buffer_size);

	TRACE("card(%ld): %s completed successfully!\n", info.id, __func__);
	return B_OK;
}


void
radeon_hd_uninit(radeon_info &info)
{
	TRACE("card(%ld): %s called\n", info.id, __func__);

	delete_area(info.shared_area);
	delete_area(info.registers_area);
	delete_area(info.framebuffer_area);
	delete_area(info.rom_area);
}


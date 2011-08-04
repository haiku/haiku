/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Fredrik Holmqvis, fredrik.holmqvist@gmail.com
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


inline bool
isAtomBIOS(uint8* bios)
{
	uint16 bios_header = RADEON_BIOS16(bios, 0x48);

	return !memcmp(&bios[bios_header + 4], "ATOM", 4) ||
		!memcmp(&bios[bios_header + 4], "MOTA", 4);
}


status_t
radeon_hd_getbios(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);

	// Enable ROM decoding
	uint32 rom_config = get_pci_config(info.pci, PCI_rom_base, 4);
	rom_config |= PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, rom_config);

	uint32 flags = get_pci_config(info.pci, PCI_rom_base, 4);
	if (flags & PCI_rom_enable)
		TRACE("%s: PCI ROM decode enabled successfully\n", __func__);

	uint32 rom_base = info.pci->u.h0.rom_base;
	uint32 rom_size = info.pci->u.h0.rom_size;

	if (rom_base == 0) {
		TRACE("%s: no PCI rom, trying shadow rom\n", __func__);
		// ROM has been copied by BIOS
		rom_base = 0xC0000;
		if (rom_size == 0) {
			rom_size = 0x7FFF;
			// A guess at maximum shadow bios size
		}
	}

	TRACE("%s: seeking rom at 0x%" B_PRIX32 " [size: 0x%" B_PRIX32 "]\n",
		__func__, rom_base, rom_size);

	uint8* bios;
	status_t result = B_ERROR;
	if (rom_base == 0 || rom_size == 0) {
		// FAIL: we never found a base to work off of.
		TRACE("%s: no VGA rom located, disabling AtomBIOS\n", __func__);
		result = B_ERROR;
	} else {
		area_id rom_area = map_physical_memory("radeon hd rom",
			rom_base, rom_size, B_ANY_KERNEL_ADDRESS, B_READ_AREA,
			(void **)&bios);

		if (info.rom_area < B_OK) {
			// FAIL : rom area wasn't mapped for access
			dprintf(DEVICE_NAME ": failed to map rom\n");
			result = B_ERROR;
		} else {
			if (bios[0] != 0x55 || bios[1] != 0xAA) {
				// FAIL : not a PCI rom
				uint16 id = bios[0] + (bios[1] << 8);
				TRACE("%s: this isn't a PCI rom (%X)\n", __func__, id);
				result = B_ERROR;
			} else if (isAtomBIOS(bios)) {
				info.rom_area = create_area("radeon hd AtomBIOS",
					(void **)&info.atom_buffer, B_ANY_KERNEL_ADDRESS,
					rom_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

				if (info.rom_area < 0) {
					// FAIL : couldn't create kernel AtomBIOS area
					dprintf(DEVICE_NAME ": %s: Error creating kernel"
						" AtomBIOS area!\n", __func__);
					result = B_ERROR;
				} else {
					memset((void*)info.atom_buffer, 0, rom_size);
						// Prevent unknown code execution by AtomBIOS parser
					memcpy(info.atom_buffer, (void *)bios, rom_size);
						// Copy AtomBIOS to kernel area

					if (isAtomBIOS(info.atom_buffer)) {
						// SUCCESS : bios copied and verified
						dprintf(DEVICE_NAME ": %s: AtomBIOS mapped!\n",
							__func__);
						set_area_protection(info.rom_area, B_READ_AREA);
							// Lock it down
						result = B_OK;
					} else {
						// FAIL : bios didn't copy properly for some reason
						dprintf(DEVICE_NAME ": %s: AtomBIOS not mapped!\n",
							__func__);
						result = B_ERROR;
					}
				}
			} else {
				dprintf(DEVICE_NAME ": %s: PCI rom found wasn't identified"
				" as AtomBIOS!\n", __func__);
				result = B_ERROR;
			}
		delete_area(rom_area);
		}
	}

	// Disable ROM decoding
	rom_config &= ~PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, rom_config);

	if (result == B_OK) {
		info.shared_info->rom_phys = rom_base;
		info.shared_info->rom_size = rom_size;
	}

	return result;
}


status_t
radeon_hd_getbios_r600(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);
	uint32 viph_control = read32(info.registers + RADEON_VIPH_CONTROL);
	uint32 bus_cntl = read32(info.registers + R600_BUS_CNTL);
	uint32 d1vga_control = read32(info.registers + AVIVO_D1VGA_CONTROL);
	uint32 d2vga_control = read32(info.registers + AVIVO_D2VGA_CONTROL);
	uint32 vga_render_control
		= read32(info.registers + AVIVO_VGA_RENDER_CONTROL);
	uint32 rom_cntl = read32(info.registers + R600_ROM_CNTL);
	uint32 general_pwrmgt = read32(info.registers + R600_GENERAL_PWRMGT);
	uint32 low_vid_lower_gpio_cntl
		= read32(info.registers + R600_LOW_VID_LOWER_GPIO_CNTL);
	uint32 medium_vid_lower_gpio_cntl
		= read32(info.registers + R600_MEDIUM_VID_LOWER_GPIO_CNTL);
	uint32 high_vid_lower_gpio_cntl
		= read32(info.registers + R600_HIGH_VID_LOWER_GPIO_CNTL);
	uint32 ctxsw_vid_lower_gpio_cntl
		= read32(info.registers + R600_CTXSW_VID_LOWER_GPIO_CNTL);
	uint32 lower_gpio_enable
		= read32(info.registers + R600_LOWER_GPIO_ENABLE);

	// disable VIP
	write32(info.registers + RADEON_VIPH_CONTROL,
		(viph_control & ~RADEON_VIPH_EN));
	// enable the rom
	write32(info.registers + R600_BUS_CNTL, (bus_cntl & ~R600_BIOS_ROM_DIS));
	// disable VGA mode
	write32(info.registers + AVIVO_D1VGA_CONTROL, (d1vga_control
		& ~(AVIVO_DVGA_CONTROL_MODE_ENABLE
			| AVIVO_DVGA_CONTROL_TIMING_SELECT)));
	write32(info.registers + D2VGA_CONTROL, (d2vga_control
		& ~(AVIVO_DVGA_CONTROL_MODE_ENABLE
			| AVIVO_DVGA_CONTROL_TIMING_SELECT)));
	write32(info.registers + AVIVO_VGA_RENDER_CONTROL,
		(vga_render_control & ~AVIVO_VGA_VSTATUS_CNTL_MASK));

	write32(info.registers + R600_ROM_CNTL,
		((rom_cntl & ~R600_SCK_PRESCALE_CRYSTAL_CLK_MASK)
		| (1 << R600_SCK_PRESCALE_CRYSTAL_CLK_SHIFT) | R600_SCK_OVERWRITE));

	write32(info.registers + R600_GENERAL_PWRMGT,
		(general_pwrmgt & ~R600_OPEN_DRAIN_PADS));
	write32(info.registers + R600_LOW_VID_LOWER_GPIO_CNTL,
		(low_vid_lower_gpio_cntl & ~0x400));
	write32(info.registers + R600_MEDIUM_VID_LOWER_GPIO_CNTL,
		(medium_vid_lower_gpio_cntl & ~0x400));
	write32(info.registers + R600_HIGH_VID_LOWER_GPIO_CNTL,
		(high_vid_lower_gpio_cntl & ~0x400));
	write32(info.registers + R600_CTXSW_VID_LOWER_GPIO_CNTL,
		(ctxsw_vid_lower_gpio_cntl & ~0x400));
	write32(info.registers + R600_LOWER_GPIO_ENABLE,
		(lower_gpio_enable | 0x400));

	status_t result = radeon_hd_getbios(info);

	// restore regs
	write32(info.registers + RADEON_VIPH_CONTROL, viph_control);
	write32(info.registers + R600_BUS_CNTL, bus_cntl);
	write32(info.registers + AVIVO_D1VGA_CONTROL, d1vga_control);
	write32(info.registers + AVIVO_D2VGA_CONTROL, d2vga_control);
	write32(info.registers + AVIVO_VGA_RENDER_CONTROL, vga_render_control);
	write32(info.registers + R600_ROM_CNTL, rom_cntl);
	write32(info.registers + R600_GENERAL_PWRMGT, general_pwrmgt);
	write32(info.registers + R600_LOW_VID_LOWER_GPIO_CNTL,
		low_vid_lower_gpio_cntl);
	write32(info.registers + R600_MEDIUM_VID_LOWER_GPIO_CNTL,
		medium_vid_lower_gpio_cntl);
	write32(info.registers + R600_HIGH_VID_LOWER_GPIO_CNTL,
		high_vid_lower_gpio_cntl);
	write32(info.registers + R600_CTXSW_VID_LOWER_GPIO_CNTL,
		ctxsw_vid_lower_gpio_cntl);
	write32(info.registers + R600_LOWER_GPIO_ENABLE, lower_gpio_enable);

	return result;
}


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
		dprintf(DEVICE_NAME ": card (%ld): couldn't map shared area!\n",
			info.id);
		return info.shared_area;
	}

	memset((void *)info.shared_info, 0, sizeof(radeon_shared_info));

	// *** Map Memory mapped IO
	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void *)info.pci->u.h0.base_registers[RHD_MMIO_BAR],
		info.pci->u.h0.base_register_sizes[RHD_MMIO_BAR],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": card (%ld): couldn't map memory I/O!\n",
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
		dprintf(DEVICE_NAME ": card(%ld): couldn't map framebuffer!\n",
			info.id);
		return info.framebuffer_area;
	}

	// Turn on write combining for the frame buffer area
	vm_set_area_memory_type(info.framebuffer_area,
		info.pci->u.h0.base_registers[RHD_FB_BAR], B_MTR_WC);

	sharedCreator.Detach();
	mmioMapper.Detach();
	frambufferMapper.Detach();

	// Pass common information to accelerant
	info.shared_info->device_id = info.device_id;
	info.shared_info->device_chipset = info.device_chipset;
	info.shared_info->registers_area = info.registers_area;
	strcpy(info.shared_info->device_identifier, info.device_identifier);

	info.shared_info->frame_buffer_area = info.framebuffer_area;
	info.shared_info->frame_buffer_phys
		= info.pci->u.h0.base_registers[RHD_FB_BAR];
	info.shared_info->frame_buffer_int
		= read32(info.registers + R6XX_CONFIG_FB_BASE);

	// *** AtomBIOS mapping

	// First we try an active bios read
	status_t biosStatus = radeon_hd_getbios(info);
	if (biosStatus != B_OK) {
		// If the active read fails, we do a disabled read
		if (info.device_chipset > RADEON_R600)
			biosStatus = radeon_hd_getbios_r600(info);
	}

	// TODO : may want to just return B_ERROR if AtomBIOS isn't
	// found as we will require it in the future

	info.shared_info->has_rom = (biosStatus == B_OK) ? true : false;
	info.shared_info->rom_area = (biosStatus == B_OK) ? info.rom_area : -1;

	// *** Pull active monitor VESA EDID from boot loader
	edid1_info* edidInfo
		= (edid1_info*)get_boot_item(EDID_BOOT_INFO, NULL);

	if (edidInfo != NULL) {
		TRACE("card(%ld): %s found VESA EDID information.\n", info.id,
			__func__);
		info.shared_info->has_edid = true;
		memcpy(&info.shared_info->edid_info, edidInfo, sizeof(edid1_info));
	} else {
		TRACE("card(%ld): %s didn't find VESA EDID modes.\n", info.id,
			__func__);
		info.shared_info->has_edid = false;
	}

	// *** Populate graphics_memory/aperture_size with KB
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


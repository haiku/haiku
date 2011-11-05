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

#define ERROR(x...) dprintf("radeon_hd: " x)


//	#pragma mark -


status_t
mapAtomBIOS(radeon_info &info, uint32 romBase, uint32 romSize)
{
	TRACE("%s: seeking AtomBIOS @ 0x%" B_PRIX32 " [size: 0x%" B_PRIX32 "]\n",
		__func__, romBase, romSize);

	uint8* rom;

	// attempt to access area specified
	area_id testArea = map_physical_memory("radeon hd rom probe",
		romBase, romSize, B_ANY_KERNEL_ADDRESS, B_READ_AREA,
		(void **)&rom);

	if (testArea < 0) {
		ERROR("%s: couldn't map potential rom @ 0x%" B_PRIX32
			"\n", __func__, romBase);
		return B_NO_MEMORY;
	}

	// check for valid BIOS signature
	if (rom[0] != 0x55 || rom[1] != 0xAA) {
		uint16 id = rom[0] + (rom[1] << 8);
		TRACE("%s: BIOS signature incorrect @ 0x%" B_PRIX32 " (%X)\n",
			__func__, romBase, id);
		delete_area(testArea);
		return B_ERROR;
	}

	// see if valid AtomBIOS rom
	uint16 romHeader = RADEON_BIOS16(rom, 0x48);
	bool romValid = !memcmp(&rom[romHeader + 4], "ATOM", 4)
		|| !memcmp(&rom[romHeader + 4], "MOTA", 4);

	if (romValid == false) {
		// FAIL : a PCI VGA bios but not AtomBIOS
		uint16 id = rom[0] + (rom[1] << 8);
		TRACE("%s: not AtomBIOS rom at 0x%" B_PRIX32 "(%X)\n",
			__func__, romBase, id);
		delete_area(testArea);
		return B_ERROR;
	}

	info.rom_area = create_area("radeon hd AtomBIOS",
		(void **)&info.atom_buffer, B_ANY_KERNEL_ADDRESS,
		romSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	if (info.rom_area < 0) {
		ERROR("%s: unable to map kernel AtomBIOS space!\n",
			__func__);
		delete_area(testArea);
		return B_NO_MEMORY;
	}

	memset((void*)info.atom_buffer, 0, romSize);
		// Prevent unknown code execution by AtomBIOS parser
	memcpy(info.atom_buffer, (void*)rom, romSize);
		// Copy AtomBIOS to kernel area

	// validate copied rom is valid
	romHeader = RADEON_BIOS16(info.atom_buffer, 0x48);
	romValid = !memcmp(&info.atom_buffer[romHeader + 4], "ATOM", 4)
		|| !memcmp(&info.atom_buffer[romHeader + 4], "MOTA", 4);

	if (romValid == true) {
		set_area_protection(info.rom_area, B_READ_AREA);
		ERROR("%s: AtomBIOS verified and locked\n", __func__);
	} else
		ERROR("%s: AtomBIOS memcpy failed!\n", __func__);

	delete_area(testArea);
	return romValid ? B_OK : B_ERROR;
}


status_t
radeon_hd_getbios(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);

	uint32 romBase = 0;
	uint32 romSize = 0;
	uint32 romMethod = 0;

	status_t mapResult = B_ERROR;

	// first we try to find the AtomBIOS rom via various methods
	for (romMethod = 0; romMethod < 3; romMethod++) {
		switch(romMethod) {
			case 0:
				// TODO: *** New ACPI method
				ERROR("%s: ACPI ATRM AtomBIOS TODO\n", __func__);
				break;
			case 1:
				// *** Discreet card on IGP, check PCI BAR 0
				// On post, the bios puts a copy of the IGP
				// AtomBIOS at the start of the video ram
				romBase = info.pci->u.h0.base_registers[PCI_BAR_FB];
				romSize = 256 * 1024;

				if (romBase == 0 || romSize == 0) {
					ERROR("%s: No base found at PCI FB BAR\n", __func__);
				} else {
					mapResult = mapAtomBIOS(info, romBase, romSize);
				}
				break;
			case 2:
			{
				// *** PCI ROM BAR
				// Enable ROM decoding for PCI BAR rom
				uint32 pciConfig = get_pci_config(info.pci, PCI_rom_base, 4);
				pciConfig |= PCI_rom_enable;
				set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

				uint32 flags = get_pci_config(info.pci, PCI_rom_base, 4);
				if ((flags & PCI_rom_enable) != 0)
					TRACE("%s: PCI ROM decode enabled\n", __func__);

				romBase = info.pci->u.h0.rom_base;
				romSize = info.pci->u.h0.rom_size;

				if (romBase == 0 || romSize == 0) {
					ERROR("%s: No base found at PCI ROM BAR\n", __func__);
				} else {
					mapResult = mapAtomBIOS(info, romBase, romSize);
				}

				// Disable ROM decoding
				pciConfig &= ~PCI_rom_enable;
				set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);
				break;
			}
		}

		if (mapResult == B_OK) {
			ERROR("%s: AtomBIOS found using active method %" B_PRIu32
				" at 0x%" B_PRIX32 "\n", __func__, romMethod, romBase);
			break;
		} else {
			ERROR("%s: AtomBIOS not found using active method %" B_PRIu32
				" at 0x%" B_PRIX32 "\n", __func__, romMethod, romBase);
		}
	}

	if (mapResult == B_OK) {
		info.shared_info->rom_phys = romBase;
		info.shared_info->rom_size = romSize;
	} else
		ERROR("%s: Active AtomBIOS search failed.\n", __func__);

	return mapResult;
}


status_t
radeon_hd_getbios_ni(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);
	uint32 bus_cntl = read32(info.registers + R600_BUS_CNTL);
	uint32 d1vga_control = read32(info.registers + AVIVO_D1VGA_CONTROL);
	uint32 d2vga_control = read32(info.registers + AVIVO_D2VGA_CONTROL);
	uint32 vga_render_control
		= read32(info.registers + AVIVO_VGA_RENDER_CONTROL);
	uint32 rom_cntl = read32(info.registers + R600_ROM_CNTL);

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

	write32(info.registers + R600_ROM_CNTL, (rom_cntl | R600_SCK_OVERWRITE));

	// try to grab the bios via PCI ROM bar
	// Enable ROM decoding for PCI BAR rom
	uint32 pciConfig = get_pci_config(info.pci, PCI_rom_base, 4);
	pciConfig |= PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

	uint32 flags = get_pci_config(info.pci, PCI_rom_base, 4);
	if (flags & PCI_rom_enable)
		TRACE("%s: PCI ROM decode enabled\n", __func__);

	uint32 romBase = info.pci->u.h0.rom_base;
	uint32 romSize = info.pci->u.h0.rom_size;

	status_t result = B_OK;
	if (romBase == 0 || romSize == 0) {
		ERROR("%s: No AtomBIOS found at PCI ROM BAR\n", __func__);
		result = B_ERROR;
	} else {
		result = mapAtomBIOS(info, romBase, romSize);
	}

	if (result == B_OK) {
		ERROR("%s: AtomBIOS found using disabled method at 0x%" B_PRIX32
			" [size: 0x%" B_PRIX32 "]\n", __func__, romBase, romSize);
		info.shared_info->rom_phys = romBase;
		info.shared_info->rom_size = romSize;
	}

	// Disable ROM decoding
	pciConfig &= ~PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

	// restore regs
	write32(info.registers + R600_BUS_CNTL, bus_cntl);
	write32(info.registers + AVIVO_D1VGA_CONTROL, d1vga_control);
	write32(info.registers + AVIVO_D2VGA_CONTROL, d2vga_control);
	write32(info.registers + AVIVO_VGA_RENDER_CONTROL, vga_render_control);
	write32(info.registers + R600_ROM_CNTL, rom_cntl);

	return result;
}


status_t
radeon_hd_getbios_r700(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);
	uint32 viph_control = read32(info.registers + RADEON_VIPH_CONTROL);
	uint32 bus_cntl = read32(info.registers + R600_BUS_CNTL);
	uint32 d1vga_control = read32(info.registers + AVIVO_D1VGA_CONTROL);
	uint32 d2vga_control = read32(info.registers + AVIVO_D2VGA_CONTROL);
	uint32 vga_render_control
		= read32(info.registers + AVIVO_VGA_RENDER_CONTROL);
	uint32 rom_cntl = read32(info.registers + R600_ROM_CNTL);

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

	write32(info.registers + R600_ROM_CNTL, (rom_cntl | R600_SCK_OVERWRITE));

	// try to grab the bios via PCI ROM bar
	// Enable ROM decoding for PCI BAR rom
	uint32 pciConfig = get_pci_config(info.pci, PCI_rom_base, 4);
	pciConfig |= PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

	uint32 flags = get_pci_config(info.pci, PCI_rom_base, 4);
	if (flags & PCI_rom_enable)
		TRACE("%s: PCI ROM decode enabled\n", __func__);

	uint32 romBase = info.pci->u.h0.rom_base;
	uint32 romSize = info.pci->u.h0.rom_size;

	status_t result = B_OK;
	if (romBase == 0 || romSize == 0) {
		ERROR("%s: No AtomBIOS found at PCI ROM BAR\n", __func__);
		result = B_ERROR;
	} else {
		result = mapAtomBIOS(info, romBase, romSize);
	}

	if (result == B_OK) {
		ERROR("%s: AtomBIOS found using disabled method at 0x%" B_PRIX32
			" [size: 0x%" B_PRIX32 "]\n", __func__, romBase, romSize);
		info.shared_info->rom_phys = romBase;
		info.shared_info->rom_size = romSize;
	}

	// Disable ROM decoding
	pciConfig &= ~PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

	// restore regs
	write32(info.registers + RADEON_VIPH_CONTROL, viph_control);
	write32(info.registers + R600_BUS_CNTL, bus_cntl);
	write32(info.registers + AVIVO_D1VGA_CONTROL, d1vga_control);
	write32(info.registers + AVIVO_D2VGA_CONTROL, d2vga_control);
	write32(info.registers + AVIVO_VGA_RENDER_CONTROL, vga_render_control);
	write32(info.registers + R600_ROM_CNTL, rom_cntl);

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

	// try to grab the bios via PCI ROM bar
	// Enable ROM decoding for PCI BAR rom
	uint32 pciConfig = get_pci_config(info.pci, PCI_rom_base, 4);
	pciConfig |= PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

	uint32 flags = get_pci_config(info.pci, PCI_rom_base, 4);
	if (flags & PCI_rom_enable)
		TRACE("%s: PCI ROM decode enabled\n", __func__);

	uint32 romBase = info.pci->u.h0.rom_base;
	uint32 romSize = info.pci->u.h0.rom_size;

	status_t result = B_OK;
	if (romBase == 0 || romSize == 0) {
		ERROR("%s: No AtomBIOS found at PCI ROM BAR\n", __func__);
		result = B_ERROR;
	} else {
		result = mapAtomBIOS(info, romBase, romSize);
	}

	if (result == B_OK) {
		ERROR("%s: AtomBIOS found using disabled method at 0x%" B_PRIX32
			" [size: 0x%" B_PRIX32 "]\n", __func__, romBase, romSize);
		info.shared_info->rom_phys = romBase;
		info.shared_info->rom_size = romSize;
	}

	// Disable ROM decoding
	pciConfig &= ~PCI_rom_enable;
	set_pci_config(info.pci, PCI_rom_base, 4, pciConfig);

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

	ERROR("%s: card(%ld): "
		"Radeon r%" B_PRIX16 " 1002:%" B_PRIX32 "\n",
		__func__, info.id, info.device_chipset, info.device_id);

	// *** Map shared info
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("radeon hd shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(radeon_shared_info)), B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		ERROR("%s: card (%ld): couldn't map shared area!\n",
			__func__, info.id);
		return info.shared_area;
	}

	memset((void *)info.shared_info, 0, sizeof(radeon_shared_info));
	sharedCreator.Detach();

	// *** Map Memory mapped IO
	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void *)info.pci->u.h0.base_registers[PCI_BAR_MMIO],
		info.pci->u.h0.base_register_sizes[PCI_BAR_MMIO],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		ERROR("%s: card (%ld): couldn't map memory I/O!\n",
			__func__, info.id);
		return info.registers_area;
	}
	mmioMapper.Detach();

	// *** Populate frame buffer information
	if (info.shared_info->device_chipset >= RADEON_R1000) {
		// Evergreen+ has memory stored in MB
		info.shared_info->graphics_memory_size
			= read32(info.registers + CONFIG_MEMSIZE) * 1024;
	} else {
		// R600-R700 has memory stored in bytes
		info.shared_info->graphics_memory_size
			= read32(info.registers + CONFIG_MEMSIZE) / 1024;
	}

	uint32 barSize = info.pci->u.h0.base_register_sizes[PCI_BAR_FB] / 1024;

	// if graphics memory is larger then PCI bar, just map bar
	if (info.shared_info->graphics_memory_size > barSize) {
		TRACE("%s: shrinking frame buffer to PCI bar...\n",
			__func__);
		info.shared_info->frame_buffer_size = barSize;
	} else {
		info.shared_info->frame_buffer_size
			= info.shared_info->graphics_memory_size;
	}

	TRACE("%s: mapping a frame buffer of %" B_PRIu32 "MB out of %" B_PRIu32
		"MB video ram\n", __func__, info.shared_info->frame_buffer_size / 1024,
		info.shared_info->graphics_memory_size / 1024);

	// *** Framebuffer mapping
	AreaKeeper frambufferMapper;
	info.framebuffer_area = frambufferMapper.Map("radeon hd frame buffer",
		(void *)info.pci->u.h0.base_registers[PCI_BAR_FB],
		info.shared_info->frame_buffer_size * 1024,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void **)&info.shared_info->frame_buffer);
	if (frambufferMapper.InitCheck() < B_OK) {
		ERROR("%s: card(%ld): couldn't map frame buffer!\n",
			__func__, info.id);
		return info.framebuffer_area;
	}

	// Turn on write combining for the frame buffer area
	vm_set_area_memory_type(info.framebuffer_area,
		info.pci->u.h0.base_registers[PCI_BAR_FB], B_MTR_WC);

	frambufferMapper.Detach();

	info.shared_info->frame_buffer_area = info.framebuffer_area;
	info.shared_info->frame_buffer_phys
		= info.pci->u.h0.base_registers[PCI_BAR_FB];

	// Pass common information to accelerant
	info.shared_info->device_index = info.id;
	info.shared_info->device_id = info.device_id;
	info.shared_info->device_chipset = info.device_chipset;
	info.shared_info->chipsetFlags = info.chipsetFlags;
	info.shared_info->dceMajor = info.dceMajor;
	info.shared_info->dceMinor = info.dceMinor;
	info.shared_info->registers_area = info.registers_area;
	strcpy(info.shared_info->device_identifier, info.device_identifier);

	// *** AtomBIOS mapping
	// First we try an active bios read
	status_t biosStatus = radeon_hd_getbios(info);

	if (biosStatus != B_OK) {
		// If the active read fails, we try a disabled read
		if (info.device_chipset >= (RADEON_R1000 | 0x20))
			biosStatus = radeon_hd_getbios_ni(info);
		else if (info.device_chipset >= RADEON_R700)
			biosStatus = radeon_hd_getbios_r700(info);
		else if (info.device_chipset >= RADEON_R600)
			biosStatus = radeon_hd_getbios_r600(info);
	}

	if (biosStatus != B_OK) {
		// *** very last resort, shadow bios VGA rom
		ERROR("%s: Can't find an AtomBIOS rom! Trying shadow rom...\n",
			__func__);

		// This works as long as the primary card is what this driver
		// is loaded for. Multiple cards may pose the risk of loading
		// the wrong AtomBIOS for the wrong card.

		uint32 romBase = 0xC0000;
		uint32 romSize = 128 * 1024;
			// what happens when AtomBIOS goes over 128Kb?
			// A Radeon HD 6990 has a 128Kb AtomBIOS

		if (mapAtomBIOS(info, romBase, romSize) == B_OK) {
			ERROR("%s: Found AtomBIOS at VGA shadow rom\n", __func__);
			// Whew!
			info.shared_info->rom_phys = romBase;
			info.shared_info->rom_size = romSize;
			biosStatus = B_OK;
		}
	}

	// Check if a valid AtomBIOS image was found.
	if (biosStatus != B_OK) {
		ERROR("%s: card (%ld): couldn't find AtomBIOS rom!\n",
			__func__, info.id);
		ERROR("%s: card (%ld): exiting. Please open a bug ticket"
			" at haiku-os.org with your /var/log/syslog\n",
			__func__, info.id);
		// Fallback to VESA (more likely crash app_server)
		return B_ERROR;
	}

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


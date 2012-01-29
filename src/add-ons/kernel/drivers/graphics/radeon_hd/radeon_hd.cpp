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
#include "sensors.h"

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


static const char radeon_chip_name[][16] = {
	"R420",
	"R423",
	"RV410",
	"RS400",
	"RS480",
	"RS600",
	"RS690",
	"RS740",
	"RV515",
	"R520",
	"RV530",
	"RV560",
	"RV570",
	"R580",
	"R600",
	"RV610",
	"RV630",
	"RV670",
	"RV620",
	"RV635",
	"RS780",
	"RS880",
	"RV770",
	"RV730",
	"RV710",
	"RV740",
	"Cedar",
	"Redwood",
	"Juniper",
	"Cypress",
	"Hemlock",
	"Palm",
	"Sumo",
	"Sumo2",
	"Caicos",
	"Turks",
	"Barts",
	"Cayman",
	"Antilles",
	"Lombok",
	"Thames",
	"Tahiti",
	"New Zealand"
};


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
		(void**)&rom);

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
		(void**)&info.atom_buffer, B_ANY_KERNEL_ADDRESS,
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


static status_t
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


static status_t
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
		ERROR("%s: No AtomBIOS location found at PCI ROM BAR\n", __func__);
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


static status_t
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
		ERROR("%s: No AtomBIOS location found at PCI ROM BAR\n", __func__);
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


static status_t
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
		ERROR("%s: No AtomBIOS location found at PCI ROM BAR\n", __func__);
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


static status_t
radeon_hd_getbios_avivo(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);
	uint32 sepromControl = read32(info.registers + RADEON_SEPROM_CNTL1);
	uint32 viphControl = read32(info.registers + RADEON_VIPH_CONTROL);
	uint32 busControl = read32(info.registers + RV370_BUS_CNTL);
	uint32 d1vgaControl = read32(info.registers + AVIVO_D1VGA_CONTROL);
	uint32 d2vgaControl = read32(info.registers + AVIVO_D2VGA_CONTROL);
	uint32 vgaRenderControl
		= read32(info.registers + AVIVO_VGA_RENDER_CONTROL);
	uint32 gpioPadA = read32(info.registers + RADEON_GPIOPAD_A);
	uint32 gpioPadEN = read32(info.registers + RADEON_GPIOPAD_EN);
	uint32 gpioPadMask = read32(info.registers + RADEON_GPIOPAD_MASK);

	write32(info.registers + RADEON_SEPROM_CNTL1,
		((sepromControl & ~RADEON_SCK_PRESCALE_MASK)
		| (0xc << RADEON_SCK_PRESCALE_SHIFT)));
	write32(info.registers + RADEON_GPIOPAD_A, 0);
	write32(info.registers + RADEON_GPIOPAD_EN, 0);
	write32(info.registers + RADEON_GPIOPAD_MASK, 0);

	// disable VIP
	write32(info.registers + RADEON_VIPH_CONTROL,
		(viphControl & ~RADEON_VIPH_EN));

	// enable the ROM
	write32(info.registers + RV370_BUS_CNTL,
		(busControl & ~RV370_BUS_BIOS_DIS_ROM));

	// disable VGA
	write32(info.registers + AVIVO_D1VGA_CONTROL,
		(d1vgaControl & ~(AVIVO_DVGA_CONTROL_MODE_ENABLE
		| AVIVO_DVGA_CONTROL_TIMING_SELECT)));
	write32(info.registers + AVIVO_D2VGA_CONTROL,
		(d2vgaControl & ~(AVIVO_DVGA_CONTROL_MODE_ENABLE
		| AVIVO_DVGA_CONTROL_TIMING_SELECT)));
	write32(info.registers + AVIVO_VGA_RENDER_CONTROL,
		(vgaRenderControl & ~AVIVO_VGA_VSTATUS_CNTL_MASK));

	uint32 romBase = info.pci->u.h0.rom_base;
	uint32 romSize = info.pci->u.h0.rom_size;

	status_t result = B_OK;
	if (romBase == 0 || romSize == 0) {
		ERROR("%s: No AtomBIOS location found at PCI ROM BAR\n", __func__);
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

	// restore registers
	write32(info.registers + RADEON_SEPROM_CNTL1, sepromControl);
	write32(info.registers + RADEON_VIPH_CONTROL, viphControl);
	write32(info.registers + RV370_BUS_CNTL, busControl);
	write32(info.registers + AVIVO_D1VGA_CONTROL, d1vgaControl);
	write32(info.registers + AVIVO_D2VGA_CONTROL, d2vgaControl);
	write32(info.registers + AVIVO_VGA_RENDER_CONTROL, vgaRenderControl);
	write32(info.registers + RADEON_GPIOPAD_A, gpioPadA);
	write32(info.registers + RADEON_GPIOPAD_EN, gpioPadEN);
	write32(info.registers + RADEON_GPIOPAD_MASK, gpioPadMask);

	return result;
}


status_t
radeon_hd_init(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);

	ERROR("%s: card(%ld): "
		"Radeon %s 1002:%" B_PRIX32 "\n", __func__, info.id,
		radeon_chip_name[info.chipsetID], info.pciID);

	// *** Map shared info
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("radeon hd shared info",
		(void**)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(radeon_shared_info)), B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		ERROR("%s: card (%ld): couldn't map shared area!\n",
			__func__, info.id);
		return info.shared_area;
	}

	memset((void*)info.shared_info, 0, sizeof(radeon_shared_info));
	sharedCreator.Detach();

	// *** Map Memory mapped IO
	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void*)info.pci->u.h0.base_registers[PCI_BAR_MMIO],
		info.pci->u.h0.base_register_sizes[PCI_BAR_MMIO],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		ERROR("%s: card (%ld): couldn't map memory I/O!\n",
			__func__, info.id);
		return info.registers_area;
	}
	mmioMapper.Detach();

	// *** Populate frame buffer information
	if (info.chipsetID >= RADEON_CEDAR) {
		if ((info.chipsetFlags & CHIP_APU) != 0
			|| (info.chipsetFlags & CHIP_IGP) != 0) {
			// Evergreen+ fusion in bytes
			info.shared_info->graphics_memory_size
				= read32(info.registers + CONFIG_MEMSIZE) / 1024;
		} else {
			// Evergreen+ has memory stored in MB
			info.shared_info->graphics_memory_size
				= read32(info.registers + CONFIG_MEMSIZE) * 1024;
		}
	} else if (info.chipsetID >= RADEON_R600) {
		// R600-R700 has memory stored in bytes
		info.shared_info->graphics_memory_size
			= read32(info.registers + CONFIG_MEMSIZE) / 1024;
	} else {
		// R420 - R600 cards
		// older cards use RADEON_CONFIG_MEMSIZE vs CONFIG_MEMSIZE
		if ((info.chipsetFlags & CHIP_IGP) != 0) {
			// NB_TOM holds amount of ram stolen for GPU
			uint32 tom = read32(info.registers + RADEON_NB_TOM);
			info.shared_info->graphics_memory_size
				= (((tom >> 16) - (tom & 0xffff) + 1) << 16);
			write32(info.registers + RADEON_CONFIG_MEMSIZE,
				info.shared_info->graphics_memory_size);
		} else {
			info.shared_info->graphics_memory_size
				= read32(info.registers + RADEON_CONFIG_MEMSIZE);
			if (info.shared_info->graphics_memory_size == 0) {
				// known bug if video memory == 8MB
				info.shared_info->graphics_memory_size = 8192;
				write32(info.registers + RADEON_CONFIG_MEMSIZE,
					info.shared_info->graphics_memory_size * 1024);
			}
		}
	}

	uint32 barSize = info.pci->u.h0.base_register_sizes[PCI_BAR_FB] / 1024;

	// if graphics memory is larger then PCI bar, just map bar
	if (info.shared_info->graphics_memory_size == 0) {
		// we can recover as we have PCI FB bar, but this should be fixed
		ERROR("%s: Error: found 0MB video ram, using PCI bar size...\n",
			__func__);
		info.shared_info->frame_buffer_size = barSize;
	} else if (info.shared_info->graphics_memory_size > barSize) {
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
		(void*)info.pci->u.h0.base_registers[PCI_BAR_FB],
		info.shared_info->frame_buffer_size * 1024,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void**)&info.shared_info->frame_buffer);
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
	info.shared_info->deviceIndex = info.id;
	info.shared_info->pciID = info.pciID;
	info.shared_info->chipsetID = info.chipsetID;
	info.shared_info->chipsetFlags = info.chipsetFlags;
	info.shared_info->dceMajor = info.dceMajor;
	info.shared_info->dceMinor = info.dceMinor;
	info.shared_info->registers_area = info.registers_area;
	strcpy(info.shared_info->deviceName, info.deviceName);
	strcpy(info.shared_info->chipsetName,
		radeon_chip_name[info.chipsetID]);

	// *** AtomBIOS mapping
	// First we try an active bios read
	status_t biosStatus = radeon_hd_getbios(info);

	if (biosStatus != B_OK) {
		// If the active read fails, we try a disabled read
		if (info.chipsetID >= RADEON_CAICOS)
			biosStatus = radeon_hd_getbios_ni(info);
		else if (info.chipsetID >= RADEON_RV770)
			biosStatus = radeon_hd_getbios_r700(info);
		else if (info.chipsetID >= RADEON_R600)
			biosStatus = radeon_hd_getbios_r600(info);
		else if (info.chipsetID >= RADEON_RS600)
			biosStatus = radeon_hd_getbios_avivo(info);
		// else legacy_read_disabled_bios
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

	TRACE("card(%ld): GPU thermal status: %" B_PRId32 "C\n", info.id,
		radeon_thermal_query(info) / 1000);

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


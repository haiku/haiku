/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <Debug.h>

#include "bios.h"

#include "accelerant.h"
#include "accelerant_protos.h"


#undef TRACE

#define TRACE_ATOM
#ifdef TRACE_ATOM
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


atom_context *gAtomContext;


status_t
bios_read_enabled(void* bios, size_t size)
{
	status_t result = B_ERROR;
	if (gInfo->rom[0] == 0x55 && gInfo->rom[1] == 0xaa) {
		TRACE("%s: found AtomBIOS signature!\n", __func__);
		bios = gInfo->rom;
		result = B_OK;
	} else
		TRACE("%s: didn't find valid AtomBIOS\n", __func__);

	return result;
}


status_t
bios_read_disabled_northern(void* bios, size_t size)
{
	uint32 bus_cntl = Read32(OUT, R600_BUS_CNTL);
	uint32 d1vga_control = Read32(OUT, D1VGA_CONTROL);
	uint32 d2vga_control = Read32(OUT, D2VGA_CONTROL);
	uint32 vga_render_control = Read32(OUT, VGA_RENDER_CONTROL);
	uint32 rom_cntl = Read32(OUT, R600_ROM_CNTL);

	// Enable rom access
	Write32(OUT, R600_BUS_CNTL, (bus_cntl & ~R600_BIOS_ROM_DIS));
	// Disable VGA mode
	Write32(OUT, D1VGA_CONTROL, (d1vga_control
		& ~(DVGA_CONTROL_MODE_ENABLE
			| DVGA_CONTROL_TIMING_SELECT)));
	Write32(OUT, D2VGA_CONTROL, (d2vga_control
		& ~(DVGA_CONTROL_MODE_ENABLE
			| DVGA_CONTROL_TIMING_SELECT)));
	Write32(OUT, VGA_RENDER_CONTROL, (vga_render_control
		& ~VGA_VSTATUS_CNTL_MASK));
	Write32(OUT, R600_ROM_CNTL, rom_cntl | R600_SCK_OVERWRITE);

	snooze(2);

	status_t result = B_ERROR;
	if (gInfo->rom[0] == 0x55 && gInfo->rom[1] == 0xaa) {
		TRACE("%s: found AtomBIOS signature!\n", __func__);
		memcpy(&bios, gInfo->rom, size);
			// grab it while we can
		result = B_OK;
	} else
		TRACE("%s: didn't find valid AtomBIOS\n", __func__);

	// restore regs
	Write32(OUT, R600_BUS_CNTL, bus_cntl);
	Write32(OUT, D1VGA_CONTROL, d1vga_control);
	Write32(OUT, D2VGA_CONTROL, d2vga_control);
	Write32(OUT, VGA_RENDER_CONTROL, vga_render_control);
	Write32(OUT, R600_ROM_CNTL, rom_cntl);

	return result;
}


status_t
bios_read_disabled_avivo(void* bios, size_t size)
{
	uint32 seprom_cntl1 = Read32(OUT, RADEON_SEPROM_CNTL1);
	uint32 viph_control = Read32(OUT, RADEON_VIPH_CONTROL);
	uint32 bus_cntl = Read32(OUT, RV370_BUS_CNTL);
	uint32 d1vga_control = Read32(OUT, D1VGA_CONTROL);
	uint32 d2vga_control = Read32(OUT, D2VGA_CONTROL);
	uint32 vga_render_control = Read32(OUT, VGA_RENDER_CONTROL);
	uint32 gpiopad_a = Read32(OUT, RADEON_GPIOPAD_A);
	uint32 gpiopad_en = Read32(OUT, RADEON_GPIOPAD_EN);
	uint32 gpiopad_mask = Read32(OUT, RADEON_GPIOPAD_MASK);

	Write32(OUT, RADEON_SEPROM_CNTL1, ((seprom_cntl1 &
		~RADEON_SCK_PRESCALE_MASK) | (0xc << RADEON_SCK_PRESCALE_SHIFT)));
	Write32(OUT, RADEON_GPIOPAD_A, 0);
	Write32(OUT, RADEON_GPIOPAD_EN, 0);
	Write32(OUT, RADEON_GPIOPAD_MASK, 0);

	// Disable VIP
	Write32(OUT, RADEON_VIPH_CONTROL, (viph_control & ~RADEON_VIPH_EN));
	// Disable VGA mode
	Write32(OUT, D1VGA_CONTROL, (d1vga_control
		& ~(DVGA_CONTROL_MODE_ENABLE
			| DVGA_CONTROL_TIMING_SELECT)));
	Write32(OUT, D2VGA_CONTROL, (d2vga_control
		& ~(DVGA_CONTROL_MODE_ENABLE
			| DVGA_CONTROL_TIMING_SELECT)));
	Write32(OUT, VGA_RENDER_CONTROL, (vga_render_control
		& ~VGA_VSTATUS_CNTL_MASK));

	snooze(2);

	status_t result = B_ERROR;
	if (gInfo->rom[0] == 0x55 && gInfo->rom[1] == 0xaa) {
		TRACE("%s: found AtomBIOS signature!\n", __func__);
		memcpy(&bios, gInfo->rom, size);
			// grab it while we can
		result = B_OK;
	} else
		TRACE("%s: didn't find valid AtomBIOS\n", __func__);

	/* restore regs */
	Write32(OUT, RADEON_SEPROM_CNTL1, seprom_cntl1);
	Write32(OUT, RADEON_VIPH_CONTROL, viph_control);
	Write32(OUT, RV370_BUS_CNTL, bus_cntl);
	Write32(OUT, D1VGA_CONTROL, d1vga_control);
	Write32(OUT, D2VGA_CONTROL, d2vga_control);
	Write32(OUT, VGA_RENDER_CONTROL, vga_render_control);
	Write32(OUT, RADEON_GPIOPAD_A, gpiopad_a);
	Write32(OUT, RADEON_GPIOPAD_EN, gpiopad_en);
	Write32(OUT, RADEON_GPIOPAD_MASK, gpiopad_mask);


	return result;
}


status_t
bios_read_disabled_r700(void* bios, size_t size)
{
	uint32 viph_control = Read32(OUT, RADEON_VIPH_CONTROL);
	uint32 bus_cntl = Read32(OUT, R600_BUS_CNTL);
	uint32 d1vga_control = Read32(OUT, D1VGA_CONTROL);
	uint32 d2vga_control = Read32(OUT, D2VGA_CONTROL);
	uint32 vga_render_control = Read32(OUT, VGA_RENDER_CONTROL);
	uint32 rom_cntl = Read32(OUT, R600_ROM_CNTL);

	// Disable VIP
	Write32(OUT, RADEON_VIPH_CONTROL, (viph_control & ~RADEON_VIPH_EN));
	// Enable rom access
	Write32(OUT, R600_BUS_CNTL, (bus_cntl & ~R600_BIOS_ROM_DIS));
	// Disable VGA mode
	Write32(OUT, D1VGA_CONTROL, (d1vga_control
		& ~(DVGA_CONTROL_MODE_ENABLE
			| DVGA_CONTROL_TIMING_SELECT)));
	Write32(OUT, D2VGA_CONTROL, (d2vga_control
		& ~(DVGA_CONTROL_MODE_ENABLE
			| DVGA_CONTROL_TIMING_SELECT)));
	Write32(OUT, VGA_RENDER_CONTROL, (vga_render_control
		& ~VGA_VSTATUS_CNTL_MASK));

	uint32 cg_spll_func_cntl = 0;
	radeon_shared_info &info = *gInfo->shared_info;
	if (info.device_chipset == (RADEON_R700 | 0x30)) {
		cg_spll_func_cntl = Read32(OUT, R600_CG_SPLL_FUNC_CNTL);

		// Enable bypass mode
		Write32(OUT, R600_CG_SPLL_FUNC_CNTL, cg_spll_func_cntl
			| R600_SPLL_BYPASS_EN);

		// wait for SPLL_CHG_STATUS to change to 1
		uint32 cg_spll_status = 0;
		while (!(cg_spll_status & R600_SPLL_CHG_STATUS))
			cg_spll_status = Read32(OUT, R600_CG_SPLL_STATUS);

		Write32(OUT, R600_ROM_CNTL, (rom_cntl & ~R600_SCK_OVERWRITE));
	} else
		Write32(OUT, R600_ROM_CNTL, rom_cntl | R600_SCK_OVERWRITE);

	snooze(2);

	status_t result = B_ERROR;
	if (gInfo->rom[0] == 0x55 && gInfo->rom[1] == 0xaa) {
		TRACE("%s: found AtomBIOS signature!\n", __func__);
		memcpy(&bios, gInfo->rom, size);
			// grab it while we can
		result = B_OK;
	} else
		TRACE("%s: didn't find valid AtomBIOS\n", __func__);

	// restore regs
	if (info.device_chipset == (RADEON_R700 | 0x30)) {
		Write32(OUT, R600_CG_SPLL_FUNC_CNTL, cg_spll_func_cntl);

		// wait for SPLL_CHG_STATUS to change to 1
		uint32 cg_spll_status = 0;
		while (!(cg_spll_status & R600_SPLL_CHG_STATUS))
			cg_spll_status = Read32(OUT, R600_CG_SPLL_STATUS);
	}
	Write32(OUT, RADEON_VIPH_CONTROL, viph_control);
	Write32(OUT, R600_BUS_CNTL, bus_cntl);
	Write32(OUT, D1VGA_CONTROL, d1vga_control);
	Write32(OUT, D2VGA_CONTROL, d2vga_control);
	Write32(OUT, VGA_RENDER_CONTROL, vga_render_control);
	Write32(OUT, R600_ROM_CNTL, rom_cntl);

	return result;
}


status_t
radeon_init_bios(void* bios)
{
	radeon_shared_info &info = *gInfo->shared_info;

	status_t bios_status;
	if (bios_read_enabled(bios, info.rom_size) != B_OK) {
		if (info.device_chipset > RADEON_R800) // TODO : >= BARTS
			bios_status = bios_read_disabled_northern(bios, info.rom_size);
		else if (info.device_chipset >= (RADEON_R700 | 0x70))
			bios_status = bios_read_disabled_r700(bios, info.rom_size);
		else if (info.device_chipset >= RADEON_R600)
			bios_status = bios_read_disabled_avivo(bios, info.rom_size);
		else
			bios_status = B_ERROR;
	}

	if (bios_status != B_OK)
		return bios_status;

	struct card_info *atom_card_info
		= (card_info*)malloc(sizeof(card_info));

	if (!atom_card_info)
		return B_NO_MEMORY;

	atom_card_info->reg_read = _read32;
	atom_card_info->reg_write = _write32;

	if (false) {
		// TODO : if rio_mem, use ioreg
		//atom_card_info->ioreg_read = cail_ioreg_read;
		//atom_card_info->ioreg_write = cail_ioreg_write;
	} else {
		TRACE("%s: Cannot find PCI I/O BAR; using MMIO\n", __func__);
		atom_card_info->ioreg_read = _read32;
		atom_card_info->ioreg_write = _write32;
	}
	atom_card_info->mc_read = _read32;
	atom_card_info->mc_write = _write32;
	atom_card_info->pll_read = _read32;
	atom_card_info->pll_write = _write32;

	// Point AtomBIOS parser to card bios and malloc gAtomContext
	gAtomContext = atom_parse(atom_card_info, bios);

	if (gAtomContext == NULL) {
		TRACE("%s: couldn't parse system AtomBIOS\n", __func__);
		return B_ERROR;
	}

	#if 0
	rdev->mode_info.atom_context = atom_parse(atom_card_info, rdev->bios);
	mutex_init(&rdev->mode_info.atom_context->mutex);
	radeon_atom_initialize_bios_scratch_regs(rdev->ddev);
	atom_allocate_fb_scratch(rdev->mode_info.atom_context);
	#endif

	return B_OK;
}

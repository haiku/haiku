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


void
radeon_bios_init_scratch()
{
	radeon_shared_info &info = *gInfo->shared_info;

	uint32 biosScratch2;
	uint32 biosScratch6;

	if (info.device_chipset >= RADEON_R600) {
		biosScratch2 = Read32(OUT, R600_BIOS_2_SCRATCH);
		biosScratch6 = Read32(OUT, R600_BIOS_6_SCRATCH);
	} else {
		biosScratch2 = Read32(OUT, RADEON_BIOS_2_SCRATCH);
		biosScratch6 = Read32(OUT, RADEON_BIOS_6_SCRATCH);
	}

	biosScratch2 &= ~ATOM_S2_VRI_BRIGHT_ENABLE;
		// bios should control backlight
	biosScratch6 |= ATOM_S6_ACC_BLOCK_DISPLAY_SWITCH;
		// bios shouldn't handle mode switching

	if (info.device_chipset >= RADEON_R600) {
		Write32(OUT, R600_BIOS_2_SCRATCH, biosScratch2);
		Write32(OUT, R600_BIOS_6_SCRATCH, biosScratch6);
	} else {
		Write32(OUT, RADEON_BIOS_2_SCRATCH, biosScratch2);
		Write32(OUT, RADEON_BIOS_6_SCRATCH, biosScratch6);
	}
}


bool
radeon_bios_isposted()
{
	// aka, is primary graphics card that POST loaded

	radeon_shared_info &info = *gInfo->shared_info;
	uint32 reg;

	if (info.device_chipset == RADEON_PALM) {
		// palms
		reg = Read32(OUT, EVERGREEN_CRTC_CONTROL
			+ EVERGREEN_CRTC0_REGISTER_OFFSET)
			| Read32(OUT, EVERGREEN_CRTC_CONTROL
			+ EVERGREEN_CRTC1_REGISTER_OFFSET);
		if ((reg & EVERGREEN_CRTC_MASTER_EN) != 0)
			return true;
	} else if (info.device_chipset >= RADEON_CEDAR) {
		// evergreen or higher
		reg = Read32(OUT, EVERGREEN_CRTC_CONTROL
				+ EVERGREEN_CRTC0_REGISTER_OFFSET)
			| Read32(OUT, EVERGREEN_CRTC_CONTROL
				+ EVERGREEN_CRTC1_REGISTER_OFFSET)
			| Read32(OUT, EVERGREEN_CRTC_CONTROL
				+ EVERGREEN_CRTC2_REGISTER_OFFSET)
			| Read32(OUT, EVERGREEN_CRTC_CONTROL
				+ EVERGREEN_CRTC3_REGISTER_OFFSET)
			| Read32(OUT, EVERGREEN_CRTC_CONTROL
				+ EVERGREEN_CRTC4_REGISTER_OFFSET)
			| Read32(OUT, EVERGREEN_CRTC_CONTROL
				+ EVERGREEN_CRTC5_REGISTER_OFFSET);
		if ((reg & EVERGREEN_CRTC_MASTER_EN) != 0)
			return true;
	} else if (info.device_chipset >= RADEON_R420) {
		// avivio through r700
		reg = Read32(OUT, AVIVO_D1CRTC_CONTROL) |
			Read32(OUT, AVIVO_D2CRTC_CONTROL);
		if ((reg & AVIVO_CRTC_EN) != 0) {
			return true;
		}
	}

	return false;
}


status_t
radeon_init_bios(uint8* bios)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.has_rom == false) {
		TRACE("%s: called even though has_rom == false\n", __func__);
		return B_ERROR;
	}

	#ifdef TRACE_ATOM
	radeon_dump_bios();
	#endif

	struct card_info *atom_card_info
		= (card_info*)malloc(sizeof(card_info));

	if (!atom_card_info)
		return B_NO_MEMORY;

	atom_card_info->reg_read = Read32Cail;
	atom_card_info->reg_write = Write32Cail;

	// use MMIO instead of PCI I/O BAR
	atom_card_info->ioreg_read = Read32Cail;
	atom_card_info->ioreg_write = Write32Cail;

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

	if ((gAtomContext->exec_sem = create_sem(1, "AtomBIOS_exec"))
		< B_NO_ERROR) {
		TRACE("%s: couldn't create semaphore for AtomBIOS exec thread!\n",
			__func__);
		return B_ERROR;
	}

	radeon_bios_init_scratch();
	atom_allocate_fb_scratch(gAtomContext);

	// post card atombios if needed
	if (radeon_bios_isposted() == false) {
		TRACE("%s: init AtomBIOS for this card as it is not not posted\n",
			__func__);
		// radeon_gpu_reset();	// <= r500 only?
		atom_asic_init(gAtomContext);
	} else {
		TRACE("%s: AtomBIOS is already posted\n",
			__func__);
	}

	return B_OK;
}


status_t
radeon_dump_bios()
{
	// For debugging use, dump card AtomBIOS
	radeon_shared_info &info = *gInfo->shared_info;

	TRACE("%s: Dumping AtomBIOS as ATOM_DEBUG is set...\n",
		__func__);

	FILE* fp;
	char filename[255];
	sprintf(filename, "/boot/common/cache/tmp/radeon_hd_bios_1002_%" B_PRIx32
		"_%" B_PRIu32 ".bin", info.device_id, info.device_index);

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		TRACE("%s: Cannot create AtomBIOS blob at %s\n", __func__, filename);
		return B_ERROR;
	}

	fwrite(gInfo->rom, info.rom_size, 1, fp);

	fclose(fp);

	TRACE("%s: AtomBIOS dumped to %s\n", __func__, filename);

	return B_OK;
}

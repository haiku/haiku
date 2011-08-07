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

	uint32 bios_2_scratch;
	uint32 bios_6_scratch;

	if (info.device_chipset >= RADEON_R600) {
		bios_2_scratch = Read32(OUT, R600_BIOS_2_SCRATCH);
		bios_6_scratch = Read32(OUT, R600_BIOS_6_SCRATCH);
	} else {
		bios_2_scratch = Read32(OUT, RADEON_BIOS_2_SCRATCH);
		bios_6_scratch = Read32(OUT, RADEON_BIOS_6_SCRATCH);
	}

	bios_2_scratch &= ~ATOM_S2_VRI_BRIGHT_ENABLE;
		// bios should control backlight
	bios_6_scratch |= ATOM_S6_ACC_BLOCK_DISPLAY_SWITCH;
		// bios shouldn't handle mode switching

	if (info.device_chipset >= RADEON_R600) {
		Write32(OUT, R600_BIOS_2_SCRATCH, bios_2_scratch);
		Write32(OUT, R600_BIOS_6_SCRATCH, bios_6_scratch);
	} else {
		Write32(OUT, RADEON_BIOS_2_SCRATCH, bios_2_scratch);
		Write32(OUT, RADEON_BIOS_6_SCRATCH, bios_6_scratch);
	}
}


status_t
radeon_init_bios(uint8* bios)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.has_rom == false) {
		TRACE("%s: called even though has_rom == false\n", __func__);
		return B_ERROR;
	}

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

	if ((gAtomContext->exec_sem = create_sem(1, "AtomBIOS_exec"))
		< B_NO_ERROR) {
		TRACE("%s: couldn't create semaphore for AtomBIOS exec thread!\n",
			__func__);
		return B_ERROR;
	}

	atom_asic_init(gAtomContext);
		// Post card

	radeon_bios_init_scratch();

	return B_OK;
}

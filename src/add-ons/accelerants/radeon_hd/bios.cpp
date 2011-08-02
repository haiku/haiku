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


atom_context *gAtomBIOS;


status_t
bios_init()
{
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

	// Point AtomBIOS parser to card bios and malloc gAtomBIOS
	gAtomBIOS = atom_parse(atom_card_info, (void*)gInfo->shared_info->rom_base);

	if (gAtomBIOS == NULL) {
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

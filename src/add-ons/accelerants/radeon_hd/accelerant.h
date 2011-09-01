/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_ACCELERANT_H
#define RADEON_HD_ACCELERANT_H


#include "atom.h"
#include "mode.h"
#include "radeon_hd.h"
#include "pll.h"
#include "dac.h"
#include "tmds.h"
#include "lvds.h"


#include <ByteOrder.h>
#include <edid.h>


#define MAX_DISPLAY 2
	// Maximum displays (more then two requires AtomBIOS)


typedef struct {
	uint32 d1vga_control;
	uint32 d2vga_control;
	uint32 vga_render_control;
	uint32 vga_hdp_control;
	uint32 d1crtc_control;
	uint32 d2crtc_control;
} gpu_mc_info;


struct accelerant_info {
	vuint8			*regs;
	area_id			regs_area;

	radeon_shared_info *shared_info;
	area_id			shared_info_area;

	display_mode	*mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	uint8*			rom;
	area_id			rom_area;

	edid1_info		edid_info;
	bool			has_edid;

	int				device;
	bool			is_clone;

	gpu_mc_info		*mc_info;		// used for last known mc state

	// LVDS panel mode passed from the bios/startup.
	display_mode	lvds_panel_mode;
};


struct register_info {
	uint16	vgaControl;
	uint16	grphEnable;
	uint16	grphUpdate;
	uint16	grphControl;
	uint16	grphSwapControl;
	uint16	grphPrimarySurfaceAddr;
	uint16	grphSecondarySurfaceAddr;
	uint16	grphPrimarySurfaceAddrHigh;
	uint16	grphSecondarySurfaceAddrHigh;
	uint16	grphPitch;
	uint16	grphSurfaceOffsetX;
	uint16	grphSurfaceOffsetY;
	uint16	grphXStart;
	uint16	grphYStart;
	uint16	grphXEnd;
	uint16	grphYEnd;
	uint16	crtControl;
	uint16	crtCountControl;
	uint16	crtInterlace;
	uint16	crtHPolarity;
	uint16	crtVPolarity;
	uint16	crtHSync;
	uint16	crtVSync;
	uint16	crtHBlank;
	uint16	crtVBlank;
	uint16	crtHTotal;
	uint16	crtVTotal;
	uint16	crtcOffset;
	uint16	modeDesktopHeight;
	uint16	modeDataFormat;
	uint16	modeCenter;
	uint16	viewportStart;
	uint16	viewportSize;
	uint16	sclUpdate;
	uint16	sclEnable;
	uint16	sclTapControl;
};


struct pll_info {
	/* reference frequency */
	uint32 reference_freq;

	/* fixed dividers */
	uint32 reference_div;
	uint32 post_div;

	/* pll in/out limits */
	uint32 pll_in_min;
	uint32 pll_in_max;
	uint32 pll_out_min;
	uint32 pll_out_max;
	uint32 lcd_pll_out_min;
	uint32 lcd_pll_out_max;
	uint32 best_vco;

	/* divider limits */
	uint32 min_ref_div;
	uint32 max_ref_div;
	uint32 min_post_div;
	uint32 max_post_div;
	uint32 min_feedback_div;
	uint32 max_feedback_div;
	uint32 min_frac_feedback_div;
	uint32 max_frac_feedback_div;

	/* flags for the current clock */
	uint32 flags;

	/* pll id */
	uint32 id;
};


typedef struct {
	bool valid;
	uint16 line_mux;
	uint16 connector_flags;
	uint32 connector_type;
	uint16 connector_object_id;
	uint32 encoder_type;
	uint16 encoder_object_id;
	// TODO struct radeon_i2c_bus_rec ddc_bus;
	// TODO struct radeon_hpd hpd;
} connector_info;


typedef struct {
	bool			active;
	uint32			connector_index; // matches connector id in connector_info
	register_info	*regs;
	bool			found_ranges;
	uint32			vfreq_max;
	uint32			vfreq_min;
	uint32			hfreq_max;
	uint32			hfreq_min;
	pll_info		pll;
} display_info;


// register MMIO modes
#define OUT 0x1	// Direct MMIO calls
#define CRT 0x2	// Crt controller calls
#define VGA 0x3 // Vga calls
#define PLL 0x4 // PLL calls
#define MC	0x5 // Memory controller calls


extern accelerant_info *gInfo;
extern atom_context *gAtomContext;
extern display_info *gDisplay[MAX_DISPLAY];
extern connector_info *gConnector[ATOM_MAX_SUPPORTED_DEVICE];


// register access

inline uint32
_read32(uint32 offset)
{
	return *(volatile uint32 *)(gInfo->regs + offset);
}


inline void
_write32(uint32 offset, uint32 value)
{
	*(volatile uint32 *)(gInfo->regs + offset) = value;
}


// AtomBIOS cail register calls (are *4... no clue why)
inline uint32
Read32Cail(uint32 offset)
{
	return _read32(offset * 4);
}


inline void
Write32Cail(uint32 offset, uint32 value)
{
	_write32(offset * 4, value);
}


inline uint32
Read32(uint32 subsystem, uint32 offset)
{
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case CRT:
		case PLL:
			return _read32(offset);
		case MC:
			return _read32(offset);
	};
}


inline void
Write32(uint32 subsystem, uint32 offset, uint32 value)
{
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case CRT:
		case PLL:
			_write32(offset, value);
			return;
		case MC:
			_write32(offset, value);
			return;
	};
}


inline void
Write32Mask(uint32 subsystem, uint32 offset, uint32 value, uint32 mask)
{
	uint32 temp;
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case MC:
			temp = _read32(offset);
			break;
		case CRT:
			temp = _read32(offset);
			break;
		case PLL:
			temp = _read32(offset);
			//temp = _read32PLL(offset);
			break;
	};

	// only effect mask
	temp &= ~mask;
	temp |= value & mask;

	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case MC:
			_write32(offset, temp);
			return;
		case CRT:
			_write32(offset, temp);
			return;
		case PLL:
			_write32(offset, temp);
			//_write32PLL(offset, temp);
			return;
	};
}


#endif	/* RADEON_HD_ACCELERANT_H */

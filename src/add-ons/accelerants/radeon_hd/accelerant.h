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


#include "mode.h"
#include "radeon_hd.h"
#include "pll.h"


#include <edid.h>


struct accelerant_info {
	vuint8			*regs;
	area_id			regs_area;

	radeon_shared_info *shared_info;
	area_id			shared_info_area;

	display_mode	*mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	edid1_info		edid_info;
	bool			has_edid;

	int				device;
	bool			is_clone;

	// LVDS panel mode passed from the bios/startup.
	display_mode	lvds_panel_mode;
};


struct register_info {
	uint16	crtid;
	uint16	vgaControl;
	uint16	grphEnable;
	uint16	grphControl;
	uint16	grphSwapControl;
	uint16	grphPrimarySurfaceAddr;
	uint16	grphPrimarySurfaceAddrHigh;
	uint16	grphSecondarySurfaceAddr;
	uint16	grphSecondarySurfaceAddrHigh;
	uint16	grphPitch;
	uint16	grphSurfaceOffsetX;
	uint16	grphSurfaceOffsetY;
	uint16	grphXStart;
	uint16	grphYStart;
	uint16	grphXEnd;
	uint16	grphYEnd;
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
	uint16	modeDesktopHeight;
	uint16	modeDataFormat;
	uint16	modeCenter;
	uint16	viewportStart;
	uint16	viewportSize;
	uint16	sclUpdate;
	uint16	sclEnable;
	uint16	sclTapControl;
};


#define HEAD_MODE_A_ANALOG		0x01
#define HEAD_MODE_B_DIGITAL		0x02
#define HEAD_MODE_CLONE			0x03
#define HEAD_MODE_LVDS_PANEL	0x08

// register MMIO modes
#define OUT 0x1	// direct MMIO calls
#define CRT 0x2	// crt controler calls
#define VGA 0x3 // vga calls
#define PLL 0x4 // PLL calls
#define MC	0x5 // Memory Controler calls

extern accelerant_info *gInfo;
extern register_info *gRegister;


status_t init_registers(uint8 crtid);


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


inline uint32
_read32MC(uint32 offset)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset == RADEON_R600) {
		_write32(RS600_MC_INDEX, ((offset & RS600_MC_INDEX_ADDR_MASK)
			| RS600_MC_INDEX_CITF_ARB0));
		return _read32(RS600_MC_DATA);
	} else if (info.device_chipset == (RADEON_R600 & 0x90)
		|| info.device_chipset == (RADEON_R700 & 0x40)) {
		_write32(RS690_MC_INDEX, (offset & RS690_MC_INDEX_ADDR_MASK));
		return _read32(RS690_MC_DATA);
	} else if (info.device_chipset == (RADEON_R700 & 0x80)
		|| info.device_chipset == (RADEON_R800 & 0x80)) {
		_write32(RS780_MC_INDEX, offset & RS780_MC_INDEX_ADDR_MASK);
		return _read32(RS780_MC_DATA);
	}

	// eh.
	return _read32(offset);
}


inline void
_write32MC(uint32 offset, uint32 data)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset == RADEON_R600) {
		_write32(RS600_MC_INDEX, ((offset & RS600_MC_INDEX_ADDR_MASK)
			| RS600_MC_INDEX_CITF_ARB0 | RS600_MC_INDEX_WR_EN));
		_write32(RS600_MC_DATA, data);
	} else if (info.device_chipset == (RADEON_R600 & 0x90)
		|| info.device_chipset == (RADEON_R700 & 0x40)) {
		_write32(RS690_MC_INDEX, ((offset & RS690_MC_INDEX_ADDR_MASK)
			| RS690_MC_INDEX_WR_EN));
		_write32(RS690_MC_DATA, data);
		_write32(RS690_MC_INDEX, RS690_MC_INDEX_WR_ACK);
	} else if (info.device_chipset == (RADEON_R700 & 0x80)
		|| info.device_chipset == (RADEON_R800 & 0x80)) {
			_write32(RS780_MC_INDEX, ((offset & RS780_MC_INDEX_ADDR_MASK)
				| RS780_MC_INDEX_WR_EN));
			_write32(RS780_MC_DATA, data);
	}
}


inline uint32
_read32PLL(uint16 offset)
{
	_write32(CLOCK_CNTL_INDEX, offset & PLL_ADDR);
	return _read32(CLOCK_CNTL_DATA);
}


inline void
_write32PLL(uint16 offset, uint32 data)
{
	_write32(CLOCK_CNTL_INDEX, (offset & PLL_ADDR) | PLL_WR_EN);
	_write32(CLOCK_CNTL_DATA, data);
}


inline uint32
Read32(uint32 subsystem, uint32 offset)
{
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
			return _read32(offset);
		case CRT:
			return _read32(offset);
		case PLL:
			return _read32PLL(offset);
		case MC:
			return _read32MC(offset);
	};
}


inline void
Write32(uint32 subsystem, uint32 offset, uint32 value)
{
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
			_write32(offset, value);
			return;
		case CRT:
			_write32(offset, value);
			return;
		case PLL:
			_write32PLL(offset, value);
			return;
		case MC:
			_write32MC(offset, value);
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
			temp = _read32(offset);
			break;
		case CRT:
			temp = _read32(offset);
			break;
		case PLL:
			temp = _read32PLL(offset);
			break;
		case MC:
			temp = _read32MC(offset);
			break;
	};

	// only effect mask
	temp &= ~mask;
	temp |= value & mask;

	switch (subsystem) {
		default:
		case OUT:
		case VGA:
			_write32(offset, temp);
			return;
		case CRT:
			_write32(offset, temp);
			return;
		case PLL:
			_write32PLL(offset, temp);
			return;
		case MC:
			_write32MC(offset, temp);
			return;
	};
}


#endif	/* RADEON_HD_ACCELERANT_H */

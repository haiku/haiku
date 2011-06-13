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
	uint16_t	crtid;
	uint16_t	grphEnable;
	uint16_t	grphControl;
	uint16_t	grphSwapControl;
	uint16_t	grphPrimarySurfaceAddr;
	uint16_t	grphPrimarySurfaceAddrHigh;
	uint16_t	grphSecondarySurfaceAddr;
	uint16_t	grphSecondarySurfaceAddrHigh;
	uint16_t	grphPitch;
	uint16_t	grphSurfaceOffsetX;
	uint16_t	grphSurfaceOffsetY;
	uint16_t	grphXStart;
	uint16_t	grphYStart;
	uint16_t	grphXEnd;
	uint16_t	grphYEnd;
	uint16_t	crtCountControl;
	uint16_t	crtInterlace;
	uint16_t	crtHPolarity;
	uint16_t	crtVPolarity;
	uint16_t	crtHSync;
	uint16_t	crtVSync;
	uint16_t	crtHBlank;
	uint16_t	crtVBlank;
	uint16_t	crtHTotal;
	uint16_t	crtVTotal;
	uint16_t	modeDesktopHeight;
	uint16_t	modeDataFormat;
	uint16_t	modeCenter;
	uint16_t	viewportStart;
	uint16_t	viewportSize;
	uint16_t	sclUpdate;
	uint16_t	sclEnable;
	uint16_t	sclTapControl;
};


#define HEAD_MODE_A_ANALOG		0x01
#define HEAD_MODE_B_DIGITAL		0x02
#define HEAD_MODE_CLONE			0x03
#define HEAD_MODE_LVDS_PANEL	0x08

extern accelerant_info *gInfo;
extern register_info *gRegister;


status_t init_registers(uint8 crtid);


// register access

inline uint32
read32(uint32 offset)
{
	return *(volatile uint32 *)(gInfo->regs + offset);
}


inline void
write32(uint32 offset, uint32 value)
{
	*(volatile uint32 *)(gInfo->regs + offset) = value;
}


inline void
write32AtMask(uint32 offset, uint32 value, uint32 mask)
{
	uint32 temp = read32(offset);
	temp &= ~mask;
	temp |= value & mask;
	write32(offset, temp);
}


inline uint32
read32MC(uint32 offset)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset == RADEON_R600) {
		write32(RS600_MC_INDEX, ((offset & RS600_MC_INDEX_ADDR_MASK)
			| RS600_MC_INDEX_CITF_ARB0));
		return read32(RS600_MC_DATA);
	} else if (info.device_chipset == (RADEON_R600 & 0x90)
		|| info.device_chipset == (RADEON_R700 & 0x40)) {
		write32(RS690_MC_INDEX, (offset & RS690_MC_INDEX_ADDR_MASK));
		return read32(RS690_MC_DATA);
	} else if (info.device_chipset == (RADEON_R700 & 0x80)
		|| info.device_chipset == (RADEON_R800 & 0x80)) {
		write32(RS780_MC_INDEX, offset & RS780_MC_INDEX_ADDR_MASK);
		return read32(RS780_MC_DATA);
	}

	// eh.
	return read32(offset);
}


inline void
write32MC(uint32 offset, uint32 data)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset == RADEON_R600) {
		write32(RS600_MC_INDEX, ((offset & RS600_MC_INDEX_ADDR_MASK)
			| RS600_MC_INDEX_CITF_ARB0 | RS600_MC_INDEX_WR_EN));
		write32(RS600_MC_DATA, data);
	} else if (info.device_chipset == (RADEON_R600 & 0x90)
		|| info.device_chipset == (RADEON_R700 & 0x40)) {
		write32(RS690_MC_INDEX, ((offset & RS690_MC_INDEX_ADDR_MASK)
			| RS690_MC_INDEX_WR_EN));
		write32(RS690_MC_DATA, data);
		write32(RS690_MC_INDEX, RS690_MC_INDEX_WR_ACK);
	} else if (info.device_chipset == (RADEON_R700 & 0x80)
		|| info.device_chipset == (RADEON_R800 & 0x80)) {
			write32(RS780_MC_INDEX, ((offset & RS780_MC_INDEX_ADDR_MASK)
				| RS780_MC_INDEX_WR_EN));
			write32(RS780_MC_DATA, data);
	}
}


inline uint32
read32PLL(uint16 offset)
{
	write32(CLOCK_CNTL_INDEX, offset & PLL_ADDR);
	return read32(CLOCK_CNTL_DATA);
}


inline void
write32PLL(uint16 offset, uint32 data)
{
	write32(CLOCK_CNTL_INDEX, (offset & PLL_ADDR) | PLL_WR_EN);
	write32(CLOCK_CNTL_DATA, data);
}


inline void
write32PLLAtMask(uint16 offset, uint32 value, uint32 mask)
{
	uint32 temp = read32PLL(offset);
	temp &= ~mask;
	temp |= value & mask;
	write32PLL(offset, temp);
}


#endif	/* RADEON_HD_ACCELERANT_H */

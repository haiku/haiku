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
write32AtMask(uint32 adress, uint32 value, uint32 mask)
{
	uint32 temp;
	temp = read32(adress);
	temp &= ~mask;
	temp |= value & mask;
	write32(adress, temp);
}


inline uint32_t
ReadMC(int screenIndex, uint32_t addr)
{
	// TODO : readMC for R5XX
	return 0;
}


inline void
WriteMC(int screenIndex, uint32_t addr, uint32_t data)
{
	// TODO : writeMC for R5XX
}


inline uint32_t
ReadPLL(int screenIndex, uint16_t offset)
{
	write32(CLOCK_CNTL_INDEX, offset & PLL_ADDR);
	return read32(CLOCK_CNTL_DATA);
}


inline void
WritePLL(int screenIndex, uint16_t offset, uint32_t data)
{
	write32(CLOCK_CNTL_INDEX, (offset & PLL_ADDR) | PLL_WR_EN);
	write32(CLOCK_CNTL_DATA, data);
}


#endif	/* RADEON_HD_ACCELERANT_H */

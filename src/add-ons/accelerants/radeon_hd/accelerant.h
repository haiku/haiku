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


#include <ByteOrder.h>
#include <edid.h>

#include "atom.h"
#include "dp_raw.h"
#include "encoder.h"
#include "mode.h"
#include "pll.h"
#include "radeon_hd.h"


#define MAX_DISPLAY 2
	// Maximum displays (more then two requires AtomBIOS)


struct gpu_state {
	uint32 d1vgaControl;
	uint32 d2vgaControl;
	uint32 vgaRenderControl;
	uint32 vgaHdpControl;
	uint32 d1crtcControl;
	uint32 d2crtcControl;
};


struct fb_info {
	bool		valid;
	uint64		vramStart;
	uint64		vramEnd;
	uint64		vramSize;

	uint64		gartStart;
	uint64		gartEnd;
	uint64		gartSize;
	uint64		agpBase;
};


struct accelerant_info {
	vuint8*			regs;
	area_id			regs_area;

	radeon_shared_info* shared_info;
	area_id			shared_info_area;

	display_mode*	mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	uint8*			rom;
	area_id			rom_area;

	edid1_info		edid_info;
	bool			has_edid;

	int				device;
	bool			is_clone;

	struct fb_info	fb;	// used for frame buffer info within MC

	volatile uint32	dpms_mode;		// current driver dpms mode
};


struct register_info {
	uint16	crtcOffset;
	uint16	vgaControl;
	uint16	grphEnable;
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
	uint16	modeDesktopHeight;
	uint16	modeDataFormat;
	uint16	viewportStart;
	uint16	viewportSize;
};


typedef struct {
	bool	valid;

	uint32	hwPin;		// GPIO hardware pin on GPU
	bool	hwCapable;	// can do hw assisted i2c

	uint32	sclMaskReg;
	uint32	sdaMaskReg;
	uint32	sclMask;
	uint32	sdaMask;

	uint32	sclEnReg;
	uint32	sdaEnReg;
	uint32	sclEnMask;
	uint32	sdaEnMask;

	uint32	sclYReg;
	uint32	sdaYReg;
	uint32	sclYMask;
	uint32	sdaYMask;

	uint32	sclAReg;
	uint32	sdaAReg;
	uint32	sclAMask;
	uint32	sdaAMask;
} gpio_info;


typedef struct {
	bool	valid;
	uint32	connectorIndex;

	uint32	auxPin; // normally GPIO pin on GPU

	uint8	config[8]; // DP configuration data
	uint8	sinkType;
	uint8	clock;
	int		laneCount;

	bool	trainingUseEncoder;

	uint8	trainingAttempts;
	uint8	trainingSet[4];
	int		trainingReadInterval;
	uint8 	linkStatus[DP_LINK_STATUS_SIZE];

	bool	eDPOn;
} dp_info;


struct encoder_info {
	bool		valid;
	uint16		objectID;
	uint32		type;
	uint32		flags;
	uint32		linkEnumeration; // ex. linkb == GRAPH_OBJECT_ENUM_ID2
	bool		isExternal;
	bool		isDPBridge;
	struct pll_info	pll;
};


typedef struct {
	bool		valid;
	uint16		objectID;
	uint32		type;
	uint32		flags;
	uint32		lvdsFlags;
	uint16		gpioID;
	struct encoder_info encoder;
	struct encoder_info encoderExternal;
	// TODO struct radeon_hpd hpd;
} connector_info;


typedef struct {
	bool			attached;
	bool			powered;
	uint32			connectorIndex; // matches connector id in connector_info
	register_info*	regs;
	bool			found_ranges;
	uint32			vfreq_max;
	uint32			vfreq_min;
	uint32			hfreq_max;
	uint32			hfreq_min;
	edid1_info		edid_info;
	display_mode	preferredMode;
} display_info;


// register MMIO modes
#define OUT 0x1	// Direct MMIO calls
#define CRT 0x2	// Crt controller calls
#define VGA 0x3 // Vga calls
#define PLL 0x4 // PLL calls
#define MC	0x5 // Memory controller calls


extern accelerant_info* gInfo;
extern atom_context* gAtomContext;
extern display_info* gDisplay[MAX_DISPLAY];
extern connector_info* gConnector[ATOM_MAX_SUPPORTED_DEVICE];
extern gpio_info* gGPIOInfo[ATOM_MAX_SUPPORTED_DEVICE];
extern dp_info* gDPInfo[ATOM_MAX_SUPPORTED_DEVICE];


// register access

inline uint32
_read32(uint32 offset)
{
	return *(volatile uint32*)(gInfo->regs + offset);
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

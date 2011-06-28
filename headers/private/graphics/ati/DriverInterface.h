/*
	Copyright 2007-2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H


#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <Drivers.h>
#include <edid.h>
#include <video_overlay.h>


// This file contains info that is shared between the kernel driver and the
// accelerant, and info that is shared among the source files of the accelerant.


#define ENABLE_DEBUG_TRACE		// if defined, turns on debug output to syslog


#define ARRAY_SIZE(a) (int(sizeof(a) / sizeof(a[0]))) 	// get number of elements in an array

struct Benaphore {
	sem_id	sem;
	int32	count;

	status_t Init(const char* name)
	{
		count = 0;
		sem = create_sem(0, name);
		return sem < 0 ? sem : B_OK;
	}

	status_t Acquire()
	{
		if (atomic_add(&count, 1) > 0)
			return acquire_sem(sem);
		return B_OK;
	}

	status_t Release()
	{
		if (atomic_add(&count, -1) > 1)
			return release_sem(sem);
		return B_OK;
	}

	void Delete()	{ delete_sem(sem); }
};


enum {
	ATI_GET_SHARED_DATA = B_DEVICE_OP_CODES_END + 123,
	ATI_DEVICE_NAME,
	ATI_GET_EDID,
	ATI_RUN_INTERRUPTS,
	ATI_SET_VESA_DISPLAY_MODE
};


// Chip type numbers.  These are used to group the chips into related
// groups.	See table chipTable in driver.c
// Note that the order of the Mach64 chip types must not be changed because
// < or > comparisons of the chip types are made.  They should be in the order
// of the evolution of the chips.

enum ChipType {
	ATI_NONE = 0,

	MACH64_264VT,
	MACH64_264GT,
	MACH64_264VTB,
	MACH64_264GTB,
	MACH64_264VT3,
	MACH64_264GTDVD,
	MACH64_264LT,
	MACH64_264VT4,
	MACH64_264GT2C,
	MACH64_264GTPRO,
	MACH64_264LTPRO,
	MACH64_264XL,
	MACH64_MOBILITY,
		Mach64_ChipsEnd,		// marks end of Mach64's
	RAGE128_GL,
	RAGE128_MOBILITY,
	RAGE128_PRO_GL,
	RAGE128_PRO_VR,
	RAGE128_PRO_ULTRA,
	RAGE128_VR,
};


#define MACH64_FAMILY(chipType)		(chipType < Mach64_ChipsEnd)
#define RAGE128_FAMILY(chipType)	(chipType > Mach64_ChipsEnd)



enum MonitorType {
	MT_VGA,			// monitor with analog VGA interface
	MT_DVI,			// monitor with DVI interface
	MT_LAPTOP		// laptop video display
};


// Mach64 parameters for computing register vaules and other parameters.

struct M64_Params {
	// Clock parameters
	uint8	clockNumberToProgram;	// obtained from video BIOS
	uint32	maxPixelClock;			// obtained from video BIOS
	int		refFreq;				// obtained from video BIOS
	int		refDivider;				// obtained from video BIOS
	uint8	xClkPostDivider;
	uint8	xClkRefDivider;
	uint16	xClkPageFaultDelay;
	uint16	xClkMaxRASDelay;
	uint16	displayFIFODepth;
	uint16	displayLoopLatency;
	uint8	vClkPostDivider;
	uint8	vClkFeedbackDivider;
};


struct R128_PLLParams {
	uint16	reference_freq;
	uint16	reference_div;
	uint32	min_pll_freq;
	uint32	max_pll_freq;
	uint16	xclk;
};


struct R128_RAMSpec {		// All values in XCLKS
	int  memReadLatency;	// Memory Read Latency
	int  memBurstLen;		// Memory Burst Length
	int  rasToCasDelay;		// RAS to CAS delay
	int  rasPercentage;		// RAS percentage
	int  writeRecovery;		// Write Recovery
	int  casLatency;		// CAS Latency
	int  readToWriteDelay;	// Read to Write Delay
	int  loopLatency;		// Loop Latency
	int  loopFudgeFactor;	// Add to memReadLatency to get loopLatency
	const char *name;
};


struct VesaMode {
	uint16			mode;		// VESA mode number
	uint16			width;
	uint16			height;
	uint8			bitsPerPixel;
};


struct DisplayModeEx : display_mode {
	uint8	bitsPerPixel;
	uint16	bytesPerRow;		// number of bytes in one line/row
};


struct OverlayBuffer : overlay_buffer {
	OverlayBuffer*	nextBuffer;	// pointer to next buffer in chain, NULL = none
	uint32			size;		// size of overlay buffer
};


struct SharedInfo {
	// Device ID info.
	uint16	vendorID;			// PCI vendor ID, from pci_info
	uint16	deviceID;			// PCI device ID, from pci_info
	uint8	revision;			// PCI device revsion, from pci_info
	ChipType chipType;			// indicates group in which chip belongs (a group has similar functionality)
	char	chipName[32];		// user recognizable name of chip

	bool	bAccelerantInUse;	// true = accelerant has been initialized
	bool	bInterruptAssigned;	// card has a useable interrupt assigned to it

	sem_id	vertBlankSem;		// vertical blank semaphore; if < 0, there is no semaphore

	// Memory mappings.
	area_id regsArea;			// area_id for the memory mapped registers. It will
								// be cloned into accelerant's address space.
	area_id videoMemArea;		// video memory area_id.  The addresses are shared with all teams.
	addr_t	videoMemAddr;		// video memory addr as viewed from virtual memory
	phys_addr_t	videoMemPCI;	// video memory addr as viewed from the PCI bus (for DMA)
	uint32	videoMemSize; 		// video memory size in bytes.

	uint32	cursorOffset;		// offset of cursor in video memory
	uint32	frameBufferOffset;	// offset of frame buffer in video memory
	uint32	maxFrameBufferSize;	// max available video memory for frame buffer

	// Color spaces supported by current video chip/driver.
	color_space	colorSpaces[6];
	uint32	colorSpaceCount;	// number of color spaces in array colorSpaces

	// List of screen modes.
	area_id modeArea;			// area containing list of display modes the driver supports
	uint32	modeCount;			// number of display modes in the list

	DisplayModeEx displayMode;	// current display mode configuration

	// List of VESA modes supported by current chip.
	uint32		vesaModeTableOffset;	// offset of table in shared info
	uint32		vesaModeCount;

	uint16		cursorHotX;		// Cursor hot spot. Top left corner of the cursor
	uint16		cursorHotY;		// is 0,0

	edid1_info	edidInfo;
	bool		bHaveEDID;		// true = EDID info from device is in edidInfo

	Benaphore	engineLock;		// for serializing access to the acceleration engine
	Benaphore	overlayLock;	// for overlay operations

	int32		overlayAllocated;	// non-zero if overlay is allocated
	uint32		overlayToken;
	OverlayBuffer* overlayBuffer;	// pointer to linked list of buffers; NULL = none

	MonitorType	displayType;

	uint16		panelX;			// laptop LCD width
	uint16		panelY;			// laptop LCD height
	uint16		panelPowerDelay;

	// Data members for Mach64 chips.
	//-------------------------------

	M64_Params	m64Params;			// parameters for Mach64 chips

	// Data members for Rage128 chips.
	//--------------------------------

	R128_RAMSpec	r128MemSpec;	// Rage128 memory timing spec's
	R128_PLLParams	r128PLLParams;	// Rage128 PLL parameters from video BIOS ROM

	uint32		r128_dpGuiMasterCntl;	// flags for accelerated drawing
};


#endif	// DRIVERINTERFACE_H

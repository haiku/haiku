/*
	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Other authors:
	Gerald Zajac 2007-2008
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H


#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <Drivers.h>
#include <edid.h>


// This is the info that needs to be shared between the kernel driver and
// the accelerant for the sample driver.

#if defined(__cplusplus)
extern "C" {
#endif

#define ENABLE_DEBUG_TRACE		// if defined, turns on debug output to syslog


#define NUM_ELEMENTS(a) ((int)(sizeof(a) / sizeof(a[0]))) 	// for computing number of elements in an array

struct benaphore {
	sem_id	sem;
	int32	ben;
};

#define INIT_BEN(x) 	x.sem = create_sem(0, "S3 "#x" benaphore");	x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define DELETE_BEN(x)	delete_sem(x.sem);


#define S3_PRIVATE_DATA_MAGIC	 0x4521 // a private driver rev, of sorts


enum {
	S3_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	S3_DEVICE_NAME,
	S3_GET_PIO,
	S3_SET_PIO,
	S3_RUN_INTERRUPTS,
};


// Chip type numbers.  These are used to group the chips into related
// groups.	See table S3_ChipTable in driver.c

enum S3_ChipType {
	S3_TRIO64 = 1,
	S3_TRIO64_VP,		// Trio64V+ has same ID as Trio64 but different revision number
	S3_TRIO64_UVP,
	S3_TRIO64_V2,
		Trio64ChipsEnd,
	S3_VIRGE,
	S3_VIRGE_VX,
	S3_VIRGE_DXGX,
	S3_VIRGE_GX2,
	S3_VIRGE_MX,
	S3_VIRGE_MXP,
	S3_TRIO_3D,
	S3_TRIO_3D_2X,
		VirgeChipsEnd,
	S3_SAVAGE_3D,
	S3_SAVAGE_MX,
	S3_SAVAGE4,
	S3_PROSAVAGE,
	S3_TWISTER,
	S3_PROSAVAGE_DDR,
	S3_SUPERSAVAGE,
	S3_SAVAGE2000,
};


#define S3_TRIO64_FAMILY(chip)	(chip < Trio64ChipsEnd)
#define S3_VIRGE_FAMILY(chip)	(chip > Trio64ChipsEnd && chip < VirgeChipsEnd)
#define S3_SAVAGE_FAMILY(chip)	(chip > VirgeChipsEnd)

#define S3_VIRGE_GX2_SERIES(chip)	(chip == S3_VIRGE_GX2 || chip == S3_TRIO_3D_2X)
#define S3_VIRGE_MX_SERIES(chip)	(chip == S3_VIRGE_MX || chip == S3_VIRGE_MXP)

#define S3_SAVAGE_3D_SERIES(chip)	((chip == S3_SAVAGE_3D) || (chip == S3_SAVAGE_MX))
#define S3_SAVAGE4_SERIES(chip)		((chip == S3_SAVAGE4)		\
									|| (chip == S3_PROSAVAGE)	\
									|| (chip == S3_TWISTER)		\
									|| (chip == S3_PROSAVAGE_DDR))
#define	S3_SAVAGE_MOBILE_SERIES(chip)	((chip == S3_SAVAGE_MX)	\
										|| (chip == S3_SUPERSAVAGE))
#define S3_MOBILE_TWISTER_SERIES(chip)	((chip == S3_TWISTER)	\
										|| (chip == S3_PROSAVAGE_DDR))



enum MonitorType {
	MT_CRT,
	MT_LCD,			// laptop LCD display
	MT_DFP			// DVI display
};


// Bitmap descriptor structures for BCI (for Savage chips)
struct HIGH {
	unsigned short Stride;
	unsigned char Bpp;
	unsigned char ResBWTile;
};

struct BMPDESC1 {
	unsigned long Offset;
	HIGH  HighPart;
};

struct BMPDESC2 {
	unsigned long LoPart;
	unsigned long HiPart;
};

union BMPDESC {
	BMPDESC1 bd1;
	BMPDESC2 bd2;
};



struct DisplayModeEx : display_mode {
	uint32	bpp;			// bits/pixel
	uint32	bytesPerRow;	// number of bytes in one line/row
};


struct SharedInfo {
	// Device ID info.
	uint16	vendorID;			// PCI vendor ID, from pci_info
	uint16	deviceID;			// PCI device ID, from pci_info
	uint8	revision;			// PCI device revsion, from pci_info
	uint32	chipType;			// indicates group in which chip belongs (a group has similar functionality)
	char	chipName[32];		// user recognizable name of chip

	bool	bAccelerantInUse;	// true = accelerant has been initialized
	bool	bInterruptAssigned;	// card has a useable interrupt assigned to it

	sem_id	vertBlankSem;		// vertical blank semaphore; if < 0, there is no semaphore

	// Memory mappings.
	area_id regsArea;			// area_id for the memory mapped registers. It will
								// be cloned into accelerant's address space.
	area_id videoMemArea;		// video memory area_id.  The addresses are shared with all teams.
	void*	videoMemAddr;		// video memory addr as viewed from virtual memory
	void*	videoMemPCI;		// video memory addr as viewed from the PCI bus (for DMA)
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

	// Cursor info.
	struct {
		uint16	hot_x;			// Cursor hot spot. Top left corner of the cursor
		uint16	hot_y;			// is 0,0
	} cursor;

	// Current display mode configuration, and other parameters related to
	// current display mode.
	DisplayModeEx displayMode;	// current display mode configuration
	int32	commonCmd;			// flags common to drawing commands of current display mode

	edid1_info	edidInfo;
	bool		bHaveEDID;		// true = EDID info from device is in edidInfo

	// Acceleration engine.
	struct {
		uint64		count;		// last fifo slot used
		uint64		lastIdle;	// last fifo slot we *know* the engine was idle after
		benaphore	lock;	 	// for serializing access to the acceleration engine
	} engine;

	int		mclk;

	MonitorType	displayType;

	uint16	panelX;				// laptop LCD width
	uint16	panelY;				// laptop LCD height

	// Command Overflow Buffer (COB) parameters for Savage chips.
	bool	bDisableCOB;		// enable/disable COB for Savage 4 & ProSavage
	uint32	cobIndex;			// size index
	uint32	cobSize;			// size in bytes
	uint32	cobOffset;			// offset in video memory
	uint32	bciThresholdLo; 	// low and high thresholds for
	uint32  bciThresholdHi; 	// shadow status update (32bit words)

	BMPDESC GlobalBD;			// Bitmap Descriptor for BCI
};


// Set some boolean condition (like enabling or disabling interrupts)
struct S3SetBoolState {
	uint32	magic;		// magic number to make sure the caller groks us
	bool	bEnable;	// state to set
};


// Retrieve the area_id of the kernel/accelerant shared info
struct S3GetPrivateData {
	uint32	magic;		// magic number to make sure the caller groks us
	area_id sharedInfoArea;	// ID of area containing shared information
};


struct S3GetSetPIO {
	uint32	  magic;	// magic number to make sure the caller groks us
	uint32	  offset;	// offset of PIO register to read/write
	uint32	  size;		// number of bytes to transfer
	uint32	  value;	// value to write or value that was read
};


#if defined(__cplusplus)
}
#endif

#endif	// DRIVERINTERFACE_H

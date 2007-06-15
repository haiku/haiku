/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the sample driver.
*/
#if defined(__cplusplus)
extern "C" {
#endif


#define NUM_ELEMENTS(a) ((int)(sizeof(a) / sizeof(a[0]))) 	// for computing number of elements in an array

typedef struct
{
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x) 	x.sem = create_sem(0, "SAVAGE "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define DELETE_BEN(x)	delete_sem(x.sem);


#define SAVAGE_PRIVATE_DATA_MAGIC	 0x5791 // a private driver rev, of sorts


enum
{
	SAVAGE_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	SAVAGE_GET_PCI,
	SAVAGE_SET_PCI,
	SAVAGE_DEVICE_NAME,
	SAVAGE_RUN_INTERRUPTS,
};


// Chip tags.  These are used to group the adapters into related
// families.  See table SavageChipsetTable in driver.c

enum S3ChipTags
{
	S3_UNKNOWN = 0,
	S3_SAVAGE3D,
	S3_SAVAGE_MX,
	S3_SAVAGE4,
	S3_PROSAVAGE,
	S3_TWISTER,
	S3_PROSAVAGEDDR,
	S3_SUPERSAVAGE,
	S3_SAVAGE2000,
};

#define S3_SAVAGE3D_SERIES(chip)	((chip==S3_SAVAGE3D) || (chip==S3_SAVAGE_MX))

#define S3_SAVAGE4_SERIES(chip)		((chip==S3_SAVAGE4)		\
									|| (chip==S3_PROSAVAGE)		\
									|| (chip==S3_TWISTER)		\
									|| (chip==S3_PROSAVAGEDDR))

#define	S3_SAVAGE_MOBILE_SERIES(chip)	((chip==S3_SAVAGE_MX) || (chip==S3_SUPERSAVAGE))

#define S3_MOBILE_TWISTER_SERIES(chip)	((chip==S3_TWISTER) || (chip==S3_PROSAVAGEDDR))


typedef enum {
	MT_NONE,
	MT_CRT,
	MT_LCD,
	MT_DFP,
	MT_TV
} SavageMonitorType;


// Bitmap descriptor structures for BCI
typedef struct _HIGH {
	unsigned short Stride;
	unsigned char Bpp;
	unsigned char ResBWTile;
} HIGH;

typedef struct _BMPDESC1 {
	unsigned long Offset;
	HIGH  HighPart;
} BMPDESC1;

typedef struct _BMPDESC2 {
	unsigned long LoPart;
	unsigned long HiPart;
} BMPDESC2;

typedef union _BMPDESC {
	BMPDESC1 bd1;
	BMPDESC2 bd2;
} BMPDESC;



typedef struct
{
	// Device ID info.
	uint16	vendorID;			// PCI vendor ID, from pci_info
	uint16	deviceID;			// PCI device ID, from pci_info
	uint8	revision;			// PCI device revsion, from pci_info
	uint32	chipset;			// indicates family in which chipset belongs (a family has similar functionality)
	char	chipsetName[32];	// user recognizable name of chipset

	bool	bAccelerantInUse;	// true = accelerant has been initialized
	bool	bInterruptAssigned;	// card has a useable interrupt assigned to it

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

	// List of screen modes.
	area_id modeArea;			// Contains the list of display modes the driver supports
	uint32	modeCount;			// Number of display modes in the list

	// Vertical blank semaphore.
	sem_id	vblank;				// vertical blank semaphore; if < 0, there is no semaphore
								// Ownership will be transfered to team opening device first
	// Flags used by driver.
	int32	flags;

	// Cursor info.
	struct
	{
		uint16	hot_x;			// Cursor hot spot. The top left corner of the cursor
		uint16	hot_y;			// is 0,0
		uint16	x;				// The location of the cursor hot spot on the
		uint16	y;				// display (or desktop?)
		uint16	width;			// Width and height of the cursor shape
		uint16	height;
		bool	bIsVisible;		// Is the cursor currently displayed?
	} cursor;

	display_mode dm;			// current display mode configuration
	int			 bitsPerPixel;	// bits per pixel of current display mode

	frame_buffer_config fbc;	// frame buffer addresses and bytes_per_row

	// Acceleration engine.
	struct
	{
		uint64		count;		// last fifo slot used
		uint64		lastIdle;	// last fifo slot we *know* the engine was idle after
		benaphore	lock;	 	// for serializing access to the acceleration engine
	} engine;

	int		mclk;
	uint32	pix_clk_max8;		// The maximum speed the pixel clock should run
	uint32	pix_clk_max16;		// at for a given pixel width.  Usually a function
	uint32	pix_clk_max32;		// of memory and DAC bandwidths.

	// Command Overflow Buffer (COB) parameters.
	bool	bDisableCOB;		// enable/disable COB for Savage 4 & ProSavage
	uint32	cobIndex;			// size index
	uint32	cobSize;			// size in bytes
	uint32	cobOffset;			// offset in video memory
	uint32	bciThresholdLo; 	// low and high thresholds for
	uint32  bciThresholdHi; 	// shadow status update (32bit words)

	BMPDESC GlobalBD;			// Bitmap Descriptor for BCI

	int		panelX;				// LCD panel width
	int		panelY;				// LCD panel height

	int		frameX0;			// viewport position
	int		frameY0;

	// The various Savage wait handlers.
	bool	(*WaitQueue)(int);
	bool	(*WaitIdleEmpty)();

	SavageMonitorType	displayType;

} SharedInfo;


// Read or write a value in PCI configuration space
typedef struct
{
	uint32	  magic;	// magic number to make sure the caller groks us
	uint32	  offset;	// Offset to read/write
	uint32	  size;		// Number of bytes to transfer
	uint32	  value;	// The value read or written
} SavageGetSetPci;


// Set some boolean condition (like enabling or disabling interrupts)
typedef struct
{
	uint32	magic;		// magic number to make sure the caller groks us
	bool	bEnable;	// state to set
} SavageSetBoolState;


// Retrieve the area_id of the kernel/accelerant shared info
typedef struct
{
	uint32	magic;		// magic number to make sure the caller groks us
	area_id sharedInfoArea;	// ID of area containing shared information
} SavageGetPrivateData;


// Retrieve the device name.  Usefull for when we have a file handle, but want
// to know the device name (like when we are cloning the accelerant)
typedef struct
{
	uint32	magic;		// magic number to make sure the caller groks us
	char	*name;		// The name of the device, less the /dev root
} SavageDeviceName;


#if defined(__cplusplus)
}
#endif


#endif

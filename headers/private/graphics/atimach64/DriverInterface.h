
#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

/* ATI Mach64 Definitions */
#include "regmach64.h"


/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the atimach64 driver.
*/
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "ATIMACH64 "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);

#define inb(a)        *((vuint8 *)((uint32)(si->regs) + (a)))
#define outb(a, b)     *((vuint8 *)((uint32)(si->regs) + (a))) = (b)
#define inh(a)        *((vuint16 *)((uint32)(si->regs) + (a)))
#define outh(a, b)     *((vuint16 *)((uint32)(si->regs) + (a))) = (b)
#define inw(a)        *((vuint32 *)((uint32)(si->regs) + (a)))
#define outw(a, b)     *((vuint32 *)((uint32)(si->regs) + (a))) = (b)
#define rom(a)         *((vuchar *)((uint32)(si->regs) + (a)))
#define romsi(a)         *((vuint16 *)((uint32)(si->regs) + (a)))
#define rommem(a)         ((char *)((uint32)(si->regs) + (a)))


#define ATIMACH64_PRIVATE_DATA_MAGIC	0x1234 /* a private driver rev, of sorts */

#define MAX_ATIMACH64_DEVICE_NAME_LENGTH 32

#define SKD_MOVE_CURSOR    0x00000001
#define SKD_PROGRAM_CLUT   0x00000002
#define SKD_SET_START_ADDR 0x00000004
#define SKD_SET_CURSOR     0x00000008
#define SKD_HANDLER_INSTALLED 0x80000000

enum {
	ATIMACH64_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	ATIMACH64_GET_PCI,
	ATIMACH64_SET_PCI,
	ATIMACH64_DEVICE_NAME,
	ATIMACH64_RUN_INTERRUPTS
};

typedef struct {
	uint16	vendor_id;	/* PCI vendor ID, from pci_info */
	uint16	device_id;	/* PCI device ID, from pci_info */
	uint8	revision;	/* PCI device revsion, from pci_info */
   void *regs;       /* Registers offset within buffer */
	area_id	regs_area;	/* Kernel's area_id for the memory mapped registers.
							It will be cloned into the accelerant's	address
							space. */
	area_id	fb_area;	/* Frame buffer's area_id.  The addresses are shared
							with all teams. */
	void	*framebuffer;	/* As viewed from virtual memory */
	void	*framebuffer_pci;	/* As viewed from the PCI bus (for DMA) */
	area_id	rom_area;	/* Mapped ROM's area_id */
	void	*rom;	/* As viewed from virtual memory.  Shared by all teams */
	area_id	mode_area;	/* Contains the list of display modes the driver supports */
	uint32	mode_count;	/* Number of display modes in the list */
	sem_id	vblank;	/* The vertical blank semaphore.  Ownership will be
						transfered to the team opening the device first */
	int32	flags;	
	int32	start_addr;

	struct {
		uint8*	data;		/*  Pointer into the frame buffer to where the
								cursor data starts */
		uint16	hot_x;		/* Cursor hot spot. The top left corner of the cursor */
		uint16	hot_y;		/* is 0,0 */
		uint16	x;			/* The location of the cursor hot spot on the */
		uint16	y;			/* display (or desktop?) */
		uint16	width;		/* Width and height of the cursor shape */
		uint16	height;
		bool	is_visible;	/* Is the cursor currently displayed? */
	}		cursor;
	uint16	first_color;
	uint16	color_count;
	bigtime_t	refresh_period;	/* Duration of one frame (ie 1/refresh rate) */
	bigtime_t	blank_period;	/* Duration of the blanking period.   These are
									usefull when faking vertical blanking
									interrupts. */
	uint8	color_data[3 * 256];	/* */
	uint8	cursor0[64*64/8];	/* AND mask for a 64x64 cursor */
	uint8	cursor1[512];		/* XOR mask for a 64x64 cursor */
	display_mode
			dm;		/* current display mode configuration */
	frame_buffer_config
			fbc;	/* bytes_per_row and start of frame buffer */
	struct {
		uint64		count;		/* last fifo slot used */
		uint64		last_idle;	/* last fifo slot we *know* the engine was idle after */ 
		benaphore	lock;		/* for serializing access to the acceleration engine */
	        uint64          fifo_limit;
	}		engine;

	uint32	pix_clk_max8;		/* The maximum speed the pixel clock should run */
	uint32	pix_clk_max16;		/*  at for a given pixel width.  Usually a function */
	uint32	pix_clk_max32;		/*  of memory and DAC bandwidths. */
	uint32	mem_size;			/* Frame buffer memory, in bytes. */
   /* Specific info for the Mach64 follows */
   uint8    board_identifier[2];
   uint8    equipment_flags[2];
   uint8    asic_identifier;
   uint8    bios_major;
   uint8    bios_minor;
   char     bios_date[20];
   uint8    VGA_Wonder_Present;
   uint8    Mach64_Present;
   uint8    Bus_Type;
   uint8    Mem_Type;
   uint8    Mem_Size;         /* The result of querying AtiMach64 */
   uint8    DAC_Type;
   uint8    DAC_SubType;
   uint8    Clock_Type;
   uint16   MinFreq;
   uint16   MaxFreq;
   uint16   RefFreq;
   uint16   RefDivider;
   uint16   NAdj;
   uint16   DRAMMemClk;
   uint16   VRAMMemClk;
   uint16   MemClk;
   uint16   CXClk;
   uint16   Clocks[MACH64_NUM_CLOCKS];
   uint8    MemCycle;
   mach64FreqRec    Freq_Table[MACH64_NUM_FREQS];
   mach64FreqRec    Freq_Table2[MACH64_NUM_FREQS];

} shared_info;

/* Read or write a value in PCI configuration space */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint32	offset;		/* Offset to read/write */
	uint32	size;		/* Number of bytes to transfer */
	uint32	value;		/* The value read or written */
} atimach64_get_set_pci;

/* Set some boolean condition (like enabling or disabling interrupts) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;		/* state to set */
} atimach64_set_bool_state;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	area_id	shared_info_area;	/* area_id containing the shared information */
} atimach64_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;		/* The name of the device, less the /dev root */
} atimach64_device_name;

enum {
	ATIMACH64_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif


#endif


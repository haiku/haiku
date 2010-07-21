/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Apsed;
	Rudolf Cornelissen 10/2002-4/2006.
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
#include "video_overlay.h"
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

#define DRIVER_PREFIX "matrox" // apsed
#define DEVICE_FORMAT "%04x_%04x_%02x%02x%02x" // apsed

/*
	Internal driver state (also for sharing info between driver and accelerant)
*/
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "G400 "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);


#define GX00_PRIVATE_DATA_MAGIC	0x0009 /* a private driver rev, of sorts */

/*dualhead extensions to flags*/
#define DUALHEAD_OFF (0<<6)
#define DUALHEAD_CLONE (1<<6)
#define DUALHEAD_ON (2<<6)
#define DUALHEAD_SWITCH (3<<6)
#define DUALHEAD_BITS (3<<6)
#define DUALHEAD_CAPABLE (1<<8)
#define TV_BITS (3<<9)
#define TV_MON (0<<9
#define TV_PAL (1<<9)
#define TV_NTSC (2<<9)
#define TV_CAPABLE (1<<11)
#define TV_VIDEO (1<<12)
#define TV_PRIMARY (1<<13)

#define SKD_MOVE_CURSOR    0x00000001
#define SKD_PROGRAM_CLUT   0x00000002
#define SKD_SET_START_ADDR 0x00000004
#define SKD_SET_CURSOR     0x00000008
#define SKD_HANDLER_INSTALLED 0x80000000

enum {
	GX00_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	GX00_GET_PCI,
	GX00_SET_PCI,
	GX00_DEVICE_NAME,
	GX00_RUN_INTERRUPTS
};

/* max. number of overlay buffers */
#define MAXBUFFERS 3
/* max. pixelclock speed the BES supports */
#define BESMAXSPEED 135000

/* internal used info on overlay buffers */
typedef	struct
{
	uint16 slopspace;
	uint32 size;
} int_buf_info;

typedef struct settings {  // apsed, see comments in mga.settings
	// for driver
	char   accelerant[B_FILE_NAME_LENGTH];
	char   primary[B_FILE_NAME_LENGTH];
	bool   dumprom;
	// for accelerant
	uint32 logmask;
	uint32 memory;
	bool   usebios;
	bool   hardcursor;
	bool   greensync;
} settings;

/* shared info */
typedef struct {
  /* a few ID things */
	uint16	vendor_id;	/* PCI vendor ID, from pci_info */
	uint16	device_id;	/* PCI device ID, from pci_info */
	uint8	revision;	/* PCI device revsion, from pci_info */
	uint8	bus;		/* PCI bus number, from pci_info */
	uint8	device;		/* PCI device number on bus, from pci_info */
	uint8	function;	/* PCI function number in device, from pci_info */

  /* used to return status for INIT_ACCELERANT and CLONE_ACCELERANT */
	bool	accelerant_in_use;

  /* bug workaround for 4.5.0 */
	uint32 use_clone_bugfix;	/*for 4.5.0, cloning of physical memory does not work*/
	uint32 * clone_bugfix_regs;

  /*memory mappings*/
	area_id	regs_area;	/* Kernel's area_id for the memory mapped registers.
							It will be cloned into the accelerant's	address
							space. */

	area_id	fb_area;	/* Frame buffer's area_id.  The addresses are shared with all teams. */
	area_id pseudo_dma_area;	/* Pseudo dma area_id. Shared by all teams. */
	area_id	dma_buffer_area;	/* Area assigned for dma*/

	void	*framebuffer;		/* As viewed from virtual memory */
	void	*framebuffer_pci;	/* As viewed from the PCI bus (for DMA) */

	void	*pseudo_dma;		/* As viewed from virtual memory */

	void	*dma_buffer;		/* buffer for dma*/
	void	*dma_buffer_pci;	/* buffer for dma - from PCI bus*/

  /*screenmode list*/
	area_id	mode_area;              /* Contains the list of display modes the driver supports */
	uint32	mode_count;             /* Number of display modes in the list */

  /*flags - used by driver*/
	uint32 flags;

  /*vblank semaphore*/
	sem_id	vblank;	                /* The vertical blank semaphore. Ownership will be
						transfered to the team opening the device first */
  /*cursor information*/
	struct {
		uint16	hot_x;		/* Cursor hot spot. The top left corner of the cursor */
		uint16	hot_y;		/* is 0,0 */
		uint16	x;		/* The location of the cursor hot spot on the */
		uint16	y;		/* desktop */
		uint16	width;		/* Width and height of the cursor shape (always 16!) */
		uint16	height;
		bool	is_visible;	/* Is the cursor currently displayed? */
	} cursor;

  /*colour lookup table*/
	uint8	color_data[3 * 256];	/* Colour lookup table - as used by DAC */

  /*more display mode stuff*/
	display_mode dm;		/* current display mode configuration: head1 */
	display_mode dm2;		/* current display mode configuration: head2 */
	uint32 dpms_flags;		/* current DPMS mode */
	bool switched_crtcs;	/* dualhead stretch and switch mode info */
	bool acc_mode;			/* signals (non)accelerated mode */
	bool interlaced_tv_mode;/* signals interlaced CRTC TV output mode */

  /*frame buffer config - for BDirectScreen*/
	frame_buffer_config fbc;	/* bytes_per_row and start of frame buffer: head1 */
	frame_buffer_config fbc2;	/* bytes_per_row and start of frame buffer: head2 */
	accelerant_device_info adi;	/* as returned by hook GET_ACCELERANT_DEVICE_INFO */

  /*acceleration engine*/
	struct {
		uint32		count;		/* last dwgsync slot used */
		uint32		last_idle;	/* last dwgsync slot we *know* the engine was idle after */ 
		benaphore	lock;		/* for serializing access to the acceleration engine */
		uint32 src_dst;			/* G100 pre SRCORG/DSTORG registers */
		uint8 y_lin;			/* MIL1/2 adress linearisation does not always work */
		uint8 depth;
	} engine;

  /* card info - information gathered from PINS (and other sources) */
	enum
	{	// card_type in order of date of MGA chip design
		MIL1 = 0,
		MYST,
		MIL2,
		G100,
		G200,
		G400,
		G400MAX,
		G450,
		G550
	} card_type;
	struct
	{
		/* specialised registers for card initialisation read from MGA BIOS (pins) */

		/* general card information */
		uint32 card_type;           /* see card_type enum above */
		bool int_assigned;			/* card has a useable INT assigned to it */
		status_t pins_status;		/* B_OK if read correctly, B_ERROR if faked */
		bool sdram;					/* TRUE if SDRAM card: needed info for 2D acceleration */

		/* PINS */
		float f_ref;				/* PLL reference-oscillator frequency (Mhz) */
		uint32 max_system_vco;		/* graphics engine PLL VCO limits (Mhz) */
		uint32 min_system_vco;
		uint32 max_pixel_vco;		/* dac1 PLL VCO limits (Mhz) */
		uint32 min_pixel_vco;
		uint32 max_video_vco;		/* dac2, maven PLL VCO limits (Mhz) */
		uint32 min_video_vco;
		uint32 std_engine_clock;	/* graphics engine clock speed needed (Mhz) */
		uint32 std_engine_clock_dh;
		uint32 max_dac1_clock;		/* dac1 limits (Mhz) */
		uint32 max_dac1_clock_8;	/* dac1 limits correlated to RAMspeed limits (Mhz) */
		uint32 max_dac1_clock_16;
		uint32 max_dac1_clock_24;
		uint32 max_dac1_clock_32;
		uint32 max_dac1_clock_32dh;
		uint32 max_dac2_clock;		/* dac2 limits (Mhz) */
		uint32 max_dac2_clock_8;	/* dac2, maven limits correlated to RAMspeed limits (Mhz) */
		uint32 max_dac2_clock_16;
		uint32 max_dac2_clock_24;
		uint32 max_dac2_clock_32;
		uint32 max_dac2_clock_32dh;
		bool secondary_head;		/* presence of functions */
		bool tvout;
		bool primary_dvi;
		bool secondary_dvi;
		uint32 memory_size;			/* memory (Mb) */
		uint32 mctlwtst_reg;		/* memory control waitstate register */
		uint32 memrdbk_reg;			/* memory readback register */
		uint32 option_reg;			/* option register */
		uint32 option2_reg;			/* option2 register */
		uint32 option3_reg;			/* option3 register */
		uint32 option4_reg;			/* option4 register */
		uint8 v3_option2_reg;		/* pins v3 option2 register, not used for G100 */
		uint8 v3_clk_div;			/* pins v3 memory and system clock division factors */
		uint8 v3_mem_type;			/* pins v3 memory type info */
		uint16 v5_mem_type;			/* pins v5 memory type info */
	} ps;

	/* mirror of the ROM (copied in driver, because may not be mapped permanently - only over fb) */
	uint8 rom_mirror[32768];

	/* CRTC delay -> used in timing for MAVEN, depending on which CRTC is driving it */
	uint8 crtc_delay;

	/* MAVEN sync polarity offset from 'reset' situation: MAVEN sync polarity setup
	 * works in a serial fashion without readback or handy reset options! */
	uint8 maven_syncpol_offset;

	/* On G450/G550 we need this info for secondary head DPMS functionality */
	bool crossed_conns;

	/* apsed: some configuration settings from ~/config/settings/kernel/drivers/mga.settings if exists */
	settings settings;

	struct
	{
		overlay_buffer myBuffer[MAXBUFFERS];/* scaler input buffers */
		int_buf_info myBufInfo[MAXBUFFERS];	/* extra info on scaler input buffers */
		overlay_token myToken;				/* scaler is free/in use */
		benaphore lock;						/* for creating buffers and aquiring overlay unit routines */
		/* variables needed for virtualscreens (move_overlay()): */
		bool active;						/* true is overlay currently in use */
		overlay_window ow;					/* current position of overlay output window */
		overlay_buffer ob;					/* current inputbuffer in use */
		overlay_view my_ov;					/* current corrected view in inputbuffer */
		uint32 h_ifactor;					/* current 'unclipped' horizontal inverse scaling factor */
		uint32 v_ifactor;					/* current 'unclipped' vertical inverse scaling factor */
	} overlay;

} shared_info;

/* Read or write a value in PCI configuration space */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint32	offset;		/* Offset to read/write */
	uint32	size;		/* Number of bytes to transfer */
	uint32	value;		/* The value read or written */
} gx00_get_set_pci;

/* Set some boolean condition (like enabling or disabling interrupts) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;		/* state to set */
} gx00_set_bool_state;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	area_id	shared_info_area;	/* area_id containing the shared information */
} gx00_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;		/* The name of the device, less the /dev root */
} gx00_device_name;

enum {
	GX00_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif


#endif

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Apsed;
	Rudolf Cornelissen 10/2002-6/2008.
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
#include "video_overlay.h"
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

#define DRIVER_PREFIX "nvidia_gpgpu"
#define DEVICE_FORMAT "%04x_%04x_%02x%02x%02x"

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

#define INIT_BEN(x)		x.sem = create_sem(0, "NV_GPGPU "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);


#define NV_PRIVATE_DATA_MAGIC	0x0009 /* a private driver rev, of sorts */

/* dualhead extensions to flags */
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
	NV_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	NV_GET_PCI,
	NV_SET_PCI,
	NV_DEVICE_NAME,
	NV_RUN_INTERRUPTS,
	NV_ISA_OUT,
	NV_ISA_IN
};

/* card_type in order of date of NV chip design */
enum {
	NV40 = 0,
	NV41,
	NV43,
	NV44,
	NV45,
	G70,
	G71,
	G72,
	G73,
	G80,
	G84,
	G86
};

/* card_arch in order of date of NV chip design */
enum {
	NV40A = 0,
	NV50A
};

/* handles to pre-defined engine commands */
#define NV_ROP5_SOLID					0x00000000 /* 2D */
#define NV_IMAGE_BLACK_RECTANGLE		0x00000001 /* 2D/3D */
#define NV_IMAGE_PATTERN				0x00000002 /* 2D */
#define NV_SCALED_IMAGE_FROM_MEMORY		0x00000003 /* 2D */
#define NV_TCL_PRIMITIVE_3D				0x00000004 /* 3D */ //2007
#define NV4_SURFACE						0x00000010 /* 2D */
#define NV10_CONTEXT_SURFACES_2D		0x00000010 /* 2D */
#define NV_IMAGE_BLIT					0x00000011 /* 2D */
#define NV12_IMAGE_BLIT					0x00000011 /* 2D */
/* fixme:
 * never use NV3_GDI_RECTANGLE_TEXT for DMA acceleration:
 * There's a hardware fault in the input->output colorspace conversion here.
 * Besides, in NV40 and up this command nolonger exists. Both 'facts' are confirmed
 * by testing.
 */
//#define NV3_GDI_RECTANGLE_TEXT			0x00000012 /* 2D */
#define NV4_GDI_RECTANGLE_TEXT			0x00000012 /* 2D */
#define NV1_RENDER_SOLID_LIN			0x00000016 /* 2D: unused */

//-----------------------------------------------------------------------------------
/* safety byte-offset from end of cardRAM for preventing acceleration engine crashes
 * caused by the existance of DMA engine command buffers in cardRAM and/or fifo
 * channel engine command re-assigning on-the-fly */

/* NV40 and higher notes:
 * - we need at least 416kB distance from the DMA command buffer:
 *   If you get too close to the DMA command buffer on NV40 and NV43 at least (both
 *   confirmed), the source DMA instance will mess-up for at least engine command
 *   NV_IMAGE_BLIT and NV12_IMAGE_BLIT; 
 * - we need at least ???kB distance from the end of RAM for fifo-reassigning 'bug'
 *   (fixme: unknown yet because fifo assignment switching isn't used here atm);
 * - keep extra failsafe room to prevent malfunctioning apps from crashing engine. */
#define NV40_PLUS_OFFSET	512 * 1024

/* fifo re-assigning bug definition:
 * if the fifo assignment is changed while at the same time card memory in the
 * dangerous region is being accessed by some application, the engine will crash.
 * This bug applies for both PIO and DMA mode acceleration! */

/* source-DMA instance bug definition:
 * if card memory in the dangerous region is being accessed by some application while
 * a DMA command buffer exists in the same memory (though in a different place),
 * the engine will crash. */
//-----------------------------------------------------------------------------------

/* internal used info on overlay buffers */
typedef	struct {
	uint16 slopspace;
	uint32 size;
} int_buf_info;

typedef struct { // apsed, see comments in nv.settings
	// for driver
	char   accelerant[B_FILE_NAME_LENGTH];
	char   primary[B_FILE_NAME_LENGTH];
	bool   dumprom;
	// for accelerant
	uint32 logmask;
	uint32 memory;
	bool   usebios;
	bool   hardcursor;
	bool   switchhead;
	bool   pgm_panel;
	bool   force_sync;
	bool   force_ws;
	uint32 gpu_clk;
	uint32 ram_clk;
} nv_settings;

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
	area_id	unaligned_dma_area;	/* Area assigned for DMA. It will be (partially) mapped to an
									aligned area using MTRR-WC. */
	area_id	dma_area;	/* Aligned area assigned for DMA. The addresses are shared with all teams. */

	void	*framebuffer;		/* As viewed from virtual memory */
	void	*framebuffer_pci;	/* As viewed from the PCI bus (for DMA) */
	void	*dma_buffer;		/* As viewed from virtual memory */
	void	*dma_buffer_pci;	/* As viewed from the PCI bus (for DMA) */

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
		bool	dh_right;	/* Is cursor on right side of stretched screen? */
	} cursor;

  /*colour lookup table*/
	uint8	color_data[3 * 256];	/* Colour lookup table - as used by DAC */

  /*more display mode stuff*/
	display_mode dm;		/* current display mode configuration: head1 */
	uint32 dpms_flags;		/* current DPMS mode */
	bool acc_mode;			/* signals (non)accelerated mode */
	bool crtc_switch_mode;	/* signals dualhead switch mode if panels are used */

  /*frame buffer config - for BDirectScreen*/
	frame_buffer_config fbc;	/* bytes_per_row and start of frame buffer: head1 */
	accelerant_device_info adi;	/* as returned by hook GET_ACCELERANT_DEVICE_INFO */

  /*acceleration engine*/
	struct {
		uint32		count;		/* last dwgsync slot used */
		uint32		last_idle;	/* last dwgsync slot we *know* the engine was idle after */ 
		benaphore	lock;		/* for serializing access to the acc engine */
		struct {
			uint32	handle[0x08];	/* FIFO channel's cmd handle for the owning cmd */
			uint32	ch_ptr[0x20];	/* cmd handle's ptr to it's assigned FIFO ch (if any) */
		} fifo;
		struct {
			uint32 put;			/* last 32-bit-word adress given to engine to exec. to */
			uint32 current;		/* first free 32-bit-word adress in buffer */
			uint32 free;		/* nr. of useable free 32-bit words remaining in buffer */
			uint32 max;			/* command buffer's useable size in 32-bit words */
		} dma;
		struct {
			uint32 clones;		/* clone 'number' (mask, slot) (one bit per clone) */
			uint32 reload;		/* reload state and surfaces (one bit per clone) */
			uint32 newmode;		/* re-allocate all buffers (one bit per clone) */
			//fixme: memory stuff needs to be expanded (shared texture allocation?)
			uint32 mem_low;		/* ptr to first free mem adress: cardmem local offset */
			uint32 mem_high;	/* ptr to last free mem adress: cardmem local offset */
			bool mode_changing;	/* a mode-change is in progress (set/clear by 2D drv) */
		} threeD;
	} engine;

	struct
	{
		/* specialised registers for card initialisation read from NV BIOS (pins) */

		/* general card information */
		uint32 card_type;           /* see card_type enum above */
		uint32 card_arch;           /* see card_arch enum above */
		bool laptop;	            /* mobile chipset or not ('internal' flatpanel!) */
		bool slaved_tmds1;			/* external TMDS encoder active on CRTC1 */
		bool slaved_tmds2;			/* external TMDS encoder active on CRTC2 */
		bool master_tmds1;			/* on die TMDS encoder active on CRTC1 */
		bool master_tmds2;			/* on die TMDS encoder active on CRTC2 */
		bool tmds1_active;			/* found panel on CRTC1 that is active */
		bool tmds2_active;			/* found panel on CRTC2 that is active */
		display_timing p1_timing;	/* 'modeline' fetched for panel 1 */
		display_timing p2_timing;	/* 'modeline' fetched for panel 2 */
		float panel1_aspect;		/* panel's aspect ratio */
		float panel2_aspect;		/* panel's aspect ratio */
		bool crtc2_prim;			/* using CRTC2 as primary CRTC */
		bool i2c_bus0;				/* we have a wired I2C bus 0 on board */
		bool i2c_bus1;				/* we have a wired I2C bus 1 on board */
		bool i2c_bus2;				/* we have a wired I2C bus 2 on board */
		bool i2c_bus3;				/* we have a wired I2C bus 3 on board */
		uint8 monitors;				/* output devices connection matrix */
		bool int_assigned;			/* card has a useable INT assigned to it */
		status_t pins_status;		/* B_OK if read correctly, B_ERROR if faked */

		/* PINS */
		float f_ref;				/* PLL reference-oscillator frequency (Mhz) */
		bool ext_pll;				/* the extended PLL contains more dividers */
		uint32 max_system_vco;		/* graphics engine PLL VCO limits (Mhz) */
		uint32 min_system_vco;
		uint32 max_pixel_vco;		/* dac1 PLL VCO limits (Mhz) */
		uint32 min_pixel_vco;
		uint32 max_video_vco;		/* dac2 PLL VCO limits (Mhz) */
		uint32 min_video_vco;
		uint32 std_engine_clock;	/* graphics engine clock speed needed (Mhz) */
		uint32 std_memory_clock;	/* card memory clock speed needed (Mhz) */
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
		bool primary_dvi;
		bool secondary_dvi;
		uint32 memory_size;			/* memory (in bytes) */
	} ps;

	/* mirror of the ROM (copied in driver, because may not be mapped permanently) */
	uint8 rom_mirror[65536];

	/* some configuration settings from ~/config/settings/kernel/drivers/nv.settings if exists */
	nv_settings settings;

} shared_info;

/* Read or write a value in PCI configuration space */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint32	offset;		/* Offset to read/write */
	uint32	size;		/* Number of bytes to transfer */
	uint32	value;		/* The value read or written */
} nv_get_set_pci;

/* Enable or Disable CRTC (1,2) interrupts */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	crtc;		/* adressed CRTC */
	bool	do_it;		/* state to set */
} nv_set_vblank_int;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	area_id	shared_info_area;	/* area_id containing the shared information */
} nv_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;		/* The name of the device, less the /dev root */
} nv_device_name;

/* Read or write a value in ISA I/O space */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint16	adress;		/* Offset to read/write */
	uint8	size;		/* Number of bytes to transfer */
	uint16	data;		/* The value read or written */
} nv_in_out_isa;

enum {
	
	_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif


#endif

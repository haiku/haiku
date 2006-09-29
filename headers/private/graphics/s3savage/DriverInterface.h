/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
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

typedef struct
{
    sem_id  sem;
    int32   ben;
} benaphore;

#define INIT_BEN(x)     x.sem = create_sem(0, "SAVAGE "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)   if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)  if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define DELETE_BEN(x)   delete_sem(x.sem);


#define SAVAGE_PRIVATE_DATA_MAGIC    0x1234 /* a private driver rev, of sorts */

#define MAX_SAVAGE_DEVICE_NAME_LENGTH 32

#define SKD_MOVE_CURSOR    0x00000001
#define SKD_PROGRAM_CLUT   0x00000002
#define SKD_SET_START_ADDR 0x00000004
#define SKD_SET_CURSOR     0x00000008
#define SKD_HANDLER_INSTALLED 0x80000000

enum
{
    SAVAGE_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
    SAVAGE_GET_PCI,
    SAVAGE_SET_PCI,
    SAVAGE_DEVICE_NAME,
    SAVAGE_RUN_INTERRUPTS,
    SAVAGE_DPRINTF
};

typedef struct
{
    uint16  vendor_id;  /* PCI vendor ID, from pci_info */
    uint16  device_id;  /* PCI device ID, from pci_info */
    uint8   revision;   /* PCI device revsion, from pci_info */
    area_id regs_area;  /* Kernel's area_id for the memory mapped registers.
                           It will be cloned into the accelerant's address
                           space. */
    area_id fb_area;    /* Frame buffer's area_id.  The addresses are shared
                           with all teams. */
    void    *framebuffer;   /* As viewed from virtual memory */
    void    *framebuffer_pci;   /* As viewed from the PCI bus (for DMA) */
    area_id rom_area;   /* Mapped ROM's area_id */
    void    *rom;       /* As viewed from virtual memory.  Shared by all teams */
    area_id mode_area;  /* Contains the list of display modes the driver supports */
    uint32  mode_count; /* Number of display modes in the list */
    sem_id  vblank; /* The vertical blank semaphore.  Ownership will be
                       transfered to the team opening the device first */
    int32   flags;    
    int32   start_addr;

    struct
    {
        uint8*  data;   /*  Pointer into the frame buffer to where the
                            cursor data starts */
        uint16  hot_x;  /* Cursor hot spot. The top left corner of the cursor */
        uint16  hot_y;  /* is 0,0 */
        uint16  x;      /* The location of the cursor hot spot on the */
        uint16  y;      /* display (or desktop?) */
        uint16  width;  /* Width and height of the cursor shape */
        uint16  height;
        bool    is_visible; /* Is the cursor currently displayed? */
    }   cursor;
    uint16  first_color;
    uint16  color_count;
    bigtime_t   refresh_period; /* Duration of one frame (ie 1/refresh rate) */
    bigtime_t   blank_period;   /* Duration of the blanking period.   These are
                                   usefull when faking vertical blanking
                                   interrupts. */
    uint8   color_data[3 * 256];    /* */
    uint8   cursor0[64*64/8];       /* AND mask for a 64x64 cursor */
    uint8   cursor1[512];    	    /* XOR mask for a 64x64 cursor */
    display_mode dm;    /* current display mode configuration */
    frame_buffer_config
            fbc;	/* bytes_per_row and start of frame buffer */
    struct
    {
        uint64  count;       /* last fifo slot used */
        uint64  last_idle;   /* last fifo slot we *know* the engine was idle after */ 
        benaphore    lock;   /* for serializing access to the acceleration engine */
    }   engine;

    uint32    pix_clk_max8;    	/* The maximum speed the pixel clock should run */
    uint32    pix_clk_max16;    /*  at for a given pixel width.  Usually a function */
    uint32    pix_clk_max32;    /*  of memory and DAC bandwidths. */
    uint32    mem_size;         /* Frame buffer memory, in bytes. */
} shared_info;

/* Read or write a value in PCI configuration space */
typedef struct
{
    uint32    magic;    /* magic number to make sure the caller groks us */
    uint32    offset;   /* Offset to read/write */
    uint32    size;    	/* Number of bytes to transfer */
    uint32    value;   	/* The value read or written */
} savage_get_set_pci;

/* Set some boolean condition (like enabling or disabling interrupts) */
typedef struct
{
    uint32  magic; /* magic number to make sure the caller groks us */
    bool    do_it; /* state to set */
} savage_set_bool_state;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct
{
    uint32  magic;        /* magic number to make sure the caller groks us */
    area_id shared_info_area;    /* area_id containing the shared information */
} savage_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct
{
    uint32    magic;        /* magic number to make sure the caller groks us */
    char    *name;        /* The name of the device, less the /dev root */
} savage_device_name;

enum
{
    SAVAGE_WAIT_FOR_VBLANK = (0 << 0)
};

enum 
{
	/* Savage3D series */
	PCI_PID_SAVAGE3D = 0x8a20,		/* Savage3D */
	PCI_PID_SAVAGE3DMV = 0x8a21,	/* Savage3D/MV */
	PCI_PID_SAVAGEMXMV = 0x8c10,	/* Savage/MX-MV */
	PCI_PID_SAVAGEMX = 0x8c11,		/* Savage/MX */
	PCI_PID_SAVAGEIXMV = 0x8c12,	/* Savage/IX-MV */
	PCI_PID_SAVAGEIX = 0x8c13,		/* Savage/IX */

	/* Savage4 series */
	PCI_PID_SAVAGE4_2 = 0x8a22,		/* Savage4 */
	PCI_PID_SAVAGE4_3 = 0x8a23,		/* Savage4 */
	PCI_PID_SAVAGE2000 = 0x9102,	/* Savage2000 */
	PCI_PID_PM133 = 0x8a25,			/* ProSavage PM133 */
	PCI_PID_KM133 = 0x8a26,			/* ProSavage KM133 */
	PCI_PID_PN133 = 0x8d01,			/* ProSavage PN133, 86C380 [ProSavageDDR K4M266] */
	PCI_PID_KN133 = 0x8d02,			/* ProSavage KN133/TwisterK AGP4X VT8636A */
	PCI_PID_KM266 = 0x8d04,			/* ProSavage8 KM266/KL266 VT8375  */
	PCI_PID_PN266 = 0x8d03,			/* VT8751 [ProSavageDDR P4M266] */

	/* SuperSavage series (unsupported) */
	PCI_PID_SUPERSAVAGE_MX128 = 0x8c22,		/* SuperSavage MX/128 */
	PCI_PID_SUPERSAVAGE_MX64 = 0x8c24,		/* SuperSavage MX/64 */
	PCI_PID_SUPERSAVAGE_MX64C = 0x8c26,		/* SuperSavage MX/64C */
	PCI_PID_SUPERSAVAGE_IX128_SDR = 0x8c2a,	/* SuperSavage IX/128 SDR */
	PCI_PID_SUPERSAVAGE_IX128_DDR = 0x8c2b,	/* SuperSavage IX/128 DDR */
	PCI_PID_SUPERSAVAGE_IX64_SDR = 0x8c2c,	/* SuperSavage IX/64 SDR */
	PCI_PID_SUPERSAVAGE_IX64_DDR = 0x8c2d,	/* SuperSavage IX/64 DDR */
	PCI_PID_SUPERSAVAGE_IXC_SDR = 0x8c2e,	/* SuperSavage IX/C SDR */
	PCI_PID_SUPERSAVAGE_IXC_DDR = 0x8c2f,	/* SuperSavage IX/C DDR */
};

#define	isSavage4Family(p)		\
	((p) == PCI_PID_SAVAGE4_2 ||	\
	 (p) == PCI_PID_SAVAGE4_3 ||	\
	 (p) == PCI_PID_SAVAGE2000 ||	\
	 (p) == PCI_PID_PM133 ||	\
	 (p) == PCI_PID_KM133 ||	\
	 (p) == PCI_PID_PN133 ||	\
	 (p) == PCI_PID_KN133 ||	\
	 (p) == PCI_PID_KM266)


#if defined(__cplusplus)
}
#endif


#endif

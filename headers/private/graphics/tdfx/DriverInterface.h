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

#define MAXBUFFERS	8

typedef	struct
{
	uint16 slopspace;
	uint32 size;
} int_buf_info;

typedef struct
{
    sem_id  sem;
    int32   ben;
} benaphore;

#define INIT_BEN(x)     x.sem = create_sem(0, "VOODOO "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)   if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)  if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define DELETE_BEN(x)   delete_sem(x.sem);


#define VOODOO_PRIVATE_DATA_MAGIC    0x1234 /* a private driver rev, of sorts */

#define MAX_VOODOO_DEVICE_NAME_LENGTH 32

#define SKD_MOVE_CURSOR    0x00000001
#define SKD_PROGRAM_CLUT   0x00000002
#define SKD_SET_START_ADDR 0x00000004
#define SKD_SET_CURSOR     0x00000008
#define SKD_HANDLER_INSTALLED 0x80000000

enum
{
    VOODOO_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
    VOODOO_GET_PCI,
    VOODOO_SET_PCI,
    VOODOO_DEVICE_NAME,
    VOODOO_RUN_INTERRUPTS,
    VOODOO_DPRINTF,
    VOODOO_INB,
    VOODOO_INW,
    VOODOO_INL,
    VOODOO_OUTB,
    VOODOO_OUTW,
    VOODOO_OUTL
};

typedef struct
{
    uint16  vendor_id;  /* PCI vendor ID, from pci_info */
    uint16  device_id;  /* PCI device ID, from pci_info */
    uint8   revision;   /* PCI device revsion, from pci_info */
    uint32  iobase;		/* PCI IOBase */
    
    area_id regs_area;  /* Kernel's area_id for the memory mapped registers.
                           It will be cloned into the accelerant's address
                           space. */
    area_id fb_area;    /* Frame buffer's area_id.  The addresses are shared
                           with all teams. */
    void    *framebuffer;   /* As viewed from virtual memory */
    void    *framebuffer_pci;   /* As viewed from the PCI bus (for DMA) */

    area_id io_area;   /* Mapped ROM's area_id */
    uint32    io;       /* As viewed from virtual memory.  Shared by all teams */
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

	struct
	{ 
    	overlay_buffer myBuffer[MAXBUFFERS];	/* scaler input buffers */ 
        int_buf_info myBufInfo[MAXBUFFERS];     /* extra info on scaler input buffers */ 
        overlay_token myToken;                  /* scaler is free/in use */ 
        benaphore lock;                         /* for creating buffers and aquiring overlay unit routines */ 
    } overlay; 

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
} voodoo_get_set_pci;

/* Set some boolean condition (like enabling or disabling interrupts) */
typedef struct
{
    uint32  magic; /* magic number to make sure the caller groks us */
    bool    do_it; /* state to set */
} voodoo_set_bool_state;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct
{
    uint32  magic;        /* magic number to make sure the caller groks us */
    area_id shared_info_area;    /* area_id containing the shared information */
} voodoo_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct
{
    uint32    magic;        /* magic number to make sure the caller groks us */
    char    *name;        /* The name of the device, less the /dev root */
} voodoo_device_name;

typedef struct
{
	uint32 port;
	uint8 data8;
	uint16 data16;
	uint32 data32;
} voodoo_port_io;

enum
{
    VOODOO_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif


#endif

/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#ifndef _DRIVERINTERFACE_H_
#define _DRIVERINTERFACE_H_

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>


#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This is the info that needs to be shared between the kernel driver and
 * the accelerant for the et6000 driver.
 */

/*****************************************************************************/
typedef struct {
    sem_id  sem;
    int32   ben;
} benaphore;

#define INIT_BEN(x) x.sem = create_sem(0, "ET6000 "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x) if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x) if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define DELETE_BEN(x) delete_sem(x.sem);
/*****************************************************************************/
#define ET6000_PRIVATE_DATA_MAGIC 0x100CC001
/*****************************************************************************/
#define ET6000_HANDLER_INSTALLED 0x80000000
/*****************************************************************************/
/*
 * How many bytes of memory the accelerator (ACL) takes from the off-screen
 * part of the adapter onboard memory to perform some of its operations.
 */
#define ET6000_ACL_NEEDS_MEMORY 4
/*****************************************************************************/
enum {
    ET6000_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
    ET6000_GET_PCI,
    ET6000_SET_PCI,
    ET6000_DEVICE_NAME,
    ET6000_PROPOSE_DISPLAY_MODE,
    ET6000_SET_DISPLAY_MODE
};
/*****************************************************************************/
typedef struct {
    uint16  vendor_id; /* PCI vendor ID, from pci_info */
    uint16  device_id; /* PCI device ID, from pci_info */
    uint8   revision;  /* PCI device revsion, from pci_info */
    area_id memoryArea;    /* Onboard memory's area_id. The addresses
                            * are shared with all teams. */
    void *framebuffer;     /* Start of the mapped framebuffer */
    void *physFramebuffer; /* Physical address of start of framebuffer */
    void *memory;          /* Start of the mapped adapter onboard memory */
    void *physMemory;      /* Physical address of start of onboard memory */
    uint32 memSize;        /* Size of available onboard memory, bytes. */
    uint16 pciConfigSpace; /* PCI header base I/O address */
    void *mmRegs; /* memory mapped registers */
    void *emRegs; /* external mapped registers */
    area_id modesArea; /* Contains the list of display modes the driver supports */
    uint32  modesNum; /* Number of display modes in the list */
    int32   flags;

    display_mode dm; /* current display mode configuration */
    uint8 bytesPerPixel; /* bytes(!) per pixel at current display mode */
    frame_buffer_config fbc; /* bytes_per_row and start of frame buffer */

    struct {
        uint64 count;     /* last fifo slot used */
        uint64 lastIdle; /* last fifo slot we *know* the engine was idle after */
        benaphore lock;   /* for serializing access to the acceleration engine */
    } engine;

    uint32 pixelClockMax16; /* The maximum speed the pixel clock should run */
    uint32 pixelClockMax24; /* at for a given pixel width. Usually a function */
                            /* of memory and DAC bandwidths. */
} ET6000SharedInfo;
/*****************************************************************************/
/* Read or write a value in PCI configuration space */
typedef struct {
    uint32  magic;      /* magic number to make sure the caller groks us */
    uint32  offset;     /* Offset to read/write */
    uint32  size;       /* Number of bytes to transfer */
    uint32  value;      /* The value read or written */
} ET6000GetSetPCI;
/*****************************************************************************/
/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct {
    uint32  magic; /* magic number to make sure the caller groks us */
    area_id sharedInfoArea; /* area_id containing the shared information */
} ET6000GetPrivateData;
/*****************************************************************************/
/*
 * Retrieve the device name. Usefull for when we have a file handle, but
 * want to know the device name (like when we are cloning the accelerant).
 */
typedef struct {
    uint32  magic;     /* magic number to make sure the caller groks us */
    char    *name;     /* The name of the device, less the /dev root */
} ET6000DeviceName;
/*****************************************************************************/
typedef struct {
    uint32  magic;     /* magic number to make sure the caller groks us */
    display_mode mode; /* Proposed mode or mode to set */
    uint16 pciConfigSpace; /* For setting the mode */
    uint32 memSize; /* For proposing the mode */
} ET6000DisplayMode;
/*****************************************************************************/

#if defined(__cplusplus)
}
#endif


#endif /* _DRIVERINTERFACE_H_ */

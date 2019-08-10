/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

/* standard kernel driver stuff */
#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>

/* this is for the standardized portion of the driver API */
/* currently only one operation is defined: B_GET_ACCELERANT_SIGNATURE */
#include <graphic_driver.h>

/* this is for sprintf() */
#include <stdio.h>

/* this is for string compares */
#include <string.h>

/* The private interface between the accelerant and the kernel driver. */
#include "DriverInterface.h"

#include "setmode.h"
#include "acl.h"
#include "bits.h"

/*****************************************************************************/
#if DEBUG > 0
#define ddprintf(a)     dprintf a
#else
#define ddprintf(a)
#endif

#define MAX_DEVICES     8

/* Tell the kernel what revision of the driver API we support */
int32   api_version = 2;

/*****************************************************************************/
/* This structure is private to the kernel driver */
typedef struct {
    uint32 isOpen; /* a count of how many times the devices has been opened */
    area_id sharedArea; /* the area shared between the driver and all of the accelerants */
    ET6000SharedInfo *si; /* a pointer to the shared area, for convenience */
#if DEBUG > 0
    uint32 interrupt_count; /* if we're debugging, a count of how many times
                       the interrupt handler has been called for this device */
#endif
    pci_info pcii; /* a convenience copy of the pci info for this device */
    char name[B_OS_NAME_LENGTH]; /* where we keep the name of the device for publishing and comparing */
} ET6000DeviceInfo;
/*****************************************************************************/
typedef struct {
#if DEBUG > 0
    uint32 total_interrupts; /* total number of interrupts seen by our handler */
#endif
    uint32 count; /* number of devices actually found */
    benaphore kernel; /* for serializing opens/closes */
    char *deviceNames[MAX_DEVICES+1]; /* device name pointer storage */
    ET6000DeviceInfo di[MAX_DEVICES]; /* device specific stuff */
} DeviceData;
/*****************************************************************************/
static DeviceData *pd;
/*****************************************************************************/
/* prototypes for our private functions */
static status_t et6000OpenHook(const char* name, uint32 flags, void** cookie);
static status_t et6000CloseHook(void* dev);
static status_t et6000FreeHook(void* dev);
static status_t et6000ReadHook(void* dev, off_t pos, void* buf, size_t* len);
static status_t et6000WriteHook(void* dev, off_t pos, const void* buf, size_t* len);
static status_t et6000ControlHook(void* dev, uint32 msg, void *buf, size_t len);
static status_t et6000MapDevice(ET6000DeviceInfo *di);
static void et6000UnmapDevice(ET6000DeviceInfo *di);
static void et6000ProbeDevices(void);
static int32 et6000Interrupt(void *data);

#if DEBUG > 0
static int et6000dump(int argc, char **argv);
#endif
/*****************************************************************************/
static pci_module_info *pci_bus;

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))

#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))
/*****************************************************************************/
static device_hooks et6000DeviceHooks = {
    et6000OpenHook,
    et6000CloseHook,
    et6000FreeHook,
    et6000ControlHook,
    et6000ReadHook,
    et6000WriteHook,
    NULL,
    NULL,
    NULL,
    NULL
};
/*****************************************************************************/
#define TSENG_VENDOR_ID 0x100C /* Tseng Labs Inc */

static uint16 et6000DeviceList[] = {
    0x3208, /* ET6000/ET6100 */
    0x4702, /* ET6300 */
    0
};

static struct {
    uint16  vendor;
    uint16  *devices;
} supportedDevices[] = {
    {TSENG_VENDOR_ID, et6000DeviceList},
    {0x0000, NULL}
};
/*****************************************************************************/
/*
 * Returns B_OK if one is found, otherwise returns
 * B_ERROR so the driver will be unloaded.
 */
status_t init_hardware(void) {
long pciIndex = 0;
pci_info pcii;
bool foundOne = FALSE;

    /* choke if we can't find the PCI bus */
    if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
        return B_ERROR;

    /* while there are more pci devices */
    while ((*pci_bus->get_nth_pci_info)(pciIndex, &pcii) == B_NO_ERROR) {
        int vendor = 0;

        ddprintf(("ET6000 init_hardware(): checking pci index %ld, device 0x%04x/0x%04x\n", pciIndex, pcii.vendor_id, pcii.device_id));
        /* if we match a supported vendor */
        while (supportedDevices[vendor].vendor) {
            if (supportedDevices[vendor].vendor == pcii.vendor_id) {
                uint16 *devices = supportedDevices[vendor].devices;
                /* while there are more supported devices */
                while (*devices) {
                    /* if we match a supported device */
                    if (*devices == pcii.device_id) {
                        ddprintf(("ET6000: we support this device\n"));
                        foundOne = TRUE;
                        goto done;
                    }
                    /* next supported device */
                    devices++;
                }
            }
            vendor++;
        }
        /* next pci_info struct, please */
        pciIndex++;
    }
    ddprintf(("ET6000: init_hardware - no supported devices\n"));

done:
    /* put away the module manager */
    put_module(B_PCI_MODULE_NAME);
    return (foundOne ? B_OK : B_ERROR);
}
/*****************************************************************************/
static void et6000ProbeDevices(void) {
uint32 pciIndex = 0;
uint32 count = 0;
ET6000DeviceInfo *di = pd->di;

    /* while there are more pci devices */
    while ((count < MAX_DEVICES) &&
        ((*pci_bus->get_nth_pci_info)(pciIndex, &(di->pcii)) == B_NO_ERROR))
    {
        int vendor = 0;

        ddprintf(("ET6000: checking pci index %ld, device 0x%04x/0x%04x\n", pciIndex, di->pcii.vendor_id, di->pcii.device_id));
        /* if we match a supported vendor */
        while (supportedDevices[vendor].vendor) {
            if (supportedDevices[vendor].vendor == di->pcii.vendor_id) {
                uint16 *devices = supportedDevices[vendor].devices;
                /* while there are more supported devices */
                while (*devices) {
                    /* if we match a supported device */
                    if (*devices == di->pcii.device_id) {
                        /* publish the device name */
                        sprintf(di->name, "graphics/%04X_%04X_%02X%02X%02X",
                                di->pcii.vendor_id, di->pcii.device_id,
                                di->pcii.bus, di->pcii.device, di->pcii.function);
                        ddprintf(("ET6000: making /dev/%s\n", di->name));
                        /* remember the name */
                        pd->deviceNames[count] = di->name;
                        /* mark the driver as available for R/W open */
                        di->isOpen = 0;
                        /* mark areas as not yet created */
                        di->sharedArea = -1;
                        /* mark pointer to shared data as invalid */
                        di->si = NULL;
                        /* inc pointer to device info */
                        di++;
                        /* inc count */
                        count++;
                        /* break out of these while loops */
                        goto next_device;
                    }
                    /* next supported device */
                    devices++;
                }
            }
            vendor++;
        }
next_device:
        /* next pci_info struct, please */
        pciIndex++;
    }
    /* propagate count */
    pd->count = count;
    /* terminate list of device names with a null pointer */
    pd->deviceNames[pd->count] = NULL;
    ddprintf(("SKD et6000ProbeDevices: %ld supported devices\n", pd->count));
}
/*****************************************************************************/
status_t init_driver(void) {
    /* get a handle for the pci bus */
    if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
        return B_ERROR;

    /* driver private data */
    pd = (DeviceData *)calloc(1, sizeof(DeviceData));
    if (!pd) {
        put_module(B_PCI_MODULE_NAME);
        return B_ERROR;
    }
    /* initialize the benaphore */
    INIT_BEN(pd->kernel);

    /* find all of our supported devices */
    et6000ProbeDevices();

#if DEBUG > 0
    add_debugger_command("et6000dump", et6000dump, "dump ET6000 kernel driver persistant data");
#endif

    return B_OK;
}
/*****************************************************************************/
const char **publish_devices(void) {
    /* return the list of supported devices */
    return (const char **)pd->deviceNames;
}
/*****************************************************************************/
device_hooks *find_device(const char *name) {
int index = 0;
    while (pd->deviceNames[index]) {
        if (strcmp(name, pd->deviceNames[index]) == 0)
            return &et6000DeviceHooks;
        index++;
    }
    return NULL;
}
/*****************************************************************************/
void uninit_driver(void) {

#if DEBUG > 0
    remove_debugger_command("et6000dump", et6000dump);
#endif

    /* free the driver data */
    DELETE_BEN(pd->kernel);
    free(pd);
    pd = NULL;

    /* put the pci module away */
    put_module(B_PCI_MODULE_NAME);
}
/*****************************************************************************/
static int32 et6000Interrupt(void *data) {
int32 handled = B_UNHANDLED_INTERRUPT;
ET6000DeviceInfo *di = (ET6000DeviceInfo *)data;
ET6000SharedInfo *si = di->si;
int32 *flags = &(si->flags);

#if DEBUG > 0
    pd->total_interrupts++;
#endif

    /* is someone already handling an interrupt for this device? */
    if (atomic_or(flags, ET6000_HANDLER_INSTALLED) & ET6000_HANDLER_INSTALLED) {
#if DEBUG > 0
        kprintf("ET6000: Already in handler!\n");
#endif
        goto exit0;
    }

    switch (et6000aclInterruptCause(si->mmRegs)) {
    case ET6000_ACL_INT_CAUSE_NONE:
        handled = B_UNHANDLED_INTERRUPT;
        break;
    case ET6000_ACL_INT_CAUSE_READ:
        et6000aclReadInterruptClear(si->mmRegs);
        handled = B_HANDLED_INTERRUPT;
        break;
    case ET6000_ACL_INT_CAUSE_WRITE:
        et6000aclWriteInterruptClear(si->mmRegs);
        handled = B_HANDLED_INTERRUPT;
        break;
    case ET6000_ACL_INT_CAUSE_BOTH: /* Can it be at all? */
        et6000aclReadInterruptClear(si->mmRegs);
        et6000aclWriteInterruptClear(si->mmRegs);
        handled = B_HANDLED_INTERRUPT;
        break;
    }

#if DEBUG > 0
    /* increment the counter for this device */
    if (handled == B_HANDLED_INTERRUPT)
        di->interrupt_count++;
#endif

    /* note that we're not in the handler any more */
    atomic_and(flags, ~ET6000_HANDLER_INSTALLED);

exit0:
    return handled;
}
/*****************************************************************************/
static uint32 et6000GetOnboardMemorySize(uint16 pciConfigSpace,
                                  volatile void *memory)
{
uint32 memSize = 0;

    ioSet8(0x3d8, 0x00, 0xa0); /* Set the KEY for color modes */
    ioSet8(0x3b8, 0x00, 0xa0); /* Set the KEY for monochrome modes */

    switch (ioGet8(0x3C2) & 0x03) {
    case 0x00: /* onboard memory is of DRAM type */
        memSize = 1024*1024 * ((ioGet8(pciConfigSpace + 0x45) & 0x03) + 1);
        break;
    case 0x03: /* onboard memory is of MDRAM type */
        memSize = /* number*8 of 32kb banks per channel */
            ((ioGet8(pciConfigSpace + 0x47) & 0x07) + 1) * 8 * 32*1024;
        if (ioGet8(pciConfigSpace + 0x45) & 0x04) /* If 2 channels */
            memSize *= 2;
        break;
    default:  /* onboard memory is of unknown type */
        memSize = 4196*1024; /* Let it be of maximum possible size */
    }

    /*
     * This algorithm would fail to recongize 2.25Mb of onboard
     * memory - it would detect 2.5Mb instead. It needs to be fixed.
     */
    if (memSize == 2621440) { /* If 2.5Mb detected */
        uint8 pci40 = ioGet8(pciConfigSpace+0x40);
        et6000EnableLinearMemoryMapping(pciConfigSpace);

        /* Check whether the memory beyond 2.25Mb really exists */
        *(volatile uint32 *)((uint32)memory + 2359296) = 0xaa55aa55;
        if (*(volatile uint32 *)((uint32)memory + 2359296) != 0xaa55aa55)
            memSize = 2359296; /* It's 2.25Mb */

        ioSet8(pciConfigSpace+0x40, 0x00, pci40); /* Restore */
    }

    return memSize;
}
/*****************************************************************************/
static status_t et6000MapDevice(ET6000DeviceInfo *di) {
char buffer[B_OS_NAME_LENGTH];
ET6000SharedInfo *si = di->si;
uint32  tmpUlong;
pci_info *pcii = &(di->pcii);

    /* Enable memory space access and I/O space access */
    tmpUlong = get_pci(PCI_command, 4);
    tmpUlong |= 0x00000003;
    set_pci(PCI_command, 4, tmpUlong);

    /* Enable ROM decoding */
    tmpUlong = get_pci(PCI_rom_base, 4);
    tmpUlong |= 0x00000001;
    set_pci(PCI_rom_base, 4, tmpUlong);

    /* PCI header base address in I/O space */
    si->pciConfigSpace = (uint16)di->pcii.u.h0.base_registers[1];

    sprintf(buffer, "%04X_%04X_%02X%02X%02X videomemory",
        di->pcii.vendor_id, di->pcii.device_id,
        di->pcii.bus, di->pcii.device, di->pcii.function);

   /*
    * We map the whole graphics card memory area (which consist of RAM memory
    * and memory mapped registers) at once. Memory mapped registers must not
    * be cacheble, so the whole area is mapped with B_MTR_UC (unable caching).
    * We certainly could map separately the RAM memory with write combining
    * (B_MTR_WC) and the memory mapped registers with B_MTR_UC.
    */
    si->memoryArea = map_physical_memory(buffer,
        di->pcii.u.h0.base_registers[0],
        di->pcii.u.h0.base_register_sizes[0],
        B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_UC,
        B_READ_AREA + B_WRITE_AREA,
        &(si->memory));

    si->framebuffer = si->memory;
    si->mmRegs = (void *)((uint32)si->memory + 0x003fff00);
    si->emRegs = (void *)((uint32)si->memory + 0x003fe000);

    /* remember the physical addresses */
    si->physMemory = si->physFramebuffer =
        (void *) di->pcii.u.h0.base_registers_pci[0];

    si->memSize = et6000GetOnboardMemorySize(si->pciConfigSpace, si->memory);

    /* in any case, return the result */
    return si->memoryArea;
}
/*****************************************************************************/
static void et6000UnmapDevice(ET6000DeviceInfo *di) {
ET6000SharedInfo *si = di->si;

    ddprintf(("et6000UnmapDevice(%08lx) begins...\n", (uint32)di));
    ddprintf((" memoryArea: %ld\n", si->memoryArea));

    if (si->memoryArea >= 0)
        delete_area(si->memoryArea);
    si->memoryArea = -1;
    si->framebuffer = NULL;
    si->physFramebuffer = NULL;
    si->memory = NULL;
    si->physMemory = NULL;

    ddprintf(("et6000UnmapDevice() ends.\n"));
}
/*****************************************************************************/
static status_t et6000OpenHook(const char* name, uint32 flags, void** cookie) {
int32 index = 0;
ET6000DeviceInfo *di;
ET6000SharedInfo *si;
status_t        result = B_OK;
char shared_name[B_OS_NAME_LENGTH];

    ddprintf(("SKD et6000OpenHook(%s, %ld, 0x%08lx)\n", name, flags, (uint32)cookie));

    /* find the device name in the list of devices */
    /* we're never passed a name we didn't publish */
    while(pd->deviceNames[index] &&
         (strcmp(name, pd->deviceNames[index]) != 0))
    {
        index++;
    }

    /* for convienience */
    di = &(pd->di[index]);

    /* make sure no one else has write access to the common data */
    AQUIRE_BEN(pd->kernel);

    /* if it's already open for writing */
    if (di->isOpen) {
        /* mark it open another time */
        goto mark_as_open;
    }
    /* create the shared area */
    sprintf(shared_name, "%04X_%04X_%02X%02X%02X shared",
        di->pcii.vendor_id, di->pcii.device_id,
        di->pcii.bus, di->pcii.device, di->pcii.function);
    /* create this area with NO user-space read or write permissions, to prevent accidental dammage */
    di->sharedArea = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS, ((sizeof(ET6000SharedInfo) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)), B_FULL_LOCK,
		B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
    if (di->sharedArea < 0) {
        /* return the error */
        result = di->sharedArea;
        goto done;
    }

    /* save a few dereferences */
    si = di->si;

    /* save the vendor and device IDs */
    si->vendor_id = di->pcii.vendor_id;
    si->device_id = di->pcii.device_id;
    si->revision = di->pcii.revision;

    si->pixelClockMax16 = 135000;
    si->pixelClockMax24 = 135000;
    if (si->vendor_id == 0x100C) { /* Tseng Labs, Inc. */
        switch (si->device_id) {
        case 0x3208:/* ET6000/ET6100 */
            if (si->revision < 0x70) { /* ET6000 */
                si->pixelClockMax16 = 135000;
                si->pixelClockMax24 = 135000;
            }
            else { /* ET6100 */
                si->pixelClockMax16 = 175000;
                si->pixelClockMax24 = 175000;
            }
            break;
        case 0x4702: /* ET6300 */
            si->pixelClockMax16 = 220000;
            si->pixelClockMax24 = 220000;
            break;
        }
    }

    /* map the device */
    result = et6000MapDevice(di);
    if (result < 0)
        goto free_shared;
    result = B_OK;

    /*
     * Clear any pending interrupts and disable interrupts. Driver
     * currently does not use interrupts and unlikely will in future.
     */
    et6000aclReadInterruptClear(si->mmRegs);
    et6000aclWriteInterruptClear(si->mmRegs);
    et6000aclMasterInterruptDisable(si->mmRegs);

    /* Install the interrupt handler */
    result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line,
                                       et6000Interrupt, (void *)di, 0);
    /* bail if we couldn't install the handler */
    if (result != B_OK)
        goto unmap;

mark_as_open:
    /* mark the device open */
    di->isOpen++;

    /* send the cookie to the opener */
    *cookie = di;

    goto done;

unmap:
    et6000UnmapDevice(di);

free_shared:
    /* clean up our shared area */
    delete_area(di->sharedArea);
    di->sharedArea = -1;
    di->si = NULL;

done:
    /* end of critical section */
    RELEASE_BEN(pd->kernel);

    /* all done, return the status */
    ddprintf(("et6000OpenHook returning 0x%08lx\n", result));

    return result;
}
/*****************************************************************************/
/*
 * et6000ReadHook - does nothing, gracefully
 */
static status_t et6000ReadHook(void* dev, off_t pos, void* buf, size_t* len)
{
    *len = 0;
    return B_NOT_ALLOWED;
}

/*****************************************************************************/
/*
 * et6000WriteHook - does nothing, gracefully
 */
static status_t et6000WriteHook(void* dev, off_t pos, const void* buf, size_t* len)
{
    *len = 0;
    return B_NOT_ALLOWED;
}
/*****************************************************************************/
/*
 * et6000CloseHook - does nothing, gracefully
 */
static status_t et6000CloseHook(void* dev)
{
    ddprintf(("SKD et6000CloseHook(%08lx)\n", (uint32)dev));
    /* we don't do anything on close: there might be dup'd fd */
    return B_NO_ERROR;
}
/*****************************************************************************/
/*
 * et6000FreeHook - close down the device
 */
static status_t et6000FreeHook(void* dev) {
ET6000DeviceInfo *di = (ET6000DeviceInfo *)dev;
ET6000SharedInfo *si = di->si;

    ddprintf(("SKD et6000FreeHook() begins...\n"));
    /* lock the driver */
    AQUIRE_BEN(pd->kernel);

    /* if opened multiple times, decrement the open count and exit */
    if (di->isOpen > 1)
        goto unlock_and_exit;

    /* Clear any pending interrupts and disable interrupts. */
    et6000aclReadInterruptClear(si->mmRegs);
    et6000aclWriteInterruptClear(si->mmRegs);
    et6000aclMasterInterruptDisable(si->mmRegs);

    /* Remove the interrupt handler */
    remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, et6000Interrupt, di);

    /* free framebuffer area */
    et6000UnmapDevice(di);

    /* clean up our shared area */
    delete_area(di->sharedArea);
    di->sharedArea = -1;
    di->si = NULL;

unlock_and_exit:
    /* mark the device available */
    di->isOpen--;
    /* unlock the driver */
    RELEASE_BEN(pd->kernel);
    ddprintf(("SKD et6000FreeHook() ends.\n"));
    /* all done */
    return B_OK;
}
/*****************************************************************************/
/*
 * et6000ControlHook - where the real work is done
 */
static status_t et6000ControlHook(void* dev, uint32 msg, void *buf, size_t len) {
ET6000DeviceInfo *di = (ET6000DeviceInfo *)dev;
status_t result = B_DEV_INVALID_IOCTL;

    /* ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len)); */
    switch (msg) {
        /* the only PUBLIC ioctl */
        case B_GET_ACCELERANT_SIGNATURE: {
            char *sig = (char *)buf;
            strcpy(sig, "et6000.accelerant");
            result = B_OK;
        } break;

        /* PRIVATE ioctl from here on */
        case ET6000_GET_PRIVATE_DATA: {
            ET6000GetPrivateData *gpd = (ET6000GetPrivateData *)buf;
            if (gpd->magic == ET6000_PRIVATE_DATA_MAGIC) {
                gpd->sharedInfoArea = di->sharedArea;
                result = B_OK;
            }
        } break;

        case ET6000_GET_PCI: {
            ET6000GetSetPCI *gsp = (ET6000GetSetPCI *)buf;
            if (gsp->magic == ET6000_PRIVATE_DATA_MAGIC) {
                pci_info *pcii = &(di->pcii);
                gsp->value = get_pci(gsp->offset, gsp->size);
                result = B_OK;
            }
        } break;

        case ET6000_SET_PCI: {
            ET6000GetSetPCI *gsp = (ET6000GetSetPCI *)buf;
            if (gsp->magic == ET6000_PRIVATE_DATA_MAGIC) {
                pci_info *pcii = &(di->pcii);
                set_pci(gsp->offset, gsp->size, gsp->value);
                result = B_OK;
            }
        } break;

        case ET6000_DEVICE_NAME: {   /* Needed for cloning */
            ET6000DeviceName *dn = (ET6000DeviceName *)buf;
            if(dn->magic == ET6000_PRIVATE_DATA_MAGIC) {
                strncpy(dn->name, di->name, B_OS_NAME_LENGTH);
                result = B_OK;
            }
        } break;

        case ET6000_PROPOSE_DISPLAY_MODE: {
            ET6000DisplayMode *dm = (ET6000DisplayMode *)buf;
            if(dm->magic == ET6000_PRIVATE_DATA_MAGIC) {
                result = et6000ProposeMode(&dm->mode, dm->memSize);
            }
        } break;

        case ET6000_SET_DISPLAY_MODE: {
            ET6000DisplayMode *dm = (ET6000DisplayMode *)buf;
            if(dm->magic == ET6000_PRIVATE_DATA_MAGIC) {
                result = et6000SetMode(&dm->mode, dm->pciConfigSpace);
            }
        } break;
    }
    return result;
}
/*****************************************************************************/
#if DEBUG > 0
static int et6000dump(int argc, char **argv) {
int i;

    kprintf("ET6000 Kernel Driver Persistant Data\n\nThere are %ld card(s)\n", pd->count);
    kprintf("Driver wide benahpore: %ld/%ld\n", pd->kernel.ben, pd->kernel.sem);

    kprintf("Total seen interrupts: %ld\n", pd->total_interrupts);
    for (i = 0; i < pd->count; i++) {
        ET6000DeviceInfo *di = &(pd->di[i]);
        uint16 device_id = di->pcii.device_id;
        ET6000SharedInfo *si = di->si;
        kprintf("  device_id: 0x%04x\n", device_id);
        kprintf("  interrupt count: %ld\n", di->interrupt_count);
        if (si) {
            kprintf("  flags:");
        }
        kprintf("\n");
    }
    return 1; /* the magic number for success */
}
#endif
/*****************************************************************************/

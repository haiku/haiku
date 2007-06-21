/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "DriverInterface.h"

/* this is for the standardized portion of the driver API */
/* currently only one operation is defined: B_GET_ACCELERANT_SIGNATURE */
#include <graphic_driver.h>


#ifdef TRACE_S3SAVAGE
#	define TRACE(a)	TraceLog a
#else
#	define TRACE(a)
#endif


#define get_pci(o, s)	 (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define SKD_HANDLER_INSTALLED 0x80000000

#define MAX_DEVICES	 8

#define DEVICE_FORMAT "%04X_%04X_%02X%02X%02X"

#define SAVAGE_NEWMMIO_REGBASE_S3	0x1000000  /* 16MB */
#define SAVAGE_NEWMMIO_REGBASE_S4	0x0000000
#define SAVAGE_NEWMMIO_REGSIZE		0x0080000	/* 512kb */

/* Tell the kernel what revision of the driver API we support */
int32	api_version = B_CUR_DRIVER_API_VERSION;

/* these structures are private to the kernel driver */

typedef struct {
	uint16	chipID;			// PCI device id of the chipset
	uint16	chipset;		// assigned chipset family identifier
	char*	chipName;		// user recognizable name for chipset (must be < 32 chars)
} ChipInfo;

/* This table maps a PCI device ID to a chipset family identifier and the chipset
   name. */
static ChipInfo SavageChipTable[] = {
	{ 0x8a20, S3_SAVAGE3D,		"Savage3D"				},
	{ 0x8a21, S3_SAVAGE3D,		"Savage3D-MV" 			},
	{ 0x8a22, S3_SAVAGE4,		"Savage4"				},
	{ 0x8a25, S3_PROSAVAGE,		"ProSavage PM133"		},
	{ 0x8a26, S3_PROSAVAGE,		"ProSavage KM133"		},
	{ 0x8c10, S3_SAVAGE_MX,		"Savage/MX-MV"			},
	{ 0x8c11, S3_SAVAGE_MX,		"Savage/MX"				},
	{ 0x8c12, S3_SAVAGE_MX,		"Savage/IX-MV"			},
	{ 0x8c13, S3_SAVAGE_MX,		"Savage/IX"				},
	{ 0x8c22, S3_SUPERSAVAGE,	"SuperSavage/MX 128"	},
	{ 0x8c24, S3_SUPERSAVAGE,	"SuperSavage/MX 64"		},
	{ 0x8c26, S3_SUPERSAVAGE,	"SuperSavage/MX 64C"	},
	{ 0x8c2a, S3_SUPERSAVAGE,	"SuperSavage/IX 128SDR"	},
	{ 0x8c2b, S3_SUPERSAVAGE,	"SuperSavage/IX 128DDR"	},
	{ 0x8c2c, S3_SUPERSAVAGE,	"SuperSavage/IX 64SDR"	},
	{ 0x8c2d, S3_SUPERSAVAGE,	"SuperSavage/IX 64DDR"	},
	{ 0x8c2e, S3_SUPERSAVAGE,	"SuperSavage/IXC 64SDR"	},
	{ 0x8c2f, S3_SUPERSAVAGE,	"SuperSavage/IXC 64DDR"	},
	{ 0x8d01, S3_TWISTER,		"Twister PN133"			},
	{ 0x8d02, S3_TWISTER,		"Twister KN133"			},
	{ 0x8d03, S3_PROSAVAGEDDR,	"ProSavage DDR"			},
	{ 0x8d04, S3_PROSAVAGEDDR,	"ProSavage DDR-K"		},
	{ 0x9102, S3_SAVAGE2000,	"Savage2000"			},
	{ 0,	  0,				NULL }
};


#define VENDOR_ID_SAVAGE 0x5333		/* S3 Savage vendor ID */

static struct {
	uint16		vendorID;
	ChipInfo*	devices;
} SupportedDevices[] = {
	{ VENDOR_ID_SAVAGE, SavageChipTable },
	{ 0x0000, NULL }
};


typedef struct {
	uint32		is_open;		/* a count of how many times the devices has been opened */
	area_id 	sharedArea;		/* the area shared between the driver and all of the accelerants */
	SharedInfo *si;				/* a pointer to the shared area, for convenience */
	vuint8	 	*regs;			/* kernel's pointer to memory mapped registers */
	ChipInfo	*pChipInfo;		/* info about the selected chipset */
	pci_info	pcii;			/* a convenience copy of the pci info for this device */
	char		name[B_OS_NAME_LENGTH]; /* where we keep the name of the device for publishing and comparing */
} DeviceInfo;


typedef struct {
	uint32		count;				/* number of devices actually found */
	benaphore	kernel; 			/* for serializing opens/closes */
	char		*deviceNames[MAX_DEVICES+1];	/* device name pointer storage */
	DeviceInfo	di[MAX_DEVICES];	/* device specific stuff */
} DeviceData;


/* prototypes for our private functions */
static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);
static status_t map_device(DeviceInfo *di);
static void unmap_device(DeviceInfo *di);
static void probe_devices(void);
static int32 savage_interrupt(void *data);

static DeviceData		*pd;
static pci_module_info	*pci_bus;
static device_hooks		graphics_device_hooks =
{
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};

#ifdef TRACE_S3SAVAGE
static void 
TraceLog(const char *fmt, ...)
{
	char string[1024];
	va_list	args;

	strcpy(string, "savage: ");
	va_start(args, fmt);
	vsprintf(&string[strlen(string)], fmt, args);
	dprintf(string);
}
#endif
	

// Functions for dealing with Vertical Blanking Interrupts.  Currently, I do
// not know the commands to handle these operations;  thus, these functions
// currently do nothing.

static bool 
InterruptIsVBI()
{
	// return true only if a vertical blanking interrupt has occured
	return false;
}


static void 
ClearVBI()
{
}

static void 
EnableVBI()
{
}

static void 
DisableVBI()
{
}



static ChipInfo* 
FindDeviceMatch(uint16 vendorID, uint16 deviceID)
{
	// Search the table of supported devices to find a chipset/device that
	// matches the vendor ID and device ID passed by the caller.
	// Return pointer to the struct containing the chipset info if match
	// is found; else return NULL.

	int vendor = 0;

	while (SupportedDevices[vendor].vendorID != 0) {	// end of table?
		if (SupportedDevices[vendor].vendorID == vendorID) {
			ChipInfo* pDevice = SupportedDevices[vendor].devices;

			while (pDevice->chipID != 0) {			// end of table?
				if (pDevice->chipID == deviceID)
					return pDevice;			// matching device/chipset found

				pDevice++;
			}
		}
		vendor++;
	}

	return NULL;		// no match found
}


/*
	init_hardware() - Returns B_OK if a device supported by this driver
	is found; otherwise, returns B_ERROR so the driver will be unloaded.
*/
status_t  
init_hardware(void)
{
	long pci_index = 0;
	pci_info pcii;
	ChipInfo* pDevice = NULL;

	/* choke if we can't find the PCI bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* while there are more pci devices */
	while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR) {
//		TRACE(("init_hardware(): checking pci index %ld, device 0x%04x/0x%04x\n", pci_index, pcii.vendor_id, pcii.device_id));
		pDevice = FindDeviceMatch(pcii.vendor_id, pcii.device_id);
		if (pDevice != NULL)
			break;

		/* next pci_info struct, please */
		pci_index++;
	}

	TRACE(("init_hardware - %s\n", pDevice == NULL ? "no supported devices" : "device supported"));

	/* put away the module manager */
	put_module(B_PCI_MODULE_NAME);
	return (pDevice == NULL ? B_ERROR : B_OK);
}


status_t  init_driver(void)
{
//	TRACE(("enter init_driver\n"));

	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (NULL == pd) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	/* initialize the benaphore */
	INIT_BEN(pd->kernel);

	/* find all of our supported devices */
	probe_devices();

//	TRACE(("exit init_driver\n"));
	return B_OK;
}


const char**
publish_devices(void)
{
	/* return the list of supported devices */
	return (const char **)pd->deviceNames;
}


device_hooks*
find_device(const char *name)
{
	int index = 0;
	while (pd->deviceNames[index]) {
		if (strcmp(name, pd->deviceNames[index]) == 0)
			return &graphics_device_hooks;
		index++;
	}
	return NULL;
}


void
 uninit_driver(void)
{
	/* free the driver data */
	DELETE_BEN(pd->kernel);
	free(pd);
	pd = NULL;

	/* put the pci module away */
	put_module(B_PCI_MODULE_NAME);
}


static status_t 
map_device(DeviceInfo *di)
{
	/* default: frame buffer in [0], control regs in [1] */
	char buffer[B_OS_NAME_LENGTH];
	SharedInfo *si = di->si;
	uint32 tmpUlong;
	pci_info *pcii = &(di->pcii);
	uint32 videoRamAddr = 0;
	uint32 videoRamSize = 0;
	uint32 regsBase = 0;

	TRACE(("enter map_device\n"));

	/* enable memory mapped IO, disable VGA I/O - this is defined in the PCI standard */
	tmpUlong = get_pci(PCI_command, 2);
	tmpUlong |= PCI_command_memory;		// enable PCI access
	tmpUlong |= PCI_command_master;		// enable busmastering
	tmpUlong &= ~PCI_command_io;		// disable ISA I/O access
	set_pci(PCI_command, 2, tmpUlong);

	// Since we do not know at this point the actual size of the video
	// memory, set it to the largest value that the respective chipset
	// family can have.

	if (S3_SAVAGE3D_SERIES(di->pChipInfo->chipset)) {
		regsBase = di->pcii.u.h0.base_registers[0] + SAVAGE_NEWMMIO_REGBASE_S3;
 		videoRamAddr = di->pcii.u.h0.base_registers[0];
		videoRamSize = 16 * 1024 * 1024;	// 16 MB is max for 3D series
		si->videoMemPCI = (void *)(di->pcii.u.h0.base_registers_pci[0]);
	} else {
		regsBase = di->pcii.u.h0.base_registers[0] + SAVAGE_NEWMMIO_REGBASE_S4;
		videoRamAddr = di->pcii.u.h0.base_registers[1];
		videoRamSize = di->pcii.u.h0.base_register_sizes[1];
		si->videoMemPCI = (void *)(di->pcii.u.h0.base_registers_pci[1]);
	}
TRACE(("base_registers 0: 0x%08lX 0x%08lX 0x%08lX\n", di->pcii.u.h0.base_registers[0], di->pcii.u.h0.base_registers_pci[0], di->pcii.u.h0.base_register_sizes[0] ));
TRACE(("base_registers 1: 0x%08lX 0x%08lX 0x%08lX\n", di->pcii.u.h0.base_registers[1], di->pcii.u.h0.base_registers_pci[1], di->pcii.u.h0.base_register_sizes[1] ));

	/* map the areas */
	sprintf(buffer, DEVICE_FORMAT " regs",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	si->regsArea = map_physical_memory(
		buffer,
		(void *)regsBase,
		SAVAGE_NEWMMIO_REGSIZE,
		B_ANY_KERNEL_ADDRESS,
		0,  /* B_READ_AREA + B_WRITE_AREA, */ /* neither read nor write, to hide it from user space apps */
		(void **)(&(di->regs)));

	/* return the error if there was some problem */
	if (si->regsArea < 0)
		return si->regsArea;

	sprintf(buffer, DEVICE_FORMAT " framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	si->videoMemArea = map_physical_memory(
		buffer,
		(void *)videoRamAddr,
		videoRamSize,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		&(si->videoMemAddr));

//	TRACE(("Video memory, area: %ld,  addr: 0x%lX, size: %ld\n", si->videoMemArea, (uint32)(si->videoMemAddr), videoRamSize));

	if (si->videoMemArea < 0) {
		/* try to map this time without write combining */
		si->videoMemArea = map_physical_memory(
			buffer,
			(void *)videoRamAddr,
			videoRamSize,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			&(si->videoMemAddr));
	}

//	TRACE(("Video memory, area: %ld,  addr: 0x%lX\n", si->videoMemArea, (uint32)(si->videoMemAddr)));

	/* if there was an error, delete our other areas */
	if (si->videoMemArea < 0) {
		delete_area(si->regsArea);
		si->regsArea = -1;
	}

	/* in any case, return the result */
	TRACE(("leave map_device\n"));
	return si->videoMemArea;
}


static void 
unmap_device(DeviceInfo *di)
{
	SharedInfo *si = di->si;
	uint32 tmpUlong;
	pci_info *pcii = &(di->pcii);

	TRACE(("enter unmap_device(%08lx)\n", (uint32)di));
//	TRACE(("regsArea: %ld\n\tvideoMemArea: %ld\n", si->regsArea, si->videoMemArea));

	/* disable memory mapped IO */
	tmpUlong = get_pci(PCI_command, 2);
	tmpUlong &= ~(PCI_command_io | PCI_command_memory);
	set_pci(PCI_command, 2, tmpUlong);

	/* delete the areas */
	if (si->regsArea >= 0)
		delete_area(si->regsArea);
	if (si->videoMemArea >= 0)
		delete_area(si->videoMemArea);

	si->regsArea = si->videoMemArea = -1;
	si->videoMemAddr = NULL;
	di->regs = NULL;

	TRACE(("exit unmap_device()\n"));
}


static void 
probe_devices(void)
{
	uint32 pci_index = 0;
	uint32 count = 0;
	DeviceInfo *di = pd->di;
	ChipInfo* pDevice;

//	TRACE(("enter probe_devices()\n"));

	/* while there are more pci devices */
	while ((count < MAX_DEVICES) && ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR)) {
//		TRACE(("checking pci index %ld, device 0x%04x/0x%04x\n", pci_index, di->pcii.vendor_id, di->pcii.device_id));

		/* if we match a supported vendor & device */
		pDevice = FindDeviceMatch(di->pcii.vendor_id, di->pcii.device_id);
		if (pDevice != NULL) {
			/* publish the device name */
			sprintf(di->name, "graphics/" DEVICE_FORMAT,
					  di->pcii.vendor_id, di->pcii.device_id,
					  di->pcii.bus, di->pcii.device, di->pcii.function);
			TRACE(("probe_devices: match found; name: %s\n", di->name));

			pd->deviceNames[count] = di->name;
			di->is_open = 0;			// mark driver as available for R/W open
			di->sharedArea = -1;		// indicate shared area not yet created
			di->si = NULL;
			di->pChipInfo = pDevice;
			di++;
			count++;
		}

		pci_index++;
	}

	pd->count = count;
	pd->deviceNames[pd->count] = NULL;	// terminate list with null pointer

	TRACE(("probe_devices: %ld supported devices\n", pd->count));
}


static uint32 
thread_interrupt_work(DeviceInfo *di)
{
	SharedInfo *si = di->si;
	uint32 handled = B_HANDLED_INTERRUPT;

	/* release the vblank semaphore */
	if (si->vblank >= 0) {
		int32 blocked;
		if ((get_sem_count(si->vblank, &blocked) == B_OK) && (blocked < 0)) {
			release_sem_etc(si->vblank, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}
	return handled;
}


static int32 
savage_interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	DeviceInfo *di = (DeviceInfo *)data;
	int32 *flags = &(di->si->flags);

	/* is someone already handling an interrupt for this device? */
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED)
		goto exit0;

	if (InterruptIsVBI()) {	// was the interrupt a VBI?
		ClearVBI();			// clear the interrupt

		handled = thread_interrupt_work(di);	// release the semaphore
	}

	/* note that we're not in the handler any more */
	atomic_and(flags, ~SKD_HANDLER_INSTALLED);

exit0:
	return handled;
}


//	#pragma mark - Device Hooks


static status_t 
open_hook(const char* name, uint32 flags, void** cookie)
{
	int32 index = 0;
	DeviceInfo *di;
	SharedInfo *si;
	thread_id	thid;
	thread_info thinfo;
	status_t	result = B_OK;
	char sharedName[B_OS_NAME_LENGTH];

	(void)flags;		// avoid compiler warning for unused arg

	TRACE(("open_hook(%s, 0x%08lx)\n", name, (uint32)cookie));

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	while (pd->deviceNames[index] && (strcmp(name, pd->deviceNames[index]) != 0))
		index++;

	/* for convienience */
	di = &(pd->di[index]);

	/* make sure no one else has write access to the common data */
	AQUIRE_BEN(pd->kernel);

	/* if it's already open for writing */
	if (di->is_open) {
		/* mark it open another time */
		goto mark_as_open;
	}

	/* create the shared area */
	sprintf(sharedName, DEVICE_FORMAT " shared",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	/* create this area with NO user-space read or write permissions, to prevent accidental dammage */
	di->sharedArea = create_area(sharedName, (void **) &(di->si), B_ANY_KERNEL_ADDRESS,
		((sizeof(SharedInfo) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)),
		B_FULL_LOCK, 0);
	if (di->sharedArea < 0) {
		/* return the error */
		result = di->sharedArea;
		goto done;
	}

	/* save a few dereferences */
	si = di->si;

	/* save the vendor and device IDs */
	si->vendorID = di->pcii.vendor_id;
	si->deviceID = di->pcii.device_id;
	si->revision = di->pcii.revision;
	si->chipset = di->pChipInfo->chipset;
	strcpy(si->chipsetName, di->pChipInfo->chipName);

	/* map the device */
	result = map_device(di);
	if (result < 0)
		goto free_shared;

	result = B_OK;

	DisableVBI();					// disable & clear any pending interrupts
	si->bInterruptAssigned = false;	// indicate interrupt not assigned yet

	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, di->name);
	if (si->vblank < 0)
		goto mark_as_open;

	/* change the owner of the semaphores to the opener's team */
	/* this is required because apps can't aquire kernel semaphores */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);

	/* If there is a valid interrupt assigned, set up interrupts */
	if ((di->pcii.u.h0.interrupt_pin == 0x00) ||
		(di->pcii.u.h0.interrupt_line == 0xff) ||	/* no IRQ assigned */
		(di->pcii.u.h0.interrupt_line <= 0x02)) {	/* system IRQ assigned */
		/* Interrupt does not exist; thus delete semaphore as it won't be used */
		delete_sem(si->vblank);
		si->vblank = -1;
	} else {
		/* otherwise install our interrupt handler */
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, savage_interrupt, (void *)di, 0);
		if (result != B_OK) {
			/* delete the semaphore as it won't be used */
			delete_sem(si->vblank);
			si->vblank = -1;
		} else {
			/* inform accelerant(s) we can use interrupt related functions */
			si->bInterruptAssigned = true;
		}
	}

mark_as_open:
	/* mark the device open */
	di->is_open++;

	/* send the cookie to the opener */
	*cookie = di;
	goto done;

free_shared:
	/* clean up our shared area */
	delete_area(di->sharedArea);
	di->sharedArea = -1;
	di->si = NULL;

done:
	/* end of critical section */
	RELEASE_BEN(pd->kernel);

	/* all done, return the status */
	TRACE(("open_hook returning 0x%08lx\n", result));
	return result;
}


static status_t 
read_hook(void* dev, off_t pos, void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter" warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t 
write_hook(void* dev, off_t pos, const void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter" warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t 
close_hook(void* dev)
{
	(void)dev;		// avoid compiler warning for unused arg

	TRACE(("close_hook()\n"));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}


static status_t 
free_hook(void* dev)
{
	DeviceInfo *di = (DeviceInfo*)dev;
	SharedInfo *si = di->si;

	TRACE(("enter free_hook()\n"));
	/* lock the driver */
	AQUIRE_BEN(pd->kernel);

	/* if opened multiple times, decrement the open count and exit */
	if (di->is_open > 1)
		goto unlock_and_exit;

	DisableVBI();		// disable & clear any pending interrupts

	if (si->bInterruptAssigned) {
		/* remove interrupt handler */
		remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, savage_interrupt, di);
	}

	/* delete the semaphores, ignoring any errors ('cause the owning team may have died on us) */
	if (si->vblank >= 0)
		delete_sem(si->vblank);
	si->vblank = -1;

	/* free regs and frame buffer areas */
	unmap_device(di);

	/* clean up our shared area */
	delete_area(di->sharedArea);
	di->sharedArea = -1;
	di->si = NULL;

unlock_and_exit:
	/* mark the device available */
	di->is_open--;
	/* unlock the driver */
	RELEASE_BEN(pd->kernel);

	TRACE(("exit free_hook()\n"));
	/* all done */
	return B_OK;
}


static status_t 
control_hook(void* dev, uint32 msg, void *buf, size_t len)
{
	DeviceInfo *di = (DeviceInfo *)dev;
	status_t result = B_DEV_INVALID_IOCTL;

	(void)len;		// avoid compiler warning for unused arg

	// TRACE(("control_hook; ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));

	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE:
		{
			char *sig = (char *)buf;
			strcpy(sig, "s3savage.accelerant");
			result = B_OK;
			break;
		}

		/* PRIVATE ioctl from here on */
		case SAVAGE_GET_PRIVATE_DATA:
		{
			SavageGetPrivateData *gpd = (SavageGetPrivateData *)buf;
			if (gpd->magic == SAVAGE_PRIVATE_DATA_MAGIC) {
				gpd->sharedInfoArea = di->sharedArea;
				result = B_OK;
			}
			break;
		}

		case SAVAGE_GET_PCI:
		{
			SavageGetSetPci *gsp = (SavageGetSetPci *)buf;
			if (gsp->magic == SAVAGE_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
			break;
		}

		case SAVAGE_SET_PCI:
		{
			SavageGetSetPci *gsp = (SavageGetSetPci *)buf;
			if (gsp->magic == SAVAGE_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
			break;
		}

		case SAVAGE_DEVICE_NAME:
		{
			SavageDeviceName *dn = (SavageDeviceName *)buf;
			if (dn->magic == SAVAGE_PRIVATE_DATA_MAGIC) {
				strcpy(dn->name, di->name);
				result = B_OK;
			}
			break;
		}

		case SAVAGE_RUN_INTERRUPTS:
		{
			SavageSetBoolState *ri = (SavageSetBoolState *)buf;
			if (ri->magic == SAVAGE_PRIVATE_DATA_MAGIC) {
				if (ri->bEnable) {
					EnableVBI();
				} else {
					DisableVBI();
				}
			}
			result = B_OK;
			break;
		}
	}
	return result;
}


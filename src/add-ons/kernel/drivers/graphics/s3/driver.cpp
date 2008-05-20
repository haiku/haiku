/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2007-2008
*/

#include <KernelExport.h>
#include <PCI.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <graphic_driver.h>

#include "DriverInterface.h"


#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
#	define TRACE(x...) dprintf("S3: " x)
#else
#	define TRACE(x...) ;
#endif


#define get_pci(o, s)	 (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define SKD_HANDLER_INSTALLED 0x80000000
#define MAX_DEVICES		8
#define DEVICE_FORMAT	"%04X_%04X_%02X%02X%02X"

int32 api_version = B_CUR_DRIVER_API_VERSION;	// revision of driver API we support


struct ChipInfo {
	uint16	chipID;			// PCI device id of the chip
	uint16	chipType;		// assigned chip type identifier
	char*	chipName;		// user recognizable name for chip (must be < 32 chars)
};

// This table maps a PCI device ID to a chip type identifier and the chip name.
// Note that the Trio64 and Trio64V+ chips have the same ID, but have a different
// revision number.  After the revision number is examined, the Trio64V+ will
// have a different chip type code and name assigned.

static const ChipInfo S3_ChipTable[] = {
	{ 0x8811, S3_TRIO64,		"Trio64"			},		// see comment above
	{ 0x8814, S3_TRIO64_UVP,	"Trio64 UV+"		},
	{ 0x8901, S3_TRIO64_V2,		"Trio64 V2/DX/GX"	},

	{ 0x5631, S3_VIRGE,			"Virge"				},
	{ 0x883D, S3_VIRGE_VX,		"Virge VX"			},
	{ 0x8A01, S3_VIRGE_DXGX,	"Virge DX/GX"		},
	{ 0x8A10, S3_VIRGE_GX2,		"Virge GX2"			},
	{ 0x8C01, S3_VIRGE_MX,		"Virge MX"			},
	{ 0x8C03, S3_VIRGE_MXP,		"Virge MX+"			},
	{ 0x8904, S3_TRIO_3D,		"Trio 3D"			},
	{ 0x8A13, S3_TRIO_3D_2X,	"Trio 3D/2X"		},

	{ 0x8a20, S3_SAVAGE_3D,		"Savage3D"				},
	{ 0x8a21, S3_SAVAGE_3D,		"Savage3D-MV" 			},
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
	{ 0x8d03, S3_PROSAVAGE_DDR,	"ProSavage DDR"			},
	{ 0x8d04, S3_PROSAVAGE_DDR,	"ProSavage DDR-K"		},
	{ 0x9102, S3_SAVAGE2000,	"Savage2000"			},
	{ 0,	  0,				NULL				}
};


#define VENDOR_ID	0x5333		// S3 vendor ID

static struct {
	uint16			vendorID;
	const ChipInfo*	devices;
} SupportedDevices[] = {
	{ VENDOR_ID, S3_ChipTable },
	{ 0x0000, NULL }
};


struct DeviceInfo {
	uint32			openCount;		// count of how many times device has been opened
	int32			flags;
	area_id 		sharedArea;		// area shared between driver and all accelerants
	SharedInfo* 	si;				// pointer to the shared area
	vuint8*	 		regs;			// kernel's pointer to memory mapped registers
	const ChipInfo*	pChipInfo;		// info about the selected chip
	pci_info		pcii;			// copy of pci info for this device
	char			name[B_OS_NAME_LENGTH]; // name of device
};


struct DeviceData {
	uint32		count;				// number of devices actually found
	benaphore	kernel; 			// for serializing opens/closes
	char*		deviceNames[MAX_DEVICES + 1];	// device name pointer storage
	DeviceInfo	di[MAX_DEVICES];	// device specific stuff
};


// Prototypes for our private functions.

static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void* buf, size_t len);
static status_t map_device(DeviceInfo* di);
static void unmap_device(DeviceInfo* di);
static void probe_devices(void);
static int32 s3_interrupt(void* data);

static DeviceData*		pd;
static pci_module_info*	pci_bus;
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



static const ChipInfo* 
FindDeviceMatch(uint16 vendorID, uint16 deviceID)
{
	// Search the table of supported devices to find a chip/device that
	// matches the vendor ID and device ID passed by the caller.
	// Return pointer to the struct containing the chip info if match
	// is found; else return NULL.

	int vendor = 0;

	while (SupportedDevices[vendor].vendorID != 0) {	// end of table?
		if (SupportedDevices[vendor].vendorID == vendorID) {
			const ChipInfo* pDevice = SupportedDevices[vendor].devices;

			while (pDevice->chipID != 0) {			// end of table?
				if (pDevice->chipID == deviceID)
					return pDevice;			// matching device/chip found

				pDevice++;
			}
		}
		vendor++;
	}

	return NULL;		// no match found
}


status_t  
init_hardware(void)
{
	// Return B_OK if a device supported by this driver is found; otherwise,
	// return B_ERROR so the driver will be unloaded.

	long pci_index = 0;
	pci_info pcii;
	const ChipInfo* pDevice = NULL;

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&pci_bus) != B_OK)
		return B_ERROR;		// unable to access PCI bus

	// Check all pci devices for a device supported by this driver.

	while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR) {
		pDevice = FindDeviceMatch(pcii.vendor_id, pcii.device_id);
		if (pDevice != NULL)
			break;

		pci_index++;
	}

	TRACE("init_hardware() - %s\n", pDevice == NULL ? "no supported devices" : "device supported");

	put_module(B_PCI_MODULE_NAME);		// put away the module manager
	return (pDevice == NULL ? B_ERROR : B_OK);
}


status_t  init_driver(void)
{
	// Get handle for the pci bus.

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&pci_bus) != B_OK)
		return B_ERROR;

	pd = (DeviceData*)calloc(1, sizeof(DeviceData));
	if (NULL == pd) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	INIT_BEN(pd->kernel);	// initialize the benaphore
	probe_devices();		// find all devices supported by this driver

	return B_OK;
}


const char**
publish_devices(void)
{
	return (const char**)pd->deviceNames;	// return list of supported devices
}


device_hooks*
find_device(const char* name)
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
	// Free the driver data.

	DELETE_BEN(pd->kernel);
	free(pd);
	pd = NULL;

	put_module(B_PCI_MODULE_NAME);	// put the pci module away
}


static status_t 
map_device(DeviceInfo* di)
{
	char areaName[B_OS_NAME_LENGTH];
	SharedInfo* si = di->si;
	pci_info* pcii = &(di->pcii);

	TRACE("enter map_device()\n");

	// enable memory mapped IO and VGA I/O

	uint32 tmpUlong = get_pci(PCI_command, 2);
	tmpUlong |= PCI_command_io | PCI_command_memory | PCI_command_master;
	set_pci(PCI_command, 2, tmpUlong);

	const uint32 SavageMmioRegBaseOld	= 0x1000000;	// 16 MB
	const uint32 SavageMmioRegBaseNew	= 0x0000000;
	const uint32 SavageMmioRegSize		= 0x0080000;	// 512 KB reg area size

	const uint32 VirgeMmioRegBase		= 0x1000000;	// 16 MB
	const uint32 VirgeMmioRegSize		= 0x10000;		// 64 KB reg area size

	uint32 videoRamAddr = 0;
	uint32 videoRamSize = 0;
	uint32 regsBase = 0;
	uint32 regAreaSize = 0;

	// Since we do not know at this point the actual size of the video
	// memory, set it to the largest value that the respective chipset
	// family can have.

	if (S3_SAVAGE_FAMILY(di->pChipInfo->chipType)) {
		if (S3_SAVAGE_3D_SERIES(di->pChipInfo->chipType)) {
			// Savage 3D & Savage MX chips.

			regsBase = di->pcii.u.h0.base_registers[0] + SavageMmioRegBaseOld;
			regAreaSize = SavageMmioRegSize;

	 		videoRamAddr = di->pcii.u.h0.base_registers[0];
			videoRamSize = 16 * 1024 * 1024;	// 16 MB is max for 3D series
			si->videoMemPCI = (void *)(di->pcii.u.h0.base_registers_pci[0]);
		} else {
			// All other Savage chips.

			regsBase = di->pcii.u.h0.base_registers[0] + SavageMmioRegBaseNew;
			regAreaSize = SavageMmioRegSize;

			videoRamAddr = di->pcii.u.h0.base_registers[1];
			videoRamSize = di->pcii.u.h0.base_register_sizes[1];
			si->videoMemPCI = (void *)(di->pcii.u.h0.base_registers_pci[1]);
		}
	} else {
		// Trio/Virge chips.

		regsBase = di->pcii.u.h0.base_registers[0] + VirgeMmioRegBase;
		regAreaSize = VirgeMmioRegSize;

 		videoRamAddr = di->pcii.u.h0.base_registers[0];
		videoRamSize = 8 * 1024 * 1024;	// 8 MB is max for Trio/Virge chips
		si->videoMemPCI = (void *)(di->pcii.u.h0.base_registers_pci[0]);
	}

	// Map the MMIO register area.

	sprintf(areaName, DEVICE_FORMAT " regs",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	si->regsArea = map_physical_memory(areaName, (void*)regsBase, regAreaSize,
		B_ANY_KERNEL_ADDRESS,
		0,		// neither read nor write, to hide it from user space apps
		(void**)(&(di->regs)));

	if (si->regsArea < 0)
		return si->regsArea;	// return error code

	// Map the video memory.

	sprintf(areaName, DEVICE_FORMAT " framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	si->videoMemArea = map_physical_memory(
		areaName,
		(void*)videoRamAddr,
		videoRamSize,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		&(si->videoMemAddr));

	if (si->videoMemArea < 0) {
		// Try to map this time without write combining.
		si->videoMemArea = map_physical_memory(
			areaName,
			(void*)videoRamAddr,
			videoRamSize,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			&(si->videoMemAddr));
	}

	TRACE("Video memory, area: %ld,  addr: 0x%lX, size: %ld\n", si->videoMemArea,
		(uint32)(si->videoMemAddr), videoRamSize);

	// If there was an error, delete other areas.
	if (si->videoMemArea < 0) {
		delete_area(si->regsArea);
		si->regsArea = -1;
	}

	TRACE("leave map_device(); result: %ld\n", si->videoMemArea);
	return si->videoMemArea;
}


static void 
unmap_device(DeviceInfo* di)
{
	SharedInfo* si = di->si;
	pci_info* pcii = &(di->pcii);

	TRACE("enter unmap_device()\n");

	// Disable memory mapped IO.
	uint32 tmpUlong = get_pci(PCI_command, 2);
	tmpUlong &= ~(PCI_command_io | PCI_command_memory);
	set_pci(PCI_command, 2, tmpUlong);

	if (si->regsArea >= 0)
		delete_area(si->regsArea);
	if (si->videoMemArea >= 0)
		delete_area(si->videoMemArea);

	si->regsArea = si->videoMemArea = -1;
	si->videoMemAddr = NULL;
	di->regs = NULL;

	TRACE("exit unmap_device()\n");
}


static void 
probe_devices(void)
{
	uint32 pci_index = 0;
	uint32 count = 0;
	DeviceInfo* di = pd->di;
	const ChipInfo* pDevice;

	while ((count < MAX_DEVICES)
			&& ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR)) {

		pDevice = FindDeviceMatch(di->pcii.vendor_id, di->pcii.device_id);
		if (pDevice != NULL) {
			// Compose device name.
			sprintf(di->name, "graphics/" DEVICE_FORMAT,
					  di->pcii.vendor_id, di->pcii.device_id,
					  di->pcii.bus, di->pcii.device, di->pcii.function);
			TRACE("probe_devices() match found; name: %s\n", di->name);

			pd->deviceNames[count] = di->name;
			di->openCount = 0;			// mark driver as available for R/W open
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

	TRACE("probe_devices() %ld supported devices\n", pd->count);
}


static uint32 
thread_interrupt_work(DeviceInfo* di)
{
	SharedInfo* si = di->si;
	uint32 handled = B_HANDLED_INTERRUPT;

	// Release vertical blanking semaphore.
	if (si->vertBlankSem >= 0) {
		int32 blocked;
		if ((get_sem_count(si->vertBlankSem, &blocked) == B_OK) && (blocked < 0)) {
			release_sem_etc(si->vertBlankSem, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}
	return handled;
}


static int32 
s3_interrupt(void* data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	DeviceInfo* di = (DeviceInfo*)data;
	int32* flags = &(di->flags);

	// Is someone already handling an interrupt for this device?
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED)
		return B_UNHANDLED_INTERRUPT;

	if (InterruptIsVBI()) {	// was interrupt a VBI?
		ClearVBI();			// clear interrupt

		handled = thread_interrupt_work(di);	// release semaphore
	}

	atomic_and(flags, ~SKD_HANDLER_INSTALLED);	// note we're not in handler anymore

	return handled;
}


//	#pragma mark - Device Hooks


static status_t 
open_hook(const char* name, uint32 flags, void** cookie)
{
	int32 index = 0;
	SharedInfo* si;
	thread_id	thid;
	thread_info thinfo;
	status_t	result = B_OK;
	char sharedName[B_OS_NAME_LENGTH];

	(void)flags;		// avoid compiler warning for unused arg

	TRACE("open_hook() - name: %s, cookie: 0x%08lx)\n", name, (uint32)cookie);

	// Find the device name in the list of devices.
	while (pd->deviceNames[index] && (strcmp(name, pd->deviceNames[index]) != 0))
		index++;

	DeviceInfo* di = &(pd->di[index]);

	// Make sure no one else has write access to the common data.
	AQUIRE_BEN(pd->kernel);

	if (di->openCount > 0)
		goto mark_as_open;

	// Create the area for shared info with NO user-space read or write permissions,
	// to prevent accidental damage.

	sprintf(sharedName, DEVICE_FORMAT " shared",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	di->sharedArea = create_area(sharedName, (void**) &(di->si), B_ANY_KERNEL_ADDRESS,
		((sizeof(SharedInfo) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)),
		B_FULL_LOCK, 0);
	if (di->sharedArea < 0) {
		result = di->sharedArea;	// return error
		goto done;
	}

	si = di->si;

	si->vendorID = di->pcii.vendor_id;
	si->deviceID = di->pcii.device_id;
	si->revision = di->pcii.revision;
	si->chipType = di->pChipInfo->chipType;
	strcpy(si->chipName, di->pChipInfo->chipName);

	// Trio64 and Trio64V+ chips have the same ID but different revision numbers.
	// Since the Trio64V+ supports MMIO, better performance can be obtained
	// from it if it is distinguished from the Trio64.

	if (si->chipType == S3_TRIO64 && si->revision & 0x40) {
		si->chipType = S3_TRIO64_VP;
		strcpy(si->chipName, "Trio64 V+");
	}


	result = map_device(di);
	if (result < 0)
		goto free_shared;

	result = B_OK;

	DisableVBI();					// disable & clear any pending interrupts
	si->bInterruptAssigned = false;	// indicate interrupt not assigned yet

	// Create a semaphore for vertical blank management.
	si->vertBlankSem = create_sem(0, di->name);
	if (si->vertBlankSem < 0)
		goto mark_as_open;

	// Change the owner of the semaphores to the opener's team.
	// This is required because apps can't aquire kernel semaphores.
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vertBlankSem, thinfo.team);

	// If there is a valid interrupt assigned, set up interrupts.

	if ((di->pcii.u.h0.interrupt_pin == 0x00) ||
		(di->pcii.u.h0.interrupt_line == 0xff) ||	// no IRQ assigned
		(di->pcii.u.h0.interrupt_line <= 0x02)) {	// system IRQ assigned
		// Interrupt does not exist; thus delete semaphore as it won't be used.
		delete_sem(si->vertBlankSem);
		si->vertBlankSem = -1;
	} else {
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, s3_interrupt, (void*)di, 0);
		if (result != B_OK) {
			// Delete semaphore as it won't be used.
			delete_sem(si->vertBlankSem);
			si->vertBlankSem = -1;
		} else {
			// Inform accelerant(s) we can use interrupt related functions.
			si->bInterruptAssigned = true;
		}
	}

mark_as_open:
	di->openCount++;	// mark device open
	*cookie = di;		// send cookie to opener
	goto done;

free_shared:
	delete_area(di->sharedArea);
	di->sharedArea = -1;
	di->si = NULL;

done:
	// End of critical section.
	RELEASE_BEN(pd->kernel);

	TRACE("open_hook() returning 0x%08lx\n", result);
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

	TRACE("close_hook()\n");
	return B_NO_ERROR;
}


static status_t 
free_hook(void* dev)
{
	DeviceInfo* di = (DeviceInfo*)dev;
	SharedInfo* si = di->si;

	TRACE("enter free_hook()\n");

	AQUIRE_BEN(pd->kernel);		// lock driver

	// If opened multiple times, decrement the open count and exit.
	if (di->openCount > 1)
		goto unlock_and_exit;

	DisableVBI();		// disable & clear any pending interrupts

	if (si->bInterruptAssigned) {
		remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, s3_interrupt, di);
	}

	// Delete the semaphores, ignoring any errors because the owning team may have died.
	if (si->vertBlankSem >= 0)
		delete_sem(si->vertBlankSem);
	si->vertBlankSem = -1;

	unmap_device(di);	// free regs and frame buffer areas

	delete_area(di->sharedArea);
	di->sharedArea = -1;
	di->si = NULL;

unlock_and_exit:
	if (di->openCount > 0)
		di->openCount--;		// mark device available
		
	RELEASE_BEN(pd->kernel);	// unlock driver

	TRACE("exit free_hook()\n");
	return B_OK;
}


static status_t 
control_hook(void* dev, uint32 msg, void* buf, size_t len)
{
	DeviceInfo* di = (DeviceInfo*)dev;

	(void)len;		// avoid compiler warning for unused arg

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char*)buf, "s3.accelerant");
			return B_OK;

		case S3_DEVICE_NAME:
			strncpy((char*)buf, di->name, B_OS_NAME_LENGTH);
			((char*)buf)[B_OS_NAME_LENGTH -1] = '\0';
			return B_OK;

		case S3_GET_PRIVATE_DATA:
		{
			S3GetPrivateData* gpd = (S3GetPrivateData*)buf;
			if (gpd->magic == S3_PRIVATE_DATA_MAGIC) {
				gpd->sharedInfoArea = di->sharedArea;
				return B_OK;
			}
			break;
		}

		case S3_RUN_INTERRUPTS:
		{
			S3SetBoolState* ri = (S3SetBoolState*)buf;
			if (ri->magic == S3_PRIVATE_DATA_MAGIC) {
				if (ri->bEnable)
					EnableVBI();
				else
					DisableVBI();
			}
			return B_OK;
		}

		case S3_GET_PIO:
		{
			S3GetSetPIO* gsp = (S3GetSetPIO*)buf;
			if (gsp->magic == S3_PRIVATE_DATA_MAGIC) {
				switch (gsp->size) {
					case 1:
						gsp->value = pci_bus->read_io_8(gsp->offset);
						break;
					case 2:
						gsp->value = pci_bus->read_io_16(gsp->offset);
						break;
					case 4:
						gsp->value = pci_bus->read_io_32(gsp->offset);
						break;
					default:
						TRACE("control_hook() S3_GET_PIO invalid size: %ld\n", gsp->size);
						return B_ERROR;
				}
				return B_OK;
			}
			break;
		}

		case S3_SET_PIO:
		{
			S3GetSetPIO* gsp = (S3GetSetPIO*)buf;
			if (gsp->magic == S3_PRIVATE_DATA_MAGIC) {
				switch (gsp->size) {
					case 1:
						pci_bus->write_io_8(gsp->offset, gsp->value);
						break;
					case 2:
						pci_bus->write_io_16(gsp->offset, gsp->value);
						break;
					case 4:
						pci_bus->write_io_32(gsp->offset, gsp->value);
						break;
					default:
						TRACE("control_hook() S3_SET_PIO invalid size: %ld\n", gsp->size);
						return B_ERROR;
				}
				return B_OK;
			}
			break;
		}
	}

	return B_DEV_INVALID_IOCTL;
}


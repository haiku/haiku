/*
	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#include <KernelExport.h>
#include <PCI.h>
#ifdef __HAIKU__
#include <drivers/bios.h>
#endif	// __HAIKU__
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



#define SKD_HANDLER_INSTALLED 0x80000000
#define MAX_DEVICES		4
#define DEVICE_FORMAT	"%04X_%04X_%02X%02X%02X"

int32 api_version = B_CUR_DRIVER_API_VERSION;	// revision of driver API we support

#define VENDOR_ID 0x5333	// S3 vendor ID


struct ChipInfo {
	uint16		chipID;			// PCI device id of the chip
	uint16		chipType;		// assigned chip type identifier
	const char*	chipName;		// user recognizable name for chip (must be < 32
								// chars)
};

// This table maps a PCI device ID to a chip type identifier and the chip name.
// Note that the Trio64 and Trio64V+ chips have the same ID, but have a different
// revision number.  After the revision number is examined, the Trio64V+ will
// have a different chip type code and name assigned.

static const ChipInfo chipTable[] = {
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


struct DeviceInfo {
	uint32			openCount;		// count of how many times device has been opened
	int32			flags;
	area_id 		sharedArea;		// area shared between driver and all accelerants
	SharedInfo* 	sharedInfo;				// pointer to shared info area memory
	vuint8*	 		regs;			// pointer to memory mapped registers
	const ChipInfo*	pChipInfo;		// info about the selected chip
	pci_info		pciInfo;		// copy of pci info for this device
	char			name[B_OS_NAME_LENGTH]; // name of device
};


static Benaphore		gLock;
static DeviceInfo		gDeviceInfo[MAX_DEVICES];
static char*			gDeviceNames[MAX_DEVICES + 1];
static pci_module_info*	gPCI;


// Prototypes for device hook functions.

static status_t device_open(const char* name, uint32 flags, void** cookie);
static status_t device_close(void* dev);
static status_t device_free(void* dev);
static status_t device_read(void* dev, off_t pos, void* buf, size_t* len);
static status_t device_write(void* dev, off_t pos, const void* buf, size_t* len);
static status_t device_ioctl(void* dev, uint32 msg, void* buf, size_t len);

static device_hooks gDeviceHooks =
{
	device_open,
	device_close,
	device_free,
	device_ioctl,
	device_read,
	device_write,
	NULL,
	NULL,
	NULL,
	NULL
};



static inline uint32
GetPCI(pci_info& info, uint8 offset, uint8 size)
{
	return gPCI->read_pci_config(info.bus, info.device, info.function, offset, size);
}


static inline void
SetPCI(pci_info& info, uint8 offset, uint8 size, uint32 value)
{
	gPCI->write_pci_config(info.bus, info.device, info.function, offset, size, value);
}


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


static status_t
MapDevice(DeviceInfo& di)
{
	char areaName[B_OS_NAME_LENGTH];
	SharedInfo& si = *(di.sharedInfo);
	pci_info& pciInfo = di.pciInfo;

	TRACE("enter MapDevice()\n");

	// Enable memory mapped IO and bus master.

	SetPCI(pciInfo, PCI_command, 2, GetPCI(pciInfo, PCI_command, 2)
		| PCI_command_io | PCI_command_memory | PCI_command_master);

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

	if (S3_SAVAGE_FAMILY(di.pChipInfo->chipType)) {
		if (S3_SAVAGE_3D_SERIES(di.pChipInfo->chipType)) {
			// Savage 3D & Savage MX chips.

			regsBase = pciInfo.u.h0.base_registers[0] + SavageMmioRegBaseOld;
			regAreaSize = SavageMmioRegSize;

	 		videoRamAddr = pciInfo.u.h0.base_registers[0];
			videoRamSize = 16 * 1024 * 1024;	// 16 MB is max for 3D series
			si.videoMemPCI = (void *)(pciInfo.u.h0.base_registers_pci[0]);
		} else {
			// All other Savage chips.

			regsBase = pciInfo.u.h0.base_registers[0] + SavageMmioRegBaseNew;
			regAreaSize = SavageMmioRegSize;

			videoRamAddr = pciInfo.u.h0.base_registers[1];
			videoRamSize = pciInfo.u.h0.base_register_sizes[1];
			si.videoMemPCI = (void *)(pciInfo.u.h0.base_registers_pci[1]);
		}
	} else {
		// Trio/Virge chips.

		regsBase = pciInfo.u.h0.base_registers[0] + VirgeMmioRegBase;
		regAreaSize = VirgeMmioRegSize;

 		videoRamAddr = pciInfo.u.h0.base_registers[0];
		videoRamSize = 8 * 1024 * 1024;	// 8 MB is max for Trio/Virge chips
		si.videoMemPCI = (void *)(pciInfo.u.h0.base_registers_pci[0]);
	}

	// Map the MMIO register area.

	sprintf(areaName, DEVICE_FORMAT " regs",
		pciInfo.vendor_id, pciInfo.device_id,
		pciInfo.bus, pciInfo.device, pciInfo.function);

	si.regsArea = map_physical_memory(areaName, regsBase, regAreaSize,
		B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA,
		(void**)(&(di.regs)));

	if (si.regsArea < 0)
		return si.regsArea;	// return error code

	// Map the video memory.

	sprintf(areaName, DEVICE_FORMAT " framebuffer",
		pciInfo.vendor_id, pciInfo.device_id,
		pciInfo.bus, pciInfo.device, pciInfo.function);

	si.videoMemArea = map_physical_memory(
		areaName,
		videoRamAddr,
		videoRamSize,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		&(si.videoMemAddr));

	if (si.videoMemArea < 0) {
		// Try to map this time without write combining.
		si.videoMemArea = map_physical_memory(
			areaName,
			videoRamAddr,
			videoRamSize,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			&(si.videoMemAddr));
	}

	TRACE("Video memory, area: %ld,  addr: 0x%" B_PRIXADDR "\n",
		si.videoMemArea, (addr_t)(si.videoMemAddr));

	// If there was an error, delete other areas.
	if (si.videoMemArea < 0) {
		delete_area(si.regsArea);
		si.regsArea = -1;
	}

	TRACE("leave MapDevice(); result: %ld\n", si.videoMemArea);
	return si.videoMemArea;
}


static void
UnmapDevice(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);

	TRACE("enter UnmapDevice()\n");

	if (si.regsArea >= 0)
		delete_area(si.regsArea);
	if (si.videoMemArea >= 0)
		delete_area(si.videoMemArea);

	si.regsArea = si.videoMemArea = -1;
	si.videoMemAddr = NULL;
	di.regs = NULL;

	TRACE("exit UnmapDevice()\n");
}


static int32
InterruptHandler(void* data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	DeviceInfo& di = *((DeviceInfo*)data);
	int32* flags = &(di.flags);

	// Is someone already handling an interrupt for this device?
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED)
		return B_UNHANDLED_INTERRUPT;

	if (InterruptIsVBI()) {	// was interrupt a VBI?
		ClearVBI();			// clear interrupt

		handled = B_HANDLED_INTERRUPT;

		// Release vertical blanking semaphore.
		sem_id& sem = di.sharedInfo->vertBlankSem;

		if (sem >= 0) {
			int32 blocked;
			if ((get_sem_count(sem, &blocked) == B_OK) && (blocked < 0)) {
				release_sem_etc(sem, -blocked, B_DO_NOT_RESCHEDULE);
				handled = B_INVOKE_SCHEDULER;
			}
		}
	}

	atomic_and(flags, ~SKD_HANDLER_INSTALLED);	// note we're not in handler anymore

	return handled;
}


static void
InitInterruptHandler(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);

	TRACE("enter InitInterruptHandler()\n");

	DisableVBI();					// disable & clear any pending interrupts
	si.bInterruptAssigned = false;	// indicate interrupt not assigned yet

	// Create a semaphore for vertical blank management.
	si.vertBlankSem = create_sem(0, di.name);
	if (si.vertBlankSem < 0)
		return;

	// Change the owner of the semaphores to the calling team (usually the
	// app_server).  This is required because apps can't aquire kernel
	// semaphores.

	thread_id threadID = find_thread(NULL);
	thread_info threadInfo;
	status_t status = get_thread_info(threadID, &threadInfo);
	if (status == B_OK)
		status = set_sem_owner(si.vertBlankSem, threadInfo.team);

	// If there is a valid interrupt assigned, set up interrupts.

	if (status == B_OK && di.pciInfo.u.h0.interrupt_pin != 0x00
		&& di.pciInfo.u.h0.interrupt_line != 0xff) {
		// We have a interrupt line to use.

		status = install_io_interrupt_handler(di.pciInfo.u.h0.interrupt_line,
			InterruptHandler, (void*)(&di), 0);

		if (status == B_OK)
			si.bInterruptAssigned = true;	// we can use interrupt related functions
	}

	if (status != B_OK) {
		// Interrupt does not exist; thus delete semaphore as it won't be used.
		delete_sem(si.vertBlankSem);
		si.vertBlankSem = -1;
	}
}


static status_t
InitDevice(DeviceInfo& di)
{
	// Perform initialization and mapping of the device, and return B_OK if
	// sucessful;  else, return error code.

	// Create the area for shared info with NO user-space read or write
	// permissions, to prevent accidental damage.

	TRACE("enter InitDevice()\n");

	pci_info& pciInfo = di.pciInfo;
	char sharedName[B_OS_NAME_LENGTH];

	sprintf(sharedName, DEVICE_FORMAT " shared",
		pciInfo.vendor_id, pciInfo.device_id,
		pciInfo.bus, pciInfo.device, pciInfo.function);

	di.sharedArea = create_area(sharedName, (void**) &(di.sharedInfo),
		B_ANY_KERNEL_ADDRESS,
		((sizeof(SharedInfo) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)),
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (di.sharedArea < 0)
		return di.sharedArea;	// return error code

	SharedInfo& si = *(di.sharedInfo);

	si.vendorID = pciInfo.vendor_id;
	si.deviceID = pciInfo.device_id;
	si.revision = pciInfo.revision;
	si.chipType = di.pChipInfo->chipType;
	strcpy(si.chipName, di.pChipInfo->chipName);

	// Trio64 and Trio64V+ chips have the same ID but different revision numbers.
	// Since the Trio64V+ supports MMIO, better performance can be obtained
	// from it if it is distinguished from the Trio64.

	if (si.chipType == S3_TRIO64 && si.revision & 0x40) {
		si.chipType = S3_TRIO64_VP;
		strcpy(si.chipName, "Trio64 V+");
	}

	status_t status = MapDevice(di);
	if (status < 0) {
		delete_area(di.sharedArea);
		di.sharedArea = -1;
		di.sharedInfo = NULL;
		return status;		// return error code
	}

	InitInterruptHandler(di);

	TRACE("Interrupt assigned:  %s\n", si.bInterruptAssigned ? "yes" : "no");
	return B_OK;
}


static const ChipInfo*
GetNextSupportedDevice(uint32& pciIndex, pci_info& pciInfo)
{
	// Search the PCI devices for a device that is supported by this driver.
	// The search starts at the device specified by argument pciIndex, and
	// continues until a supported device is found or there are no more devices
	// to examine.  Argument pciIndex is incremented after each device is
	// examined.

	// If a supported device is found, return a pointer to the struct containing
	// the chip info; else return NULL.

	while (gPCI->get_nth_pci_info(pciIndex, &pciInfo) == B_OK) {
		if (pciInfo.vendor_id == VENDOR_ID) {
			// Search the table of supported devices to find a chip/device that
			// matches device ID of the current PCI device.

			const ChipInfo* pDevice = chipTable;

			while (pDevice->chipID != 0) {		// end of table?
				if (pDevice->chipID == pciInfo.device_id) {
					pciIndex++;
					return pDevice;	// matching device/chip found
				}

				pDevice++;
			}
		}

		pciIndex++;
	}

	return NULL;		// no supported device found found
}



#ifdef __HAIKU__

static status_t
GetEdidFromBIOS(edid1_raw& edidRaw)
{
	// Get the EDID info from the video BIOS, and return B_OK if successful.

#define ADDRESS_SEGMENT(address) ((addr_t)(address) >> 4)
#define ADDRESS_OFFSET(address) ((addr_t)(address) & 0xf)

	bios_module_info* biosModule;
	status_t status = get_module(B_BIOS_MODULE_NAME, (module_info**)&biosModule);
	if (status != B_OK) {
		TRACE("GetEdidFromBIOS(): failed to get BIOS module: 0x%" B_PRIx32 "\n",
			status);
		return status;
	}

	bios_state* state;
	status = biosModule->prepare(&state);
	if (status != B_OK) {
		TRACE("GetEdidFromBIOS(): bios_prepare() failed: 0x%" B_PRIx32 "\n",
			status);
		put_module(B_BIOS_MODULE_NAME);
		return status;
	}

	bios_regs regs = {};
	regs.eax = 0x4f15;
	regs.ebx = 0;			// 0 = report DDC service
	regs.ecx = 0;
	regs.es = 0;
	regs.edi = 0;

	status = biosModule->interrupt(state, 0x10, &regs);
	if (status == B_OK) {
		// AH contains the error code, and AL determines whether or not the
		// function is supported.
		if (regs.eax != 0x4f)
			status = B_NOT_SUPPORTED;

		// Test if DDC is supported by the monitor.
		if ((regs.ebx & 3) == 0)
			status = B_NOT_SUPPORTED;
	}

	if (status == B_OK) {
		edid1_raw* edid = (edid1_raw*)biosModule->allocate_mem(state,
			sizeof(edid1_raw));
		if (edid == NULL) {
			status = B_NO_MEMORY;
			goto out;
		}

		regs.eax = 0x4f15;
		regs.ebx = 1;		// 1 = read EDID
		regs.ecx = 0;
		regs.edx = 0;
		regs.es  = ADDRESS_SEGMENT(edid);
		regs.edi = ADDRESS_OFFSET(edid);

		status = biosModule->interrupt(state, 0x10, &regs);
		if (status == B_OK) {
			if (regs.eax != 0x4f) {
				status = B_NOT_SUPPORTED;
			} else {
				// Copy the EDID info to the caller's location, and compute the
				// checksum of the EDID info while copying.

				uint8 sum = 0;
				uint8 allOr = 0;
				uint8* dest = (uint8*)&edidRaw;
				uint8* src = (uint8*)edid;

				for (uint32 j = 0; j < sizeof(edidRaw); j++) {
					sum += *src;
					allOr |= *src;
					*dest++ = *src++;
				}

				if (allOr == 0) {
					TRACE("GetEdidFromBIOS(); EDID info contains only zeros\n");
					status = B_ERROR;
				} else if (sum != 0) {
					TRACE("GetEdidFromBIOS(); Checksum error in EDID info\n");
					status = B_ERROR;
				}
			}
		}
	}

out:
	biosModule->finish(state);
	put_module(B_BIOS_MODULE_NAME);

	TRACE("GetEdidFromBIOS() status: 0x%" B_PRIx32 "\n", status);
	return status;
}

#endif	// __HAIKU__



//	#pragma mark - Kernel Interface


status_t
init_hardware(void)
{
	// Return B_OK if a device supported by this driver is found; otherwise,
	// return B_ERROR so the driver will be unloaded.

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI) != B_OK)
		return B_ERROR;		// unable to access PCI bus

	// Check pci devices for a device supported by this driver.

	uint32 pciIndex = 0;
	pci_info pciInfo;
	const ChipInfo* pDevice = GetNextSupportedDevice(pciIndex, pciInfo);

	TRACE("init_hardware() - %s\n", pDevice == NULL ? "no supported devices" : "device supported");

	put_module(B_PCI_MODULE_NAME);		// put away the module manager

	return (pDevice == NULL ? B_ERROR : B_OK);
}


status_t  init_driver(void)
{
	// Get handle for the pci bus.

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI) != B_OK)
		return B_ERROR;

	status_t status = gLock.Init("S3 driver lock");
	if (status < B_OK)
		return status;

	// Get info about all the devices supported by this driver.

	uint32 pciIndex = 0;
	uint32 count = 0;

	while (count < MAX_DEVICES) {
		DeviceInfo& di = gDeviceInfo[count];

		const ChipInfo* pDevice = GetNextSupportedDevice(pciIndex, di.pciInfo);
		if (pDevice == NULL)
			break;			// all supported devices have been obtained

		// Compose device name.
		sprintf(di.name, "graphics/" DEVICE_FORMAT,
				  di.pciInfo.vendor_id, di.pciInfo.device_id,
				  di.pciInfo.bus, di.pciInfo.device, di.pciInfo.function);
		TRACE("init_driver() match found; name: %s\n", di.name);

		gDeviceNames[count] = di.name;
		di.openCount = 0;		// mark driver as available for R/W open
		di.sharedArea = -1;		// indicate shared area not yet created
		di.sharedInfo = NULL;
		di.pChipInfo = pDevice;
		count++;
	}

	gDeviceNames[count] = NULL;	// terminate list with null pointer

	TRACE("init_driver() %ld supported devices\n", count);

	return B_OK;
}


void
uninit_driver(void)
{
	// Free the driver data.

	gLock.Delete();
	put_module(B_PCI_MODULE_NAME);	// put the pci module away
}


const char**
publish_devices(void)
{
	return (const char**)gDeviceNames;	// return list of supported devices
}


device_hooks*
find_device(const char* name)
{
	int index = 0;
	while (gDeviceNames[index] != NULL) {
		if (strcmp(name, gDeviceNames[index]) == 0)
			return &gDeviceHooks;
		index++;
	}

	return NULL;
}



//	#pragma mark - Device Hooks


static status_t
device_open(const char* name, uint32 /*flags*/, void** cookie)
{
	status_t status = B_OK;

	TRACE("device_open() - name: %s, cookie: 0x%" B_PRIXADDR "\n", name,
		(addr_t)cookie);

	// Find the device name in the list of devices.

	int32 index = 0;
	while (gDeviceNames[index] != NULL && (strcmp(name, gDeviceNames[index]) != 0))
		index++;

	if (gDeviceNames[index] == NULL)
		return B_BAD_VALUE;		// device name not found in list of devices

	DeviceInfo& di = gDeviceInfo[index];

	gLock.Acquire();	// make sure no one else has write access to common data

	if (di.openCount == 0)
		status = InitDevice(di);

	gLock.Release();

	if (status == B_OK) {
		di.openCount++;		// mark device open
		*cookie = &di;		// send cookie to opener
	}

	TRACE("device_open() returning 0x%lx,  open count: %ld\n", status, di.openCount);
	return status;
}


static status_t
device_read(void* dev, off_t pos, void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter" warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_write(void* dev, off_t pos, const void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter" warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_close(void* dev)
{
	(void)dev;		// avoid compiler warning for unused arg

	TRACE("device_close()\n");
	return B_NO_ERROR;
}


static status_t
device_free(void* dev)
{
	DeviceInfo& di = *((DeviceInfo*)dev);
	SharedInfo& si = *(di.sharedInfo);
	pci_info& pciInfo = di.pciInfo;

	TRACE("enter device_free()\n");

	gLock.Acquire();		// lock driver

	// If opened multiple times, merely decrement the open count and exit.

	if (di.openCount <= 1) {
		DisableVBI();		// disable & clear any pending interrupts

		if (si.bInterruptAssigned) {
			remove_io_interrupt_handler(pciInfo.u.h0.interrupt_line, InterruptHandler, &di);
		}

		// Delete the semaphores, ignoring any errors because the owning team may have died.
		if (si.vertBlankSem >= 0)
			delete_sem(si.vertBlankSem);
		si.vertBlankSem = -1;

		UnmapDevice(di);	// free regs and frame buffer areas

		delete_area(di.sharedArea);
		di.sharedArea = -1;
		di.sharedInfo = NULL;
	}

	if (di.openCount > 0)
		di.openCount--;		// mark device available

	gLock.Release();	// unlock driver

	TRACE("exit device_free() openCount: %ld\n", di.openCount);
	return B_OK;
}


static status_t
device_ioctl(void* dev, uint32 msg, void* buf, size_t len)
{
	DeviceInfo& di = *((DeviceInfo*)dev);

	(void)len;		// avoid compiler warning for unused arg

//	TRACE("device_ioctl(); ioctl: %lu, buf: 0x%08lx, len: %lu\n", msg, (uint32)buf, len);

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char*)buf, "s3.accelerant");
			return B_OK;

		case S3_DEVICE_NAME:
			strncpy((char*)buf, di.name, B_OS_NAME_LENGTH);
			((char*)buf)[B_OS_NAME_LENGTH -1] = '\0';
			return B_OK;

		case S3_GET_PRIVATE_DATA:
		{
			S3GetPrivateData* gpd = (S3GetPrivateData*)buf;
			if (gpd->magic == S3_PRIVATE_DATA_MAGIC) {
				gpd->sharedInfoArea = di.sharedArea;
				return B_OK;
			}
			break;
		}

		case S3_GET_EDID:
		{
#ifdef __HAIKU__
			S3GetEDID* ged = (S3GetEDID*)buf;
			if (ged->magic == S3_PRIVATE_DATA_MAGIC) {
				edid1_raw rawEdid;
				status_t status = GetEdidFromBIOS(rawEdid);
				if (status == B_OK)
					user_memcpy(&ged->rawEdid, &rawEdid, sizeof(rawEdid));
				return status;
			}
#else
			return B_UNSUPPORTED;
#endif
			break;
		}

		case S3_GET_PIO:
		{
			S3GetSetPIO* gsp = (S3GetSetPIO*)buf;
			if (gsp->magic == S3_PRIVATE_DATA_MAGIC) {
				switch (gsp->size) {
					case 1:
						gsp->value = gPCI->read_io_8(gsp->offset);
						break;
					case 2:
						gsp->value = gPCI->read_io_16(gsp->offset);
						break;
					case 4:
						gsp->value = gPCI->read_io_32(gsp->offset);
						break;
					default:
						TRACE("device_ioctl() S3_GET_PIO invalid size: %ld\n", gsp->size);
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
						gPCI->write_io_8(gsp->offset, gsp->value);
						break;
					case 2:
						gPCI->write_io_16(gsp->offset, gsp->value);
						break;
					case 4:
						gPCI->write_io_32(gsp->offset, gsp->value);
						break;
					default:
						TRACE("device_ioctl() S3_SET_PIO invalid size: %ld\n", gsp->size);
						return B_ERROR;
				}
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
	}

	return B_DEV_INVALID_IOCTL;
}

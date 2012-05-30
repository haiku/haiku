/*
 * Copyright 2007-2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include <AGP.h>
#include <KernelExport.h>
#include <PCI.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <graphic_driver.h>
#include <boot_item.h>
#include <arch/x86/vm86.h>

#include "DriverInterface.h"


#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
#	define TRACE(x...) dprintf("i810: " x)
#else
#	define TRACE(x...) ;
#endif


#define ACCELERANT_NAME    "intel_810.accelerant"

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

#define MAX_DEVICES		4
#define DEVICE_FORMAT	"%04X_%04X_%02X%02X%02X"

#define VENDOR_ID	0x8086	// Intel vendor ID


struct ChipInfo {
	uint16		chipID;		// PCI device id of the chip
	const char*	chipName;	// user recognizable name (must be < 32 chars)
};


// This table maps a PCI device ID to a chip type identifier and the chip name.

static const ChipInfo chipTable[] = {
	{ 0x7121, "i810"	},
	{ 0x7123, "i810-dc100"	},
	{ 0x7125, "i810e"	},
	{ 0x1132, "i815"	},
	{ 0,    NULL }
};


struct DeviceInfo {
	uint32			openCount;		// count of how many times device has been opened
	int32			flags;
	area_id 		sharedArea;		// area shared between driver and accelerants
	SharedInfo* 	sharedInfo;		// pointer to shared info area memory
	vuint8*	 		regs;			// pointer to memory mapped registers
	const ChipInfo*	pChipInfo;		// info about the selected chip
	pci_info		pciInfo;		// copy of pci info for this device
	area_id			gttArea;		// area used for GTT
	addr_t			gttAddr;		// virtual address of GTT
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
static status_t device_write(void* dev, off_t pos, const void* buf,
					size_t* len);
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



// Video chip register definitions.
//=================================

#define INTERRUPT_ENABLED		0x020a0
#define INTERRUPT_MASK			0x020a8

// Graphics address translation table.
#define PAGE_TABLE_CONTROL		0x02020
#define PAGE_TABLE_ENABLED		0x01

#define PTE_BASE				0x10000
#define PTE_VALID				0x01


// Macros for memory mapped I/O.
//==============================

#define INREG16(addr)		(*((vuint16*)(di.regs + (addr))))
#define INREG32(addr)		(*((vuint32*)(di.regs + (addr))))

#define OUTREG16(addr, val)	(*((vuint16*)(di.regs + (addr))) = (val))
#define OUTREG32(addr, val)	(*((vuint32*)(di.regs + (addr))) = (val))



static inline uint32
GetPCI(pci_info& info, uint8 offset, uint8 size)
{
	return gPCI->read_pci_config(info.bus, info.device, info.function, offset,
		size);
}


static inline void
SetPCI(pci_info& info, uint8 offset, uint8 size, uint32 value)
{
	gPCI->write_pci_config(info.bus, info.device, info.function, offset, size,
		value);
}


static status_t
GetEdidFromBIOS(edid1_raw& edidRaw)
{
	// Get the EDID info from the video BIOS, and return B_OK if successful.

#define ADDRESS_SEGMENT(address) ((addr_t)(address) >> 4)
#define ADDRESS_OFFSET(address) ((addr_t)(address) & 0xf)

	vm86_state vmState;

	status_t status = vm86_prepare(&vmState, 0x2000);
	if (status != B_OK) {
		TRACE("GetEdidFromBIOS(); vm86_prepare() failed, status: 0x%lx\n",
			status);
		return status;
	}

	vmState.regs.eax = 0x4f15;
	vmState.regs.ebx = 0;		// 0 = report DDC service
	vmState.regs.ecx = 0;
	vmState.regs.es = 0;
	vmState.regs.edi = 0;

	status = vm86_do_int(&vmState, 0x10);
	if (status == B_OK) {
		// AH contains the error code, and AL determines wether or not the
		// function is supported.
		if (vmState.regs.eax != 0x4f)
			status = B_NOT_SUPPORTED;

		// Test if DDC is supported by the monitor.
		if ((vmState.regs.ebx & 3) == 0)
			status = B_NOT_SUPPORTED;
	}

	if (status == B_OK) {
		// According to the author of the vm86 functions, the address of any
		// object to receive data must be >= 0x1000 and within the ram size
		// specified in the second argument of the vm86_prepare() call above.
		// Thus, the address of the struct to receive the EDID info is set to
		// 0x1000.

		edid1_raw* edid = (edid1_raw*)0x1000;

		vmState.regs.eax = 0x4f15;
		vmState.regs.ebx = 1;		// 1 = read EDID
		vmState.regs.ecx = 0;
		vmState.regs.edx = 0;
		vmState.regs.es  = ADDRESS_SEGMENT(edid);
		vmState.regs.edi = ADDRESS_OFFSET(edid);

		status = vm86_do_int(&vmState, 0x10);
		if (status == B_OK) {
			if (vmState.regs.eax != 0x4f) {
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

	vm86_cleanup(&vmState);

	TRACE("GetEdidFromBIOS() status: 0x%lx\n", status);
	return status;
}


static status_t
InitDevice(DeviceInfo& di)
{
	// Perform initialization and mapping of the device, and return B_OK if
	// sucessful;  else, return error code.

	TRACE("enter InitDevice()\n");

	// Create the area for shared info with NO user-space read or write
	// permissions, to prevent accidental damage.

	size_t sharedSize = (sizeof(SharedInfo) + 7) & ~7;

	di.sharedArea = create_area("i810 shared info",
		(void**) &(di.sharedInfo),
		B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize),
		B_FULL_LOCK, 0);
	if (di.sharedArea < 0)
		return di.sharedArea;	// return error code

	SharedInfo& si = *(di.sharedInfo);
	memset(&si, 0, sharedSize);
	si.regsArea = -1;			// indicate area has not yet been created
	si.videoMemArea = -1;

	pci_info& pciInfo = di.pciInfo;

	si.vendorID = pciInfo.vendor_id;
	si.deviceID = pciInfo.device_id;
	si.revision = pciInfo.revision;
	strcpy(si.chipName, di.pChipInfo->chipName);

	// Enable memory mapped IO and bus master.

	SetPCI(pciInfo, PCI_command, 2, GetPCI(pciInfo, PCI_command, 2)
		| PCI_command_io | PCI_command_memory | PCI_command_master);

	// Map the MMIO register area.

	phys_addr_t regsBase = pciInfo.u.h0.base_registers[1];
	uint32 regAreaSize = pciInfo.u.h0.base_register_sizes[1];

	si.regsArea = map_physical_memory("i810 mmio registers",
		regsBase,
		regAreaSize,
		B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&di.regs);

	if (si.regsArea < 0) {
		TRACE("Unable to map MMIO, error: 0x%lx\n", si.regsArea);
		return si.regsArea;
	}

	// Allocate memory for the GTT which must be 64K for the 810/815 chips.

	uint32 gttSize = 64 * 1024;
	di.gttArea = create_area("GTT memory", (void**) &(di.gttAddr),
		B_ANY_KERNEL_ADDRESS, gttSize, B_FULL_LOCK | B_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA);

	if (di.gttArea < B_OK) {
		TRACE("Unable to create GTT, error: 0x%lx\n", di.gttArea);
		return B_NO_MEMORY;
	}	

	memset((void*)(di.gttAddr), 0, gttSize);

	// Get the physical address of the GTT, and set GTT address in the chip.
	
	physical_entry entry;
	status_t status = get_memory_map((void *)(di.gttAddr),
		B_PAGE_SIZE, &entry, 1);
	if (status < B_OK) {
		TRACE("Unable to get physical address of GTT, error: 0x%lx\n", status);
		return status;
	}

	OUTREG32(PAGE_TABLE_CONTROL, entry.address | PAGE_TABLE_ENABLED);	
	INREG32(PAGE_TABLE_CONTROL);	

	// Allocate video memory to be used for the frame buffer.

	si.videoMemSize = 4 * 1024 * 1024;
	si.videoMemArea = create_area("video memory", (void**)&(si.videoMemAddr),
		B_ANY_ADDRESS, si.videoMemSize, B_FULL_LOCK,
		B_READ_AREA | B_WRITE_AREA);
	if (si.videoMemArea < B_OK) {
		TRACE("Unable to create video memory, error: 0x%lx\n", si.videoMemArea);
		return B_NO_MEMORY;
	}

	// Get the physical address of each page of the video memory, and put
	// the physical address of each page into the GTT table.

	for (uint32 offset = 0; offset < si.videoMemSize; offset += B_PAGE_SIZE) {
		status = get_memory_map((void *)(si.videoMemAddr + offset),
			B_PAGE_SIZE, &entry, 1);
		if (status < B_OK) {
			TRACE("Unable to get physical address of video memory page, error:"
				" 0x%lx  offset: %ld\n", status, offset);
			return status;
		}

		if (offset == 0)
			si.videoMemPCI = entry.address;

		OUTREG32(PTE_BASE + ((offset / B_PAGE_SIZE) * 4),
			entry.address | PTE_VALID);
	}

	TRACE("InitDevice() exit OK\n");
	return B_OK;
}


static void
DeleteAreas(DeviceInfo& di)
{
	// Delete all areas that were created.

	if (di.sharedArea >= 0 && di.sharedInfo != NULL) {
		SharedInfo& si = *(di.sharedInfo);
		if (si.regsArea >= 0)
			delete_area(si.regsArea);
		if (si.videoMemArea >= 0)
			delete_area(si.videoMemArea);
	}

	if (di.gttArea >= 0)
		delete_area(di.gttArea);
	di.gttArea = -1;
	di.gttAddr = (addr_t)NULL;

	if (di.sharedArea >= 0)
		delete_area(di.sharedArea);
	di.sharedArea = -1;
	di.sharedInfo = NULL;
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

			while (pDevice->chipID != 0) {	// end of table?
				if (pDevice->chipID == pciInfo.device_id)
					return pDevice;		// matching device/chip found

				pDevice++;
			}
		}

		pciIndex++;
	}

	return NULL;		// no supported device found
}



//	#pragma mark - Kernel Interface


status_t
init_hardware(void)
{
	// Return B_OK if a device supported by this driver is found; otherwise,
	// return B_ERROR so the driver will be unloaded.

	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI);
	if (status != B_OK) {
		TRACE("PCI module unavailable, error 0x%lx\n", status);
		return status;
	}

	// Check pci devices for a device supported by this driver.

	uint32 pciIndex = 0;
	pci_info pciInfo;
	const ChipInfo* pDevice = GetNextSupportedDevice(pciIndex, pciInfo);

	TRACE("init_hardware() - %s\n",
		pDevice == NULL ? "no supported devices" : "device supported");

	put_module(B_PCI_MODULE_NAME);		// put away the module manager

	return (pDevice == NULL ? B_ERROR : B_OK);
}


status_t
init_driver(void)
{
	// Get handle for the pci bus.

	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI);
	if (status != B_OK) {
		TRACE("PCI module unavailable, error 0x%lx\n", status);
		return status;
	}

	status = gLock.Init("i810 driver lock");
	if (status < B_OK) {
		put_module(B_AGP_GART_MODULE_NAME);
		put_module(B_PCI_MODULE_NAME);
		return status;
	}

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
		di.gttArea = -1;		// indicate GTT area not yet created
		di.gttAddr = (addr_t)NULL;
		di.pChipInfo = pDevice;
		count++;
		pciIndex++;
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
	put_module(B_AGP_GART_MODULE_NAME);
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
	int i = 0;
	while (gDeviceNames[i] != NULL) {
		if (strcmp(name, gDeviceNames[i]) == 0)
			return &gDeviceHooks;
		i++;
	}

	return NULL;
}



//	#pragma mark - Device Hooks


static status_t
device_open(const char* name, uint32 /*flags*/, void** cookie)
{
	status_t status = B_OK;

	TRACE("device_open() - name: %s, cookie: 0x%08lx)\n", name, (uint32)cookie);

	// Find the device name in the list of devices.

	int32 i = 0;
	while (gDeviceNames[i] != NULL && (strcmp(name, gDeviceNames[i]) != 0))
		i++;

	if (gDeviceNames[i] == NULL)
		return B_BAD_VALUE;		// device name not found in list of devices

	DeviceInfo& di = gDeviceInfo[i];

	gLock.Acquire();	// make sure no one else has write access to common data

	if (di.openCount == 0) {
		status = InitDevice(di);
		if (status < B_OK)
			DeleteAreas(di);	// error occurred; delete any areas created
	}
	
	gLock.Release();

	if (status == B_OK) {
		di.openCount++;		// mark device open
		*cookie = &di;		// send cookie to opener
	}

	TRACE("device_open() returning 0x%lx,  open count: %ld\n", status,
		di.openCount);
	return status;
}


static status_t
device_read(void* dev, off_t pos, void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter"
	// warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_write(void* dev, off_t pos, const void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter"
	// warnings.
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

	TRACE("enter device_free()\n");

	gLock.Acquire();		// lock driver

	// If opened multiple times, merely decrement the open count and exit.

	if (di.openCount <= 1)
		DeleteAreas(di);

	if (di.openCount > 0)
		di.openCount--;		// mark device available

	gLock.Release();	// unlock driver

	TRACE("exit device_free() openCount: %ld\n", di.openCount);
	return B_OK;
}


static status_t
device_ioctl(void* dev, uint32 msg, void* buffer, size_t bufferLength)
{
	DeviceInfo& di = *((DeviceInfo*)dev);

	TRACE("device_ioctl(); ioctl: %lu, buffer: 0x%08lx, bufLen: %lu\n", msg,
		(uint32)buffer, bufferLength);

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char*)buffer, ACCELERANT_NAME);
			TRACE("Intel 810 accelerant: %s\n", ACCELERANT_NAME);
			return B_OK;

		case INTEL_DEVICE_NAME:
			strncpy((char*)buffer, di.name, B_OS_NAME_LENGTH);
			((char*)buffer)[B_OS_NAME_LENGTH -1] = '\0';
			return B_OK;

		case INTEL_GET_SHARED_DATA:
			if (bufferLength != sizeof(area_id))
				return B_BAD_DATA;

			*((area_id*)buffer) = di.sharedArea;
			return B_OK;

		case INTEL_GET_EDID:
		{
			if (bufferLength != sizeof(edid1_raw))
				return B_BAD_DATA;

			edid1_raw rawEdid;
			status_t status = GetEdidFromBIOS(rawEdid);
			if (status == B_OK)
				user_memcpy((edid1_raw*)buffer, &rawEdid, sizeof(rawEdid));
			return status;
		}
	}

	return B_DEV_INVALID_IOCTL;
}

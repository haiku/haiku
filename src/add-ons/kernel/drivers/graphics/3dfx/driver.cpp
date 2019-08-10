/*
 * Copyright 2007-2010 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include <KernelExport.h>
#include <PCI.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <graphic_driver.h>
#ifdef __HAIKU__
#include <boot_item.h>
#endif	// __HAIKU__

#include "DriverInterface.h"


#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
#	define TRACE(x...) dprintf("3dfx: " x)
#else
#	define TRACE(x...) ;
#endif


#define ACCELERANT_NAME	 "3dfx.accelerant"

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

#define SKD_HANDLER_INSTALLED 0x80000000
#define MAX_DEVICES		4
#define DEVICE_FORMAT	"%04X_%04X_%02X%02X%02X"

int32 api_version = B_CUR_DRIVER_API_VERSION;	// revision of driver API used

#define VENDOR_ID	0x121A		// 3DFX vendor ID


struct ChipInfo {
	uint16		chipID;			// PCI device id of the chip
	ChipType	chipType;		// assigned chip type identifier
	const char*	chipName;		// user recognizable name for chip
								//   (must be < 32 chars)
};


// This table maps a PCI device ID to a chip type identifier and the chip name.

static const ChipInfo chipTable[] = {
	{ 0x03, BANSHEE,	"Banshee"	},
	{ 0x05, VOODOO_3,	"Voodoo 3"	},
	{ 0x09, VOODOO_5,	"Voodoo 5"	},
	{ 0,	TDFX_NONE,	NULL }
};


struct DeviceInfo {
	uint32			openCount;		// count of how many times device has been opened
	int32			flags;
	area_id 		sharedArea;		// area shared between driver and all accelerants
	SharedInfo* 	sharedInfo;		// pointer to shared info area memory
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
MapDevice(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);
	pci_info& pciInfo = di.pciInfo;

	TRACE("enter MapDevice()\n");

	// Enable memory mapped IO and bus master.

	SetPCI(pciInfo, PCI_command, 2, GetPCI(pciInfo, PCI_command, 2)
		| PCI_command_io | PCI_command_memory | PCI_command_master);

	// Map the video memory.

	phys_addr_t videoRamAddr = pciInfo.u.h0.base_registers[1];
	uint32 videoRamSize = pciInfo.u.h0.base_register_sizes[1];
	si.videoMemPCI = videoRamAddr;
	char frameBufferAreaName[] = "3DFX frame buffer";

	si.videoMemArea = map_physical_memory(
		frameBufferAreaName,
		videoRamAddr,
		videoRamSize,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		(void**)&si.videoMemAddr);

	TRACE("Video memory, area: %ld,  addr: 0x%lX, size: %ld\n",
		si.videoMemArea, (uint32)(si.videoMemAddr), videoRamSize);

	if (si.videoMemArea < 0) {
		// Try to map this time without write combining.
		si.videoMemArea = map_physical_memory(
			frameBufferAreaName,
			videoRamAddr,
			videoRamSize,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			(void**)&si.videoMemAddr);
	}

	if (si.videoMemArea < 0)
		return si.videoMemArea;

	// Map the MMIO register area.

	phys_addr_t regsBase = pciInfo.u.h0.base_registers[0];
	uint32 regAreaSize = pciInfo.u.h0.base_register_sizes[0];

	si.regsArea = map_physical_memory("3DFX mmio registers",
		regsBase,
		regAreaSize,
		B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA,
		(void**)&di.regs);

	// If there was an error, delete other areas.
	if (si.regsArea < 0) {
		delete_area(si.videoMemArea);
		si.videoMemArea = -1;
	}

	TRACE("leave MapDevice(); result: %ld\n", si.regsArea);
	return si.regsArea;
}


static void
UnmapDevice(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);

	if (si.regsArea >= 0)
		delete_area(si.regsArea);
	if (si.videoMemArea >= 0)
		delete_area(si.videoMemArea);

	si.regsArea = si.videoMemArea = -1;
	si.videoMemAddr = (addr_t)NULL;
	di.regs = NULL;
}


static status_t
InitDevice(DeviceInfo& di)
{
	// Perform initialization and mapping of the device, and return B_OK if
	// sucessful;  else, return error code.

	// Create the area for shared info with NO user-space read or write
	// permissions, to prevent accidental damage.

	TRACE("enter InitDevice()\n");

	size_t sharedSize = (sizeof(SharedInfo) + 7) & ~7;

	di.sharedArea = create_area("3DFX shared info",
		(void**) &(di.sharedInfo),
		B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize),
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (di.sharedArea < 0)
		return di.sharedArea;	// return error code

	SharedInfo& si = *(di.sharedInfo);

	memset(&si, 0, sharedSize);

	pci_info& pciInfo = di.pciInfo;

	si.vendorID = pciInfo.vendor_id;
	si.deviceID = pciInfo.device_id;
	si.revision = pciInfo.revision;
	si.chipType = di.pChipInfo->chipType;
	strcpy(si.chipName, di.pChipInfo->chipName);

	status_t status = MapDevice(di);
	if (status < 0) {
		delete_area(di.sharedArea);
		di.sharedArea = -1;
		di.sharedInfo = NULL;
		return status;		// return error code
	}

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

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI) != B_OK)
		return B_ERROR;		// unable to access PCI bus

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

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI) != B_OK)
		return B_ERROR;

	status_t status = gLock.Init("3DFX driver lock");
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

	if (di.openCount == 0)
		status = InitDevice(di);

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

	TRACE("enter device_free()\n");

	gLock.Acquire();		// lock driver

	// If opened multiple times, merely decrement the open count and exit.

	if (di.openCount <= 1) {
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
device_ioctl(void* dev, uint32 msg, void* buffer, size_t bufferLength)
{
	DeviceInfo& di = *((DeviceInfo*)dev);

#ifndef __HAIKU__
	(void)bufferLength;		// avoid compiler warning for unused arg
#endif

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char*)buffer, ACCELERANT_NAME);
			return B_OK;

		case TDFX_DEVICE_NAME:
			strncpy((char*)buffer, di.name, B_OS_NAME_LENGTH);
			((char*)buffer)[B_OS_NAME_LENGTH -1] = '\0';
			return B_OK;

		case TDFX_GET_SHARED_DATA:
#ifdef __HAIKU__
			if (bufferLength != sizeof(area_id))
				return B_BAD_DATA;
#endif

			*((area_id*)buffer) = di.sharedArea;
			return B_OK;

		case TDFX_GET_PIO_REG:
		{
#ifdef __HAIKU__
			if (bufferLength != sizeof(PIORegInfo))
				return B_BAD_DATA;
#endif

			PIORegInfo* regInfo = (PIORegInfo*)buffer;
			if (regInfo->magic == TDFX_PRIVATE_DATA_MAGIC) {
				int ioAddr = di.pciInfo.u.h0.base_registers[2] + regInfo->offset;
				if (regInfo->index >= 0) {
					gPCI->write_io_8(ioAddr, regInfo->index);
					regInfo->value = gPCI->read_io_8(ioAddr + 1);
				} else {
					regInfo->value = gPCI->read_io_8(ioAddr);
				}
				return B_OK;
			}
			break;
		}

		case TDFX_SET_PIO_REG:
		{
#ifdef __HAIKU__
			if (bufferLength != sizeof(PIORegInfo))
				return B_BAD_DATA;
#endif

			PIORegInfo* regInfo = (PIORegInfo*)buffer;
			if (regInfo->magic == TDFX_PRIVATE_DATA_MAGIC) {
				int ioAddr = di.pciInfo.u.h0.base_registers[2] + regInfo->offset;
				if (regInfo->index >= 0) {
					gPCI->write_io_8(ioAddr, regInfo->index);
					gPCI->write_io_8(ioAddr + 1, regInfo->value);
				} else {
					gPCI->write_io_8(ioAddr, regInfo->value);
				}
				return B_OK;
			}
			break;
		}
	}

	return B_DEV_INVALID_IOCTL;
}

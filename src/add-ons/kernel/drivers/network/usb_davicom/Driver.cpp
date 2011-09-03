/*
 *  Davicom 9601 USB 1.1 Ethernet Driver.
 *  Copyright 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of 
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *	
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <lock.h> // for mutex
#else
#include "BeOSCompatibility.h" // for pseudo mutex
#endif

#include "DavicomDevice.h"
#include "Driver.h"
#include "Settings.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_davicom/";
DavicomDevice *gDavicomDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;

usb_support_descriptor gSupportedDevices[] = {
	{ 0, 0, 0, 0x0fe6, 0x8101}, // "Supereal SR9600"
	{ 0, 0, 0, 0x07aa, 0x9601}, // "Corega FEther USB-TXC"
	{ 0, 0, 0, 0x0a46, 0x9601}, // "Davicom DM9601"
	{ 0, 0, 0, 0x0a46, 0x6688}, // "ZT6688 USB NIC"
	{ 0, 0, 0, 0x0a46, 0x0268}, // "ShanTou ST268 USB NIC"
	{ 0, 0, 0, 0x0a46, 0x8515}, // "ADMtek ADM8515 USB NIC"
	{ 0, 0, 0, 0x0a47, 0x9601}, // "Hirose USB-100"
	{ 0, 0, 0, 0x0a46, 0x9000}	// "DM9000E"
};

mutex gDriverLock;
// auto-release helper class
class DriverSmartLock {
public:
	DriverSmartLock()  { mutex_lock(&gDriverLock);   }
	~DriverSmartLock() { mutex_unlock(&gDriverLock); }
};

DavicomDevice *
create_davicom_device(usb_device device)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error of getting USB device descriptor.\n");
		return NULL;
	}

#define IDS(__vendor, __product) (((__vendor) << 16) | (__product))
	
	switch(IDS(deviceDescriptor->vendor_id, deviceDescriptor->product_id)) {
		case IDS(0x0fe6, 0x8101):
			return new DavicomDevice(device, "Sunrising JP108");
		case IDS(0x07aa, 0x9601):
			return new DavicomDevice(device, "Corega FEther USB-TXC");
		case IDS(0x0a46, 0x9601):
			return new DavicomDevice(device, "Davicom USB-100");
		case IDS(0x0a46, 0x6688):
			return new DavicomDevice(device, "ZT6688 USB NIC");
		case IDS(0x0a46, 0x0268):
			return new DavicomDevice(device, "ShanTou ST268 USB NIC");
		case IDS(0x0a46, 0x8515):
			return new DavicomDevice(device, "ADMtek ADM8515 USB NIC");
		case IDS(0x0a47, 0x9601):
			return new DavicomDevice(device, "Hirose USB-100");
		case IDS(0x0a46, 0x9000):
			return new DavicomDevice(device, "DM9000E");
	}
	return NULL;
}


status_t
usb_davicom_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	DriverSmartLock driverLock; // released on exit

	// check if this is a replug of an existing device first
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDavicomDevices[i] == NULL)
			continue;

		if (gDavicomDevices[i]->CompareAndReattach(device) != B_OK)
			continue;

		TRACE("The device is plugged back. Use entry at %ld.\n", i);
		*cookie = gDavicomDevices[i];
		return B_OK;
	}

	// no such device yet, create a new one
	DavicomDevice *davicomDevice = create_davicom_device(device);
	if (davicomDevice == 0) {
		return ENODEV;
	}

	status_t status = davicomDevice->InitCheck();
	if (status < B_OK) {
		delete davicomDevice;
		return status;
	}

	status = davicomDevice->SetupDevice(false);
	if (status < B_OK) {
		delete davicomDevice;
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDavicomDevices[i] != NULL)
			continue;

		gDavicomDevices[i] = davicomDevice;
		*cookie = davicomDevice;

		TRACE("New device is added at %ld.\n", i);
		return B_OK;
	}

	// no space for the device
	TRACE_ALWAYS("Error: no more device entries availble.\n");

	delete davicomDevice;
	return B_ERROR;
}


status_t
usb_davicom_device_removed(void *cookie)
{
	DriverSmartLock driverLock; // released on exit

	DavicomDevice *device = (DavicomDevice *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDavicomDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gDavicomDevices[i] = NULL;
				delete device;
				TRACE("Device at %ld deleted.\n", i);
			}
			break;
		}
	}

	return B_OK;
}


//#pragma mark -


status_t
init_hardware()
{
	return B_OK;
}


status_t
init_driver()
{
	status_t status = get_module(B_USB_MODULE_NAME,
		(module_info **)&gUSBModule);
	if (status < B_OK)
		return status;

	load_settings();
	
	TRACE_ALWAYS("%s\n", kVersion);
	
	for (int32 i = 0; i < MAX_DEVICES; i++)
		gDavicomDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_davicom_device_added,
		&usb_davicom_device_removed
	};

	gUSBModule->register_driver(DRIVER_NAME, gSupportedDevices, 
		sizeof(gSupportedDevices) / sizeof(usb_support_descriptor), NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	return B_OK;
}


void
uninit_driver()
{
	gUSBModule->uninstall_notify(DRIVER_NAME);
	mutex_lock(&gDriverLock);

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDavicomDevices[i]) {
			delete gDavicomDevices[i];
			gDavicomDevices[i] = NULL;
		}
	}

	for (int32 i = 0; gDeviceNames[i]; i++) {
		free(gDeviceNames[i]);
		gDeviceNames[i] = NULL;
	}

	mutex_destroy(&gDriverLock);
	put_module(B_USB_MODULE_NAME);
	
	release_settings();
}


static status_t
usb_davicom_open(const char *name, uint32 flags, void **cookie)
{
	DriverSmartLock driverLock; // released on exit

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gDavicomDevices[index]) {
		status = gDavicomDevices[index]->Open(flags);
		*cookie = gDavicomDevices[index];
	}

	return status;
}


static status_t
usb_davicom_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	DavicomDevice *device = (DavicomDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_davicom_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	DavicomDevice *device = (DavicomDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_davicom_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	DavicomDevice *device = (DavicomDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_davicom_close(void *cookie)
{
	DavicomDevice *device = (DavicomDevice *)cookie;
	return device->Close();
}


static status_t
usb_davicom_free(void *cookie)
{
	DavicomDevice *device = (DavicomDevice *)cookie;
	
	DriverSmartLock driverLock; // released on exit
	
	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDavicomDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			gDavicomDevices[i] = NULL;
			delete device;
			TRACE("Device at %ld deleted.\n", i);
			break;
		}
	}

	return status;
}


const char **
publish_devices()
{
	for (int32 i = 0; gDeviceNames[i]; i++) {
		free(gDeviceNames[i]);
		gDeviceNames[i] = NULL;
	}

	DriverSmartLock driverLock; // released on exit
	
	int32 deviceCount = 0;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDavicomDevices[i] == NULL)
			continue;

		gDeviceNames[deviceCount] = (char *)malloc(strlen(sDeviceBaseName) + 4);
		if (gDeviceNames[deviceCount]) {
			sprintf(gDeviceNames[deviceCount], "%s%ld", sDeviceBaseName, i);
			TRACE("publishing %s\n", gDeviceNames[deviceCount]);
			deviceCount++;
		} else
			TRACE_ALWAYS("Error: out of memory during allocating device name.\n");
	}

	gDeviceNames[deviceCount] = NULL;
	return (const char **)&gDeviceNames[0];
}


device_hooks *
find_device(const char *name)
{
	static device_hooks deviceHooks = {
		usb_davicom_open,
		usb_davicom_close,
		usb_davicom_free,
		usb_davicom_control,
		usb_davicom_read,
		usb_davicom_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}


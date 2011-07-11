/*
 *	Beceem WiMax USB Driver
 *	Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 *
 *	Partially using:
 *		USB Ethernet Control Model devices
 *			(c) 2008 by Michael Lotz, <mmlr@mlotz.ch>
 *		ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver
 *			(c) 2008 by S.Zharski, <imker@gmx.li>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <lock.h> // for mutex
#else
#include "BeOSCompatibility.h" // for pseudo mutex
#endif

#include "BeceemDevice.h"
#include "Driver.h"
#include "Settings.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_beceemwmx/";
BeceemDevice *gBeceemDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;


usb_support_descriptor gSupportedDevices[] = {
	{ 0, 0, 0, 0x0489, 0xe016},
		// "Ubee WiMAX Mobile USB - PXU1900 (Clear)"
	{ 0, 0, 0, 0x0489, 0xe017},
		// "Ubee WiMAX Mobile USB - PXU1901 (Sprint)"
	{ 0, 0, 0, 0x198f, 0x0210},
		// "Motorola USBw 100 WiMAX CPE"
	{ 0, 0, 0, 0x198f, 0x0220},
		// "Huawei Mobile USB, Sierra 250U Dual 3G/WiMAX, Sprint U301 WiMAX"
	{ 0, 0, 0, 0x19d2, 0x0007}
		// "ZTE TU25 WiMAX Adapter [Beceem BCS200]"
};


mutex gDriverLock;


// auto-release helper class
class DriverSmartLock {
public:
	DriverSmartLock()  { mutex_lock(&gDriverLock);   }
	~DriverSmartLock() { mutex_unlock(&gDriverLock); }
};


BeceemDevice *
create_beceem_device(usb_device device)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error: Problem getting USB device descriptor.\n");
		return NULL;
	}

	#define IDS(__vendor, __product) (((__vendor) << 16) | (__product))

	switch(IDS(deviceDescriptor->vendor_id, deviceDescriptor->product_id)) {
		case IDS(0x0489, 0xe016):
			return new BeceemDevice(device,
				"Ubee WiMAX Mobile USB - PXU1900 (Clear)");
		case IDS(0x0489, 0xe017):
			return new BeceemDevice(device,
				"Ubee WiMAX Mobile USB - PXU1901 (Sprint)");
		case IDS(0x198f, 0x0210):
			return new BeceemDevice(device,
				"Motorola USBw 100 WiMAX CPE");
		case IDS(0x198f, 0x0220):
			return new BeceemDevice(device,
				"Beceem Mobile USB WiMAX CPE");
		case IDS(0x19d2, 0x0007):
			return new BeceemDevice(device,
				"ZTE TU25 WiMAX Adapter [Beceem BCS200]");
	}
	return NULL;
}


status_t
usb_beceem_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	DriverSmartLock driverLock; // released on exit

	// check if this is a replug of an existing device first
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gBeceemDevices[i] == NULL)
			continue;

		if (gBeceemDevices[i]->CompareAndReattach(device) != B_OK)
			continue;

		TRACE("Debug: The device is plugged back. Use entry at %ld.\n", i);
		*cookie = gBeceemDevices[i];
		return B_OK;
	}

	// no such device yet, create a new one
	BeceemDevice *beceemDevice = create_beceem_device(device);
	if (beceemDevice == 0) {
		return ENODEV;
	}

	status_t status = beceemDevice->InitCheck();
	if (status < B_OK) {
		delete beceemDevice;
		return status;
	}

	status = beceemDevice->SetupDevice(false);
	if (status < B_OK) {
		delete beceemDevice;
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gBeceemDevices[i] != NULL)
			continue;

		gBeceemDevices[i] = beceemDevice;
		*cookie = beceemDevice;

		TRACE("Debug: New device is added at %ld.\n", i);
		return B_OK;
	}

	// no space for the device
	TRACE_ALWAYS("Error: no more device entries availble.\n");

	delete beceemDevice;
	return B_ERROR;
}


status_t
usb_beceem_device_removed(void *cookie)
{

	TRACE("Debug: device was removed.\n");
	DriverSmartLock driverLock; // released on exit

	BeceemDevice *device = (BeceemDevice *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gBeceemDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				TRACE("Debug: Device %ld will be deleted when freed.\n", i);
				device->Removed();
			} else {
				TRACE("Debug: Device at %ld will be deleted.\n", i);
				gBeceemDevices[i] = NULL;
				delete device;
				TRACE("Debug: Device at %ld deleted.\n", i);
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
		gBeceemDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_beceem_device_added,
		&usb_beceem_device_removed
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
		if (gBeceemDevices[i]) {
			delete gBeceemDevices[i];
			gBeceemDevices[i] = NULL;
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
usb_beceem_open(const char *name, uint32 flags, void **cookie)
{
	DriverSmartLock driverLock; // released on exit

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gBeceemDevices[index]) {
		status = gBeceemDevices[index]->Open(flags);
		*cookie = gBeceemDevices[index];
	}

	return status;
}


static status_t
usb_beceem_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	BeceemDevice *device = (BeceemDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_beceem_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	BeceemDevice *device = (BeceemDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_beceem_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	BeceemDevice *device = (BeceemDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_beceem_close(void *cookie)
{
	BeceemDevice *device = (BeceemDevice *)cookie;
	return device->Close();
}


static status_t
usb_beceem_free(void *cookie)
{
	TRACE("Debug: Device to be freed\n");
	BeceemDevice *device = (BeceemDevice *)cookie;

	DriverSmartLock driverLock; // released on exit

	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gBeceemDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			TRACE("Debug: Device was removed, but it was open.\n");
			gBeceemDevices[i] = NULL;
			delete device;
			TRACE("Debug: Device at %ld deleted.\n", i);
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
		if (gBeceemDevices[i] == NULL)
			continue;

		gDeviceNames[deviceCount] = (char *)malloc(strlen(sDeviceBaseName) + 4);
		if (gDeviceNames[deviceCount]) {
			sprintf(gDeviceNames[deviceCount], "%s%ld", sDeviceBaseName, i);
			TRACE("Debug: publishing %s\n", gDeviceNames[deviceCount]);
			deviceCount++;
		} else
			TRACE_ALWAYS("Error: out of memory allocating device name.\n");
	}

	gDeviceNames[deviceCount] = NULL;
	return (const char **)&gDeviceNames[0];
}


device_hooks *
find_device(const char *name)
{
	static device_hooks deviceHooks = {
		usb_beceem_open,
		usb_beceem_close,
		usb_beceem_free,
		usb_beceem_control,
		usb_beceem_read,
		usb_beceem_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}


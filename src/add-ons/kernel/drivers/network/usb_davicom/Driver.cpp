/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2008, 2011 Siarzhuk Zharski <imker@gmx.li>
 *	Copyright (c) 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 */


#include "Driver.h"

#include <stdio.h>

#include <lock.h>
#include <util/AutoLock.h>

#include "DavicomDevice.h"
#include "Settings.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_davicom/";
DavicomDevice *gDavicomDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;
mutex gDriverLock;


// IMPORTANT: keep entries sorted by ids to let the
// binary search lookup procedure work correctly !!!
DeviceInfo gSupportedDevices[] = {
	{ { 0x01e1, 0x9601 }, "Noname DM9601" },
	{ { 0x07aa, 0x9601 }, "Corega FEther USB-TXC" },
	{ { 0x0a46, 0x0268 }, "ShanTou ST268 USB NIC" },
	{ { 0x0a46, 0x6688 }, "ZT6688 USB NIC" },
	{ { 0x0a46, 0x8515 }, "ADMtek ADM8515 USB NIC" },
	{ { 0x0a46, 0x9000 }, "DM9000E" },
	{ { 0x0a46, 0x9601 }, "Davicom DM9601" },
	{ { 0x0a47, 0x9601 }, "Hirose USB-100" },
	{ { 0x0fe6, 0x8101 }, "Sunrising SR9600" },
	{ { 0x0fe6, 0x9700 }, "Kontron DM9601" }
};


DavicomDevice *
lookup_and_create_device(usb_device device)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error of getting USB device descriptor.\n");
		return NULL;
	}

	TRACE("trying %#06x:%#06x.\n",
			deviceDescriptor->vendor_id, deviceDescriptor->product_id);

	// use binary search to lookup device in table
	uint32 id = deviceDescriptor->vendor_id << 16
					| deviceDescriptor->product_id;
	int left  = -1;
	int right = _countof(gSupportedDevices);
	while ((right - left) > 1) {
		int i = (left + right) / 2;
		((gSupportedDevices[i].Key() < id) ? left : right) = i;
	}

	if (gSupportedDevices[right].Key() == id)
		return new DavicomDevice(device, gSupportedDevices[right]);

	TRACE_ALWAYS("Search for %#x failed %d-%d.\n", id, left, right);
	return NULL;
}


status_t
usb_davicom_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	MutexLocker lock(gDriverLock); // released on exit

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
	DavicomDevice *davicomDevice = lookup_and_create_device(device);
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
	MutexLocker lock(gDriverLock); // released on exit

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


// #pragma mark -


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

	const size_t count = _countof(gSupportedDevices);
	static usb_support_descriptor sDescriptors[count] = {{ 0 }};

	for (size_t i = 0; i < count; i++) {
		sDescriptors[i].vendor  = gSupportedDevices[i].VendorId();
		sDescriptors[i].product = gSupportedDevices[i].ProductId();
	}

	gUSBModule->register_driver(DRIVER_NAME, sDescriptors, count, NULL);
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
	MutexLocker lock(gDriverLock); // released on exit

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

	MutexLocker lock(gDriverLock); // released on exit

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

	MutexLocker lock(gDriverLock); // released on exit

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
			TRACE_ALWAYS("Error: out of memory during allocating dev.name.\n");
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
		NULL,				// select
		NULL				// deselect
	};

	return &deviceHooks;
}


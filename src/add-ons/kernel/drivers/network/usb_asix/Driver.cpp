/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Driver.h"

#include <stdio.h>

#include <lock.h> // for mutex
#include <util/AutoLock.h>

#include "AX88172Device.h"
#include "AX88178Device.h"
#include "AX88772Device.h"
#include "Settings.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_asix/";
ASIXDevice *gASIXDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;
mutex gDriverLock;


// IMPORTANT: keep entries sorted by ids to let the
// binary search lookup procedure work correctly !!!
DeviceInfo gSupportedDevices[] = {
	{ { { 0x0411, 0x003d } }, DeviceInfo::AX88172, "Melco LUA-U2-KTX" },
	{ { { 0x04bb, 0x0930 } }, DeviceInfo::AX88178, "I/O Data ETG-US2" },
	{ { { 0x04f1, 0x3008 } }, DeviceInfo::AX88172, "JVC MP-PRX1" },
	{ { { 0x050d, 0x5055 } }, DeviceInfo::AX88178, "Belkin F5D5055" },
	{ { { 0x0557, 0x2009 } }, DeviceInfo::AX88172, "ATEN UC-210T" },
	{ { { 0x05ac, 0x1402 } }, DeviceInfo::AX88772, "Apple A1277" },
	{ { { 0x077b, 0x2226 } }, DeviceInfo::AX88172, "LinkSys USB 2.0" },
	{ { { 0x07aa, 0x0017 } }, DeviceInfo::AX88172, "Corega USB2TX" },
	{ { { 0x07b8, 0x420a } }, DeviceInfo::AX88172, "ABOCOM UF200" },
	{ { { 0x07d1, 0x3c05 } }, DeviceInfo::AX88772, "D-Link DUB-E100 rev.B1" },
	{ { { 0x0846, 0x1040 } }, DeviceInfo::AX88172, "NetGear USB 2.0 Ethernet" },
	{ { { 0x086e, 0x1920 } }, DeviceInfo::AX88172, "System TALKS SGC-X2UL" },
	{ { { 0x08dd, 0x90ff } }, DeviceInfo::AX88172, "Billionton USB2AR" },
	{ { { 0x0b95, 0x1720 } }, DeviceInfo::AX88172, "ASIX 88172 10/100" },
	{ { { 0x0b95, 0x1780 } }, DeviceInfo::AX88178, "ASIX 88178 10/100/1000" },
	{ { { 0x0b95, 0x7720 } }, DeviceInfo::AX88772, "ASIX 88772 10/100" },
	{ { { 0x0df6, 0x061c } }, DeviceInfo::AX88178, "Sitecom LN-028" },
	{ { { 0x1189, 0x0893 } }, DeviceInfo::AX88172, "Acer C&M EP-1427X-2" },
	{ { { 0x13b1, 0x0018 } }, DeviceInfo::AX88772, "Linksys USB200M rev.2" },
	{ { { 0x14ea, 0xab11 } }, DeviceInfo::AX88178, "Planex GU-1000T" },
	{ { { 0x1557, 0x7720 } }, DeviceInfo::AX88772, "OQO 01+ Ethernet" },
	{ { { 0x1631, 0x6200 } }, DeviceInfo::AX88172, "GoodWay USB2Ethernet" },
	{ { { 0x1737, 0x0039 } }, DeviceInfo::AX88178, "LinkSys 1000" },
	{ { { 0x2001, 0x1A00 } }, DeviceInfo::AX88172, "D-Link DUB-E100" },
	{ { { 0x2001, 0x3c05 } }, DeviceInfo::AX88772, "D-Link DUB-E100 rev.B1" },
	{ { { 0x6189, 0x182d } }, DeviceInfo::AX88172, "Sitecom LN-029" }
}; 


ASIXDevice *
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
	DeviceInfo::Id id = { { deviceDescriptor->vendor_id, 
							deviceDescriptor->product_id } };
	int left  = -1;
	int right = _countof(gSupportedDevices);
	while ((right - left) > 1) {
		int i = (left + right) / 2;
		((gSupportedDevices[i].Key() < id.fKey) ? left : right) = i;
	}

	if (gSupportedDevices[right].Key() == id.fKey) {
		switch (gSupportedDevices[right].fType) {
			case DeviceInfo::AX88172:
				return new AX88172Device(device, gSupportedDevices[right]);
			case DeviceInfo::AX88772:
				return new AX88772Device(device, gSupportedDevices[right]);
			case DeviceInfo::AX88178:
				return new AX88178Device(device, gSupportedDevices[right]);
			default:
				TRACE_ALWAYS("Unknown device type:%#x ignored.\n",
						static_cast<int>(gSupportedDevices[right].fType));
				break;
		}
	}

	return NULL;
}


status_t
usb_asix_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	MutexLocker lock(gDriverLock); // released on exit

	// check if this is a replug of an existing device first
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gASIXDevices[i] == NULL)
			continue;

		if (gASIXDevices[i]->CompareAndReattach(device) != B_OK)
			continue;

		TRACE("The device is plugged back. Use entry at %ld.\n", i);
		*cookie = gASIXDevices[i];
		return B_OK;
	}

	// no such device yet, create a new one
	ASIXDevice *asixDevice = lookup_and_create_device(device);
	if (asixDevice == 0) {
		return ENODEV;
	}

	status_t status = asixDevice->InitCheck();
	if (status < B_OK) {
		delete asixDevice;
		return status;
	}

	status = asixDevice->SetupDevice(false);
	if (status < B_OK) {
		delete asixDevice;
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gASIXDevices[i] != NULL)
			continue;

		gASIXDevices[i] = asixDevice;
		*cookie = asixDevice;

		TRACE("New device is added at %ld.\n", i);
		return B_OK;
	}

	// no space for the device
	TRACE_ALWAYS("Error: no more device entries availble.\n");

	delete asixDevice;
	return B_ERROR;
}


status_t
usb_asix_device_removed(void *cookie)
{
	MutexLocker lock(gDriverLock); // released on exit

	ASIXDevice *device = (ASIXDevice *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gASIXDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gASIXDevices[i] = NULL;
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
		gASIXDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_asix_device_added,
		&usb_asix_device_removed
	};

	const size_t count = _countof(gSupportedDevices);
	static usb_support_descriptor sDescriptors[count] = {{ 0 }};

	for(size_t i = 0; i < count; i++) {
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
		if (gASIXDevices[i]) {
			delete gASIXDevices[i];
			gASIXDevices[i] = NULL;
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
usb_asix_open(const char *name, uint32 flags, void **cookie)
{
	MutexLocker lock(gDriverLock); // released on exit

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gASIXDevices[index]) {
		status = gASIXDevices[index]->Open(flags);
		*cookie = gASIXDevices[index];
	}

	return status;
}


static status_t
usb_asix_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	ASIXDevice *device = (ASIXDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_asix_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	ASIXDevice *device = (ASIXDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_asix_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	ASIXDevice *device = (ASIXDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_asix_close(void *cookie)
{
	ASIXDevice *device = (ASIXDevice *)cookie;
	return device->Close();
}


static status_t
usb_asix_free(void *cookie)
{
	ASIXDevice *device = (ASIXDevice *)cookie;

	MutexLocker lock(gDriverLock); // released on exit

	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gASIXDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			gASIXDevices[i] = NULL;
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
		if (gASIXDevices[i] == NULL)
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
		usb_asix_open,
		usb_asix_close,
		usb_asix_free,
		usb_asix_control,
		usb_asix_read,
		usb_asix_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}


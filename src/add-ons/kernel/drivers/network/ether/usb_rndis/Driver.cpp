/*
	Driver for USB RNDIS Network devices
	Copyright (C) 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lock.h>

#include "Driver.h"
#include "RNDISDevice.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_rndis/";
RNDISDevice *gRNDISDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;
mutex gDriverLock;


status_t
usb_rndis_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	// check if this is a replug of an existing device first
	mutex_lock(&gDriverLock);

	// no such device yet, create a new one
	RNDISDevice *rndisDevice = new RNDISDevice(device);
	status_t status = rndisDevice->InitCheck();
	if (status < B_OK) {
		delete rndisDevice;
		mutex_unlock(&gDriverLock);
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gRNDISDevices[i] != NULL)
			continue;

		gRNDISDevices[i] = rndisDevice;
		*cookie = rndisDevice;

		TRACE_ALWAYS("rndis device %" B_PRId32 " added\n", i);
		mutex_unlock(&gDriverLock);
		return B_OK;
	}

	// no space for the device
	delete rndisDevice;
	mutex_unlock(&gDriverLock);
	return B_ERROR;
}


status_t
usb_rndis_device_removed(void *cookie)
{
	mutex_lock(&gDriverLock);

	RNDISDevice *device = (RNDISDevice *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gRNDISDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gRNDISDevices[i] = NULL;
				delete device;
			}
			break;
		}
	}

	mutex_unlock(&gDriverLock);
	return B_OK;
}


//#pragma mark -


status_t
init_hardware()
{
	TRACE("init_hardware()\n");
	return B_OK;
}


status_t
init_driver()
{
	TRACE("init_driver()\n");
	status_t status = get_module(B_USB_MODULE_NAME,
		(module_info **)&gUSBModule);
	if (status < B_OK)
		return status;

	for (int32 i = 0; i < MAX_DEVICES; i++)
		gRNDISDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_rndis_device_added,
		&usb_rndis_device_removed
	};

	static usb_support_descriptor supportDescriptor = {
		0xE0, /* CDC - Wireless device */
		0x01, 0x03, /* RNDIS subclass/protocol */
		0, 0 /* no specific vendor or device */
	};

	gUSBModule->register_driver(DRIVER_NAME, &supportDescriptor, 1, NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	return B_OK;
}


void
uninit_driver()
{
	TRACE("uninit_driver()\n");
	gUSBModule->uninstall_notify(DRIVER_NAME);
	mutex_lock(&gDriverLock);

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gRNDISDevices[i] != NULL) {
			delete gRNDISDevices[i];
			gRNDISDevices[i] = NULL;
		}
	}

	for (int32 i = 0; gDeviceNames[i]; i++) {
		free(gDeviceNames[i]);
		gDeviceNames[i] = NULL;
	}

	mutex_destroy(&gDriverLock);
	put_module(B_USB_MODULE_NAME);
}


static status_t
usb_rndis_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s, %" B_PRIu32 ", %p)\n", name, flags, cookie);
	mutex_lock(&gDriverLock);

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gRNDISDevices[index] != NULL) {
		status = gRNDISDevices[index]->Open();
		if (status == B_OK)
			*cookie = gRNDISDevices[index];
	}

	mutex_unlock(&gDriverLock);
	return status;
}


static status_t
usb_rndis_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE("read(%p, %" B_PRIdOFF ", %p, %lu)\n", cookie, position, buffer, *numBytes);
	RNDISDevice *device = (RNDISDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_rndis_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE("write(%p, %" B_PRIdOFF ", %p, %lu)\n", cookie, position, buffer, *numBytes);
	RNDISDevice *device = (RNDISDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_rndis_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE("control(%p, %" B_PRIu32 ", %p, %lu)\n", cookie, op, buffer, length);
	RNDISDevice *device = (RNDISDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_rndis_close(void *cookie)
{
	TRACE("close(%p)\n", cookie);
	RNDISDevice *device = (RNDISDevice *)cookie;
	return device->Close();
}


static status_t
usb_rndis_free(void *cookie)
{
	TRACE("free(%p)\n", cookie);
	RNDISDevice *device = (RNDISDevice *)cookie;
	mutex_lock(&gDriverLock);
	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gRNDISDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			gRNDISDevices[i] = NULL;
			delete device;
			break;
		}
	}

	mutex_unlock(&gDriverLock);
	return status;
}


const char **
publish_devices()
{
	TRACE("publish_devices()\n");
	for (int32 i = 0; gDeviceNames[i]; i++) {
		free(gDeviceNames[i]);
		gDeviceNames[i] = NULL;
	}

	int32 deviceCount = 0;
	mutex_lock(&gDriverLock);
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gRNDISDevices[i] == NULL)
			continue;

		gDeviceNames[deviceCount] = (char *)malloc(strlen(sDeviceBaseName) + 4);
		if (gDeviceNames[deviceCount] != NULL) {
			sprintf(gDeviceNames[deviceCount], "%s%" B_PRId32, sDeviceBaseName,
				i);
			TRACE("publishing %s\n", gDeviceNames[deviceCount]);
			deviceCount++;
		} else
			TRACE_ALWAYS("publish_devices - no memory to allocate device name\n");
	}

	gDeviceNames[deviceCount] = NULL;
	mutex_unlock(&gDriverLock);
	return (const char **)&gDeviceNames[0];
}


device_hooks *
find_device(const char *name)
{
	TRACE("find_device(%s)\n", name);
	static device_hooks deviceHooks = {
		usb_rndis_open,
		usb_rndis_close,
		usb_rndis_free,
		usb_rndis_control,
		usb_rndis_read,
		usb_rndis_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}

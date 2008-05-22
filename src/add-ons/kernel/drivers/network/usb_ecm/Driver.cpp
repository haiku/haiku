/*
	Driver for USB Ethernet Control Model devices
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <lock.h> // for mutex
#else
#include "BeOSCompatibility.h" // for pseudo mutex
#endif

#include "Driver.h"
#include "ECMDevice.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_ecm/";
ECMDevice *gECMDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;
mutex gDriverLock;


status_t
usb_ecm_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	// check if this is a replug of an existing device first
	mutex_lock(&gDriverLock);
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gECMDevices[i] == NULL)
			continue;

		if (gECMDevices[i]->CompareAndReattach(device) != B_OK)
			continue;

		TRACE_ALWAYS("ecm device %ld replugged\n", i);
		*cookie = gECMDevices[i];
		mutex_unlock(&gDriverLock);
		return B_OK;
	}

	// no such device yet, create a new one
	ECMDevice *ecmDevice = new ECMDevice(device);
	status_t status = ecmDevice->InitCheck();
	if (status < B_OK) {
		delete ecmDevice;
		mutex_unlock(&gDriverLock);
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gECMDevices[i] != NULL)
			continue;

		gECMDevices[i] = ecmDevice;
		*cookie = ecmDevice;

		TRACE_ALWAYS("ecm device %ld added\n", i);
		mutex_unlock(&gDriverLock);
		return B_OK;
	}

	// no space for the device
	delete ecmDevice;
	mutex_unlock(&gDriverLock);
	return B_ERROR;
}


status_t
usb_ecm_device_removed(void *cookie)
{
	mutex_lock(&gDriverLock);

	ECMDevice *device = (ECMDevice *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gECMDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gECMDevices[i] = NULL;
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
		gECMDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_ecm_device_added,
		&usb_ecm_device_removed
	};

	static usb_support_descriptor supportDescriptor = {
		USB_INTERFACE_CLASS_CDC, /* CDC - Communication Device Class */
		USB_INTERFACE_SUBCLASS_ECM, /* ECM - Ethernet Control Model */
		0, 0, 0 /* no protocol, vendor or device */
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
		if (gECMDevices[i]) {
			delete gECMDevices[i];
			gECMDevices[i] = NULL;
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
usb_ecm_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s, %lu, %p)\n", name, flags, cookie);
	mutex_lock(&gDriverLock);

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gECMDevices[index]) {
		status = gECMDevices[index]->Open();
		*cookie = gECMDevices[index];
	}

	mutex_unlock(&gDriverLock);
	return status;
}


static status_t
usb_ecm_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE("read(%p, %Ld, %p, %lu)\n", cookie, position, buffer, *numBytes);
	ECMDevice *device = (ECMDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_ecm_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE("write(%p, %Ld, %p, %lu)\n", cookie, position, buffer, *numBytes);
	ECMDevice *device = (ECMDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_ecm_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE("control(%p, %lu, %p, %lu)\n", cookie, op, buffer, length);
	ECMDevice *device = (ECMDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_ecm_close(void *cookie)
{
	TRACE("close(%p)\n", cookie);
	ECMDevice *device = (ECMDevice *)cookie;
	return device->Close();
}


static status_t
usb_ecm_free(void *cookie)
{
	TRACE("free(%p)\n", cookie);
	ECMDevice *device = (ECMDevice *)cookie;
	mutex_lock(&gDriverLock);
	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gECMDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			gECMDevices[i] = NULL;
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
		if (gECMDevices[i] == NULL)
			continue;

		gDeviceNames[deviceCount] = (char *)malloc(strlen(sDeviceBaseName) + 4);
		if (gDeviceNames[deviceCount]) {
			sprintf(gDeviceNames[deviceCount], "%s%ld", sDeviceBaseName, i);
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
		usb_ecm_open,
		usb_ecm_close,
		usb_ecm_free,
		usb_ecm_control,
		usb_ecm_read,
		usb_ecm_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}

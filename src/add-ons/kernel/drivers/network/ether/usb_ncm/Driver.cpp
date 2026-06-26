/*
	Driver for USB NCM Network devices
	Copyright (C) 2022-2026 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#include <lock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Driver.h"
#include "NCMDevice.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "net/usb_ncm/";
NCMDevice *gNCMDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];
usb_module_info *gUSBModule = NULL;
mutex gDriverLock;


status_t
usb_ncm_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	mutex_lock(&gDriverLock);

	// create the device
	NCMDevice *ncmDevice = new NCMDevice(device);
	status_t status = ncmDevice->InitCheck();
	if (status < B_OK) {
		TRACE_ALWAYS("Failed to initialize NCM device: %s\n", strerror(status));
		delete ncmDevice;
		mutex_unlock(&gDriverLock);
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gNCMDevices[i] != NULL)
			continue;

		gNCMDevices[i] = ncmDevice;
		*cookie = ncmDevice;

		TRACE_ALWAYS("ncm device %" B_PRId32 " added\n", i);
		mutex_unlock(&gDriverLock);
		return B_OK;
	}

	// no space for the device
	delete ncmDevice;
	mutex_unlock(&gDriverLock);
	return B_ERROR;
}


status_t
usb_ncm_device_removed(void *cookie)
{
	mutex_lock(&gDriverLock);

	NCMDevice *device = (NCMDevice *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gNCMDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gNCMDevices[i] = NULL;
				delete device;
			}
			break;
		}
	}

	mutex_unlock(&gDriverLock);
	return B_OK;
}


// #pragma mark -


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
	status_t status = get_module(B_USB_MODULE_NAME, (module_info **)&gUSBModule);
	if (status < B_OK)
		return status;

	for (int32 i = 0; i < MAX_DEVICES; i++)
		gNCMDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_ncm_device_added,
		&usb_ncm_device_removed
	};

	static usb_support_descriptor supportDescriptors[] = {
		{
			.dev_class = USB_CDC_COMMUNICATION_INTERFACE_CLASS,
			.dev_subclass = USB_CDC_COMMUNICATION_INTERFACE_NCM_SUBCLASS,
			.dev_protocol = 0x00, /* None */
			0, 0 /* no specific vendor or device */
		},
	};

	gUSBModule->register_driver(DRIVER_NAME, supportDescriptors, B_COUNT_OF(supportDescriptors),
		NULL);
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
		if (gNCMDevices[i] != NULL) {
			delete gNCMDevices[i];
			gNCMDevices[i] = NULL;
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
usb_ncm_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s, %" B_PRIu32 ", %p)\n", name, flags, cookie);
	mutex_lock(&gDriverLock);

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gNCMDevices[index] != NULL) {
		status = gNCMDevices[index]->Open();
		if (status == B_OK)
			*cookie = gNCMDevices[index];
	}

	mutex_unlock(&gDriverLock);
	return status;
}


static status_t
usb_ncm_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	//TRACE("read(%p, %" B_PRIdOFF ", %p, %lu)\n", cookie, position, buffer, *numBytes);
	NCMDevice *device = (NCMDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_ncm_write(void *cookie, off_t position, const void *buffer, size_t *numBytes)
{
	//TRACE("write(%p, %" B_PRIdOFF ", %p, %lu)\n", cookie, position, buffer, *numBytes);
	NCMDevice *device = (NCMDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_ncm_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE("control(%p, %" B_PRIu32 ", %p, %lu)\n", cookie, op, buffer, length);
	NCMDevice *device = (NCMDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_ncm_close(void *cookie)
{
	TRACE("close(%p)\n", cookie);
	NCMDevice *device = (NCMDevice *)cookie;
	return device->Close();
}


static status_t
usb_ncm_free(void *cookie)
{
	TRACE("free(%p)\n", cookie);
	NCMDevice *device = (NCMDevice *)cookie;
	mutex_lock(&gDriverLock);
	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gNCMDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			gNCMDevices[i] = NULL;
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
		if (gNCMDevices[i] == NULL)
			continue;

		gDeviceNames[deviceCount] = (char *)malloc(strlen(sDeviceBaseName) + 4);
		if (gDeviceNames[deviceCount] != NULL) {
			sprintf(gDeviceNames[deviceCount], "%s%" B_PRId32, sDeviceBaseName, i);
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
		usb_ncm_open,
		usb_ncm_close,
		usb_ncm_free,
		usb_ncm_control,
		usb_ncm_read,
		usb_ncm_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}

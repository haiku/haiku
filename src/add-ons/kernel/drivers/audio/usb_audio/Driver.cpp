/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lock.h> // for mutex

#include "Driver.h"
#include "Settings.h"
#include "Device.h"
#include "USB_audio_spec.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;


static const char *sDeviceBaseName = "audio/hmulti/USB Audio/";
Device *gDevices[MAX_DEVICES];
char *gDeviceNames[MAX_DEVICES + 1];

usb_module_info *gUSBModule = NULL;


mutex gDriverLock;
// auto - release helper class
class DriverSmartLock {
public:
	DriverSmartLock()	{ mutex_lock(&gDriverLock);		}
	~DriverSmartLock()	{ mutex_unlock(&gDriverLock);	}
};


status_t
usb_audio_device_added(usb_device device, void **cookie)
{
	*cookie = NULL;

	DriverSmartLock driverLock; // released on exit

	// check if this is a replug of an existing device first
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == NULL)
			continue;

		if (gDevices[i]->CompareAndReattach(device) != B_OK)
			continue;

		TRACE("The device is plugged back. Use entry at %ld.\n", i);
		*cookie = gDevices[i];
		return B_OK;
	}

	// no such device yet, create a new one
	Device *audioDevice = new Device(device);
	if (audioDevice == 0) {
		return ENODEV;
	}

	status_t status = audioDevice->InitCheck();
	if (status < B_OK) {
		delete audioDevice;
		return status;
	}

	status = audioDevice->SetupDevice(false);
	if (status < B_OK) {
		delete audioDevice;
		return status;
	}

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] != NULL)
			continue;

		gDevices[i] = audioDevice;
		*cookie = audioDevice;

		TRACE("New device is added at %ld.\n", i);
		return B_OK;
	}

	// no space for the device
	TRACE_ALWAYS("Error: no more device entries availble.\n");

	delete audioDevice;
	return B_ERROR;
}


status_t
usb_audio_device_removed(void *cookie)
{
	DriverSmartLock driverLock; // released on exit

	Device *device = (Device *)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gDevices[i] = NULL;
				delete device;
				TRACE("Device at %ld deleted.\n", i);
			}
			break;
		}
	}

	return B_OK;
}


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
		gDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_audio_device_added,
		&usb_audio_device_removed
	};

	static usb_support_descriptor supportedDevices[] = {
		{UAS_AUDIO, 0, 0, 0, 0 }
	};

	gUSBModule->register_driver(DRIVER_NAME, supportedDevices, 0, NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	return B_OK;
}


void
uninit_driver()
{
	gUSBModule->uninstall_notify(DRIVER_NAME);
	mutex_lock(&gDriverLock);

	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i]) {
			delete gDevices[i];
			gDevices[i] = NULL;
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
usb_audio_open(const char *name, uint32 flags, void **cookie)
{
	DriverSmartLock driverLock; // released on exit

	*cookie = NULL;
	status_t status = ENODEV;
	int32 index = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (index >= 0 && index < MAX_DEVICES && gDevices[index]) {
		status = gDevices[index]->Open(flags);
		*cookie = gDevices[index];
	}

	return status;
}


static status_t
usb_audio_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	Device *device = (Device *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_audio_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	Device *device = (Device *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_audio_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	Device *device = (Device *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_audio_close(void *cookie)
{
	Device *device = (Device *)cookie;
	return device->Close();
}


static status_t
usb_audio_free(void *cookie)
{
	Device *device = (Device *)cookie;

	DriverSmartLock driverLock; // released on exit

	status_t status = device->Free();
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == device) {
			// the device is removed already but as it was open the
			// removed hook has not deleted the object
			gDevices[i] = NULL;
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
		if (gDevices[i] == NULL)
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
		usb_audio_open,
		usb_audio_close,
		usb_audio_free,
		usb_audio_control,
		usb_audio_read,
		usb_audio_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	return &deviceHooks;
}


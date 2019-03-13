/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009-13 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */

#include "Driver.h"

#include <AutoLock.h>
#include <usb/USB_audio.h>

#include "Device.h"
#include "Settings.h"


static const char* sDeviceBaseName = "audio/hmulti/usb/";

usb_module_info* gUSBModule = NULL;

Device* gDevices[MAX_DEVICES];
char* gDeviceNames[MAX_DEVICES + 1];

mutex gDriverLock;
int32 api_version = B_CUR_DRIVER_API_VERSION;


status_t
usb_audio_device_added(usb_device device, void** cookie)
{
	*cookie = NULL;

	MutexLocker _(gDriverLock);

	// check if this is a replug of an existing device first
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == NULL)
			continue;

		if (gDevices[i]->CompareAndReattach(device) != B_OK)
			continue;

		TRACE(INF, "The device is plugged back. Use entry at %ld.\n", i);
		*cookie = gDevices[i];
		return B_OK;
	}

	// no such device yet, create a new one
	Device* audioDevice = new(std::nothrow) Device(device);
	if (audioDevice == 0)
		return ENODEV;

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

		TRACE(INF, "New device is added at %ld.\n", i);
		return B_OK;
	}

	// no space for the device
	TRACE(ERR, "Error: no more device entries availble.\n");

	delete audioDevice;
	return B_ERROR;
}


status_t
usb_audio_device_removed(void* cookie)
{
	MutexLocker _(gDriverLock);

	Device* device = (Device*)cookie;
	for (int32 i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				gDevices[i] = NULL;
				delete device;
				TRACE(INF, "Device at %ld deleted.\n", i);
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
		(module_info**)&gUSBModule);
	if (status < B_OK)
		return status;

	load_settings();

	TRACE(ERR, "%s\n", kVersion); // TODO: always???

	for (int32 i = 0; i < MAX_DEVICES; i++)
		gDevices[i] = NULL;

	gDeviceNames[0] = NULL;
	mutex_init(&gDriverLock, DRIVER_NAME"_devices");

	static usb_notify_hooks notifyHooks = {
		&usb_audio_device_added,
		&usb_audio_device_removed
	};

	static usb_support_descriptor supportedDevices[] = {
		{ USB_AUDIO_INTERFACE_AUDIO_CLASS, 0, 0, 0, 0 }
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
usb_audio_open(const char* name, uint32 flags, void** cookie)
{
	MutexLocker _(gDriverLock);

	*cookie = NULL;
	status_t status = ENODEV;
	for (int32 i = 0; i < MAX_DEVICES && gDevices[i] != NULL; i++) {
		if (strcmp(gDeviceNames[i], name) == 0) {
			status = gDevices[i]->Open(flags);
			*cookie = gDevices[i];
			break;
		}
	}

	return status;
}


static status_t
usb_audio_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	Device* device = (Device*)cookie;
	return device->Read((uint8*)buffer, numBytes);
}


static status_t
usb_audio_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	Device* device = (Device*)cookie;
	return device->Write((const uint8*)buffer, numBytes);
}


static status_t
usb_audio_control(void* cookie, uint32 op, void* buffer, size_t length)
{
	Device* device = (Device*)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_audio_close(void* cookie)
{
	Device* device = (Device*)cookie;
	return device->Close();
}


static status_t
usb_audio_free(void* cookie)
{
	Device* device = (Device*)cookie;
	return device->Free();
}


const char**
publish_devices()
{
	MutexLocker _(gDriverLock);

	for (int32 i = 0; gDeviceNames[i]; i++) {
		free(gDeviceNames[i]);
		gDeviceNames[i] = NULL;
	}

	int32 deviceCount = 0;
	for (size_t i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == NULL)
			continue;

		gDeviceNames[deviceCount] = (char*)malloc(strlen(sDeviceBaseName) + 4);
		if (gDeviceNames[deviceCount]) {
			sprintf(gDeviceNames[deviceCount], "%s%ld", sDeviceBaseName, i + 1);
			TRACE(INF, "publishing %s\n", gDeviceNames[deviceCount]);
			deviceCount++;
		} else
			TRACE(ERR, "Error: out of memory during allocating device name.\n");
	}

	gDeviceNames[deviceCount] = NULL;
	return (const char**)&gDeviceNames[0];
}


device_hooks*
find_device(const char* name)
{
	static device_hooks deviceHooks = {
		usb_audio_open,
		usb_audio_close,
		usb_audio_free,
		usb_audio_control,
		usb_audio_read,
		usb_audio_write,
		NULL,				// select
		NULL				// deselect
	};

	return &deviceHooks;
}


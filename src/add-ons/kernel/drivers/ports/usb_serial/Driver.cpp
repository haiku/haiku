/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include <KernelExport.h>
#include <Drivers.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "Driver.h"
#include "SerialDevice.h"
#include "USB3.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDeviceBaseName = "ports/usb";
SerialDevice *gSerialDevices[DEVICES_COUNT];
char *gDeviceNames[DEVICES_COUNT + 1];
usb_module_info *gUSBModule = NULL;
tty_module_info *gTTYModule = NULL;
sem_id gDriverLock = -1;


status_t
usb_serial_device_added(usb_device device, void **cookie)
{
	TRACE_FUNCALLS("> usb_serial_device_added(0x%08x, 0x%08x)\n", device, cookie);

	status_t status = B_OK;
	const usb_device_descriptor *descriptor
		= gUSBModule->get_device_descriptor(device);

	TRACE_ALWAYS("probing device: 0x%04x/0x%04x\n", descriptor->vendor_id,
		descriptor->product_id);

	*cookie = NULL;
	SerialDevice *serialDevice = SerialDevice::MakeDevice(device,
		descriptor->vendor_id, descriptor->product_id);

	const usb_configuration_info *configuration;
	for (int i = 0; i < descriptor->num_configurations; i++) {
		configuration = gUSBModule->get_nth_configuration(device, i);
		if (configuration == NULL)
			continue;

		status = serialDevice->AddDevice(configuration);
		if (status == B_OK) {
			// Found!
			break;
		}
	}

	if (status < B_OK) {
		delete serialDevice;
		return status;
	}


	acquire_sem(gDriverLock);
	for (int32 i = 0; i < DEVICES_COUNT; i++) {
		if (gSerialDevices[i] != NULL)
			continue;

		status = serialDevice->Init();
		if (status < B_OK) {
			delete serialDevice;
			return status;
		}

		gSerialDevices[i] = serialDevice;
		*cookie = serialDevice;

		release_sem(gDriverLock);
		TRACE_ALWAYS("%s (0x%04x/0x%04x) added\n", serialDevice->Description(),
			descriptor->vendor_id, descriptor->product_id);
		return B_OK;
	}

	release_sem(gDriverLock);
	return B_ERROR;
}


status_t
usb_serial_device_removed(void *cookie)
{
	TRACE_FUNCALLS("> usb_serial_device_removed(0x%08x)\n", cookie);

	acquire_sem(gDriverLock);

	SerialDevice *device = (SerialDevice *)cookie;
	for (int32 i = 0; i < DEVICES_COUNT; i++) {
		if (gSerialDevices[i] == device) {
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				delete device;
				gSerialDevices[i] = NULL;
			}
			break;
		}
	}

	release_sem(gDriverLock);
	TRACE_FUNCRET("< usb_serial_device_removed() returns\n");
	return B_OK;
}


//#pragma mark -


/* init_hardware - called once the first time the driver is loaded */
status_t
init_hardware()
{
	TRACE("init_hardware\n");
	return B_OK;
}


/* init_driver - called every time the driver is loaded. */
status_t
init_driver()
{
	load_settings();
	create_log_file();

	TRACE_FUNCALLS("> init_driver()\n");

	status_t status = get_module(B_TTY_MODULE_NAME, (module_info **)&gTTYModule);
	if (status < B_OK)
		return status;

	status = get_module(B_USB_MODULE_NAME, (module_info **)&gUSBModule);
	if (status < B_OK) {
		put_module(B_TTY_MODULE_NAME);
		return status;
	}

	for (int32 i = 0; i < DEVICES_COUNT; i++)
		gSerialDevices[i] = NULL;

	gDeviceNames[0] = NULL;

	gDriverLock = create_sem(1, DRIVER_NAME"_devices_table_lock");
	if (gDriverLock < B_OK) {
		put_module(B_USB_MODULE_NAME);
		put_module(B_TTY_MODULE_NAME);
		return gDriverLock;
	}

	static usb_notify_hooks notifyHooks = {
		&usb_serial_device_added,
		&usb_serial_device_removed
	};

	gUSBModule->register_driver(DRIVER_NAME, NULL, 0, NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	TRACE_FUNCRET("< init_driver() returns\n");
	return B_OK;
}


/* uninit_driver - called every time the driver is unloaded */
void
uninit_driver()
{
	TRACE_FUNCALLS("> uninit_driver()\n");

	gUSBModule->uninstall_notify(DRIVER_NAME);
	acquire_sem(gDriverLock);

	for (int32 i = 0; i < DEVICES_COUNT; i++) {
		if (gSerialDevices[i]) {
			delete gSerialDevices[i];
			gSerialDevices[i] = NULL;
		}
	}

	for (int32 i = 0; gDeviceNames[i]; i++)
		free(gDeviceNames[i]);

	delete_sem(gDriverLock);
	put_module(B_USB_MODULE_NAME);
	put_module(B_TTY_MODULE_NAME);

	TRACE_FUNCRET("< uninit_driver() returns\n");
}


bool
usb_serial_service(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	TRACE_FUNCALLS("> usb_serial_service(%p, 0x%08lx, %p, %lu)\n", tty,
		op, buffer, length);

	for (int32 i = 0; i < DEVICES_COUNT; i++) {
		if (gSerialDevices[i]
			&& gSerialDevices[i]->Service(tty, op, buffer, length)) {
			TRACE_FUNCRET("< usb_serial_service() returns: true\n");
			return true;
		}
	}

	TRACE_FUNCRET("< usb_serial_service() returns: false\n");
	return false;
}


/* usb_serial_open - handle open() calls */
static status_t
usb_serial_open(const char *name, uint32 flags, void **cookie)
{
	TRACE_FUNCALLS("> usb_serial_open(%s, 0x%08x, 0x%08x)\n", name, flags, cookie);
	acquire_sem(gDriverLock);
	status_t status = ENODEV;

	*cookie = NULL;
	int i = strtol(name + strlen(sDeviceBaseName), NULL, 10);
	if (i >= 0 && i < DEVICES_COUNT && gSerialDevices[i]) {
		status = gSerialDevices[i]->Open(flags);
		*cookie = gSerialDevices[i];
	}

	release_sem(gDriverLock);
	TRACE_FUNCRET("< usb_serial_open() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_read - handle read() calls */
static status_t
usb_serial_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE_FUNCALLS("> usb_serial_read(0x%08x, %Ld, 0x%08x, %d)\n", cookie,
		position, buffer, *numBytes);
	SerialDevice *device = (SerialDevice *)cookie;
	status_t status = device->Read((char *)buffer, numBytes);
	TRACE_FUNCRET("< usb_serial_read() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_write - handle write() calls */
static status_t
usb_serial_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE_FUNCALLS("> usb_serial_write(0x%08x, %Ld, 0x%08x, %d)\n", cookie,
		position, buffer, *numBytes);
	SerialDevice *device = (SerialDevice *)cookie;
	status_t status = device->Write((const char *)buffer, numBytes);
	TRACE_FUNCRET("< usb_serial_write() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_control - handle ioctl calls */
static status_t
usb_serial_control(void *cookie, uint32 op, void *arg, size_t length)
{
	TRACE_FUNCALLS("> usb_serial_control(0x%08x, 0x%08x, 0x%08x, %d)\n",
		cookie, op, arg, length);
	SerialDevice *device = (SerialDevice *)cookie;
	status_t status = device->Control(op, arg, length);
	TRACE_FUNCRET("< usb_serial_control() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_select - handle select start */
static status_t
usb_serial_select(void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	TRACE_FUNCALLS("> usb_serial_select(0x%08x, 0x%08x, 0x%08x, %p)\n",
		cookie, event, ref, sync);
	SerialDevice *device = (SerialDevice *)cookie;
	status_t status = device->Select(event, ref, sync);
	TRACE_FUNCRET("< usb_serial_select() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_deselect - handle select exit */
static status_t
usb_serial_deselect(void *cookie, uint8 event, selectsync *sync)
{
	TRACE_FUNCALLS("> usb_serial_deselect(0x%08x, 0x%08x, %p)\n",
		cookie, event, sync);
	SerialDevice *device = (SerialDevice *)cookie;
	status_t status = device->DeSelect(event, sync);
	TRACE_FUNCRET("< usb_serial_deselect() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_close - handle close() calls */
static status_t
usb_serial_close(void *cookie)
{
	TRACE_FUNCALLS("> usb_serial_close(0x%08x)\n", cookie);
	SerialDevice *device = (SerialDevice *)cookie;
	status_t status = device->Close();
	TRACE_FUNCRET("< usb_serial_close() returns: 0x%08x\n", status);
	return status;
}


/* usb_serial_free - called after last device is closed, and all i/o complete. */
static status_t
usb_serial_free(void *cookie)
{
	TRACE_FUNCALLS("> usb_serial_free(0x%08x)\n", cookie);
	SerialDevice *device = (SerialDevice *)cookie;
	acquire_sem(gDriverLock);
	status_t status = device->Free();
	if (device->IsRemoved()) {
		for (int32 i = 0; i < DEVICES_COUNT; i++) {
			if (gSerialDevices[i] == device) {
				// the device is removed already but as it was open the
				// removed hook has not deleted the object
				delete device;
				gSerialDevices[i] = NULL;
				break;
			}
		}
	}

	release_sem(gDriverLock);
	TRACE_FUNCRET("< usb_serial_free() returns: 0x%08x\n", status);
	return status;
}


/* publish_devices - null-terminated array of devices supported by this driver. */
const char **
publish_devices()
{
	TRACE_FUNCALLS("> publish_devices()\n");
	for (int32 i = 0; gDeviceNames[i]; i++)
		free(gDeviceNames[i]);

	int32 j = 0;
	acquire_sem(gDriverLock);
	for(int32 i = 0; i < DEVICES_COUNT; i++) {
		if (gSerialDevices[i]) {
			gDeviceNames[j] = (char *)malloc(strlen(sDeviceBaseName) + 4);
			if (gDeviceNames[j]) {
				sprintf(gDeviceNames[j], "%s%ld", sDeviceBaseName, i);
				j++;
			} else
				TRACE_ALWAYS("publish_devices - no memory to allocate device names\n");
		}
	}

	gDeviceNames[j] = NULL;
	release_sem(gDriverLock);
	return (const char **)&gDeviceNames[0];
}


/* find_device - return poiter to device hooks structure for a given device */
device_hooks *
find_device(const char *name)
{
	static device_hooks deviceHooks = {
		usb_serial_open,			/* -> open entry point */
		usb_serial_close,			/* -> close entry point */
		usb_serial_free,			/* -> free cookie */
		usb_serial_control,			/* -> control entry point */
		usb_serial_read,			/* -> read entry point */
		usb_serial_write,			/* -> write entry point */
		usb_serial_select,			/* -> select entry point */
		usb_serial_deselect			/* -> deselect entry point */
	};

	TRACE_FUNCALLS("> find_device(%s)\n", name);
	return &deviceHooks;
}

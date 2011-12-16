/*
 * Copyright 2008-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include <ByteOrder.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "usb_printer.h"
#include "USB_printer.h"

#define DRIVER_NAME			"usb_printer"
#define DEVICE_NAME_BASE	"printer/usb/"
#define DEVICE_NAME			DEVICE_NAME_BASE"%ld"

#define SECONDS             ((bigtime_t)1000 * 1000)
#define MINUTES             (60 * SECONDS)
#define HOURS               (60 * MINUTES)

// large timeout so user can fix any issues (for instance insert paper)
// before printing can start or continue
#define WRITE_TIMEOUT       (1 * HOURS)
#define READ_TIMEOUT        (2 * SECONDS)

// #define TRACE_USB_PRINTER 1
#ifdef TRACE_USB_PRINTER
#define TRACE(x...)			dprintf(DRIVER_NAME": "x)
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME": "x)
#else
#define TRACE(x...)			/* nothing */
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME": "x)
#endif


int32 api_version = B_CUR_DRIVER_API_VERSION;
static usb_module_info *gUSBModule = NULL;
static printer_device *gDeviceList = NULL;
static uint32 gDeviceCount = 0;
static mutex gDeviceListLock;
static char **gDeviceNames = NULL;



//
//#pragma mark - Forward Declarations
//


static void	usb_printer_callback(void *cookie, status_t status, void *data,
				size_t actualLength);

status_t	usb_printer_transfer_data(printer_device *device, bool directionIn,
				void *data, size_t dataLength);


//
//#pragma mark - Device Allocation Helper Functions
//


void
usb_printer_free_device(printer_device *device)
{
	mutex_lock(&device->lock);
	mutex_destroy(&device->lock);
	delete_sem(device->notify);
	free(device);
}


//
//#pragma mark - Bulk-only Functions
//


void
usb_printer_reset_recovery(printer_device *device)
{
	gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
	gUSBModule->clear_feature(device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
}


status_t
usb_printer_transfer_data(printer_device *device, bool directionIn, void *data,
	size_t dataLength)
{
	status_t result = gUSBModule->queue_bulk(directionIn ? device->bulk_in
		: device->bulk_out, data, dataLength, usb_printer_callback, device);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to queue data transfer\n");
		return result;
	}

	do {
		bigtime_t timeout = directionIn ? READ_TIMEOUT : WRITE_TIMEOUT;
		result = acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT,
			timeout);
		if (result == B_TIMED_OUT) {
			// Cancel the transfer and collect the sem that should now be
			// released through the callback on cancel. Handling of device
			// reset is done in usb_printer_operation() when it detects that
			// the transfer failed.
			gUSBModule->cancel_queued_transfers(directionIn ? device->bulk_in
				: device->bulk_out);
			acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT, 0);
		}
	} while (result == B_INTERRUPTED);

	if (result != B_OK) {
		TRACE_ALWAYS("acquire_sem failed while waiting for data transfer\n");
		return result;
	}

	return B_OK;
}


//
//#pragma mark - Device Attach/Detach Notifications and Callback
//


static void
usb_printer_callback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	printer_device *device = (printer_device *)cookie;
	device->status = status;
	device->actual_length = actualLength;
	release_sem(device->notify);
}


static status_t
usb_printer_device_added(usb_device newDevice, void **cookie)
{
	TRACE("device_added(0x%08lx)\n", newDevice);
	printer_device *device = (printer_device *)malloc(sizeof(printer_device));
	device->device = newDevice;
	device->removed = false;
	device->open_count = 0;
	device->interface = 0xff;
	device->alternate_setting = 0;

	// scan through the interfaces to find our bulk-only data interface
	const usb_configuration_info *configuration =
		gUSBModule->get_configuration(newDevice);
	if (configuration == NULL) {
		free(device);
		return B_ERROR;
	}

	for (size_t i = 0; i < configuration->interface_count; i++) {
		for (size_t j = 0; j < configuration->interface[i].alt_count; j++) {
			usb_interface_info *interface = &configuration->interface[i].alt[j];
			if (interface == NULL
				|| interface->descr->interface_class != PRINTER_INTERFACE_CLASS
				|| interface->descr->interface_subclass !=
					PRINTER_INTERFACE_SUBCLASS
				|| !(interface->descr->interface_protocol == PIT_UNIDIRECTIONAL
					|| interface->descr->interface_protocol == PIT_BIDIRECTIONAL
					|| interface->descr->interface_protocol
						== PIT_1284_4_COMPATIBLE)) {
				continue;
			}

			bool hasIn = false;
			bool hasOut = false;
			for (size_t k = 0; k < interface->endpoint_count; k++) {
				usb_endpoint_info *endpoint = &interface->endpoint[k];
				if (endpoint == NULL
					|| endpoint->descr->attributes != USB_ENDPOINT_ATTR_BULK) {
					continue;
				}

				if (!hasIn && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN)) {
					device->bulk_in = endpoint->handle;
					hasIn = true;
				} else if (!hasOut && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN) == 0) {
					device->bulk_out = endpoint->handle;
					hasOut = true;
				}

				if (hasIn && hasOut)
					break;
			}

			if (!(hasIn && hasOut))
				continue;

			device->interface = interface->descr->interface_number;
			device->alternate = j;
			device->alternate_setting = interface->descr->alternate_setting;

			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid interface found\n");
		free(device);
		return B_ERROR;
	}

	status_t status = gUSBModule->set_alt_interface(newDevice,
		&configuration->interface[device->interface].alt[device->alternate]);
	if (status < B_OK) {
		free(device);
		return B_ERROR;
	}

	mutex_init(&device->lock, "usb_printer device lock");

	device->notify = create_sem(0, "usb_printer callback notify");
	if (device->notify < B_OK) {
		mutex_destroy(&device->lock);
		status_t result = device->notify;
		free(device);
		return result;
	}

	mutex_lock(&gDeviceListLock);
	uint32 freeDeviceMask = 0;
	device->device_number = 0;
	printer_device *other = gDeviceList;
	while (other != NULL) {
		// set max device number + 1
		if (other->device_number >= device->device_number)
			device->device_number = other->device_number + 1;

		// record used devices
		if (other->device_number < 32) {
			freeDeviceMask |= 1 << other->device_number;
		}

		other = other->link;
	}
	if (freeDeviceMask != (uint32)-1) {
		// reuse device number of first 32 devices
		for (int32 deviceNumber = 0; deviceNumber < 32; deviceNumber++) {
			if ((freeDeviceMask & (1 << deviceNumber)) == 0) {
				device->device_number = deviceNumber;
				break;
			}
		}
	}

	device->link = gDeviceList;
	gDeviceList = device;
	uint32 deviceNumber = gDeviceCount++;
	sprintf(device->name, DEVICE_NAME, deviceNumber);
	mutex_unlock(&gDeviceListLock);

	TRACE("new device: 0x%08lx\n", (uint32)device);
	*cookie = (void *)device;
	return B_OK;
}


static status_t
usb_printer_device_removed(void *cookie)
{
	TRACE("device_removed(0x%08lx)\n", (uint32)cookie);
	printer_device *device = (printer_device *)cookie;

	mutex_lock(&gDeviceListLock);
	if (gDeviceList == device) {
		gDeviceList = device->link;
	} else {
		printer_device *element = gDeviceList;
		while (element) {
			if (element->link == device) {
				element->link = device->link;
				break;
			}

			element = element->link;
		}
	}
	gDeviceCount--;

	device->removed = true;
	gUSBModule->cancel_queued_transfers(device->bulk_in);
	gUSBModule->cancel_queued_transfers(device->bulk_out);
	if (device->open_count == 0)
		usb_printer_free_device(device);

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


//
//#pragma mark - Driver Hooks
//


static status_t
usb_printer_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s)\n", name);
	if (strncmp(name, DEVICE_NAME_BASE, strlen(DEVICE_NAME_BASE)) != 0)
		return B_NAME_NOT_FOUND;

	mutex_lock(&gDeviceListLock);
	printer_device *device = gDeviceList;
	while (device) {
		if (strncmp(name, device->name, 32) == 0) {
			if (device->removed) {
				mutex_unlock(&gDeviceListLock);
				return B_ERROR;
			}

			device->open_count++;
			*cookie = device;
			mutex_unlock(&gDeviceListLock);
			return B_OK;
		}

		device = device->link;
	}

	mutex_unlock(&gDeviceListLock);
	return B_NAME_NOT_FOUND;
}


static status_t
usb_printer_close(void *cookie)
{
	TRACE("close()\n");
	return B_OK;
}


static status_t
usb_printer_free(void *cookie)
{
	TRACE("free()\n");
	mutex_lock(&gDeviceListLock);

	printer_device *device = (printer_device *)cookie;
	device->open_count--;
	if (device->open_count == 0 && device->removed) {
		// we can simply free the device here as it has been removed from
		// the device list in the device removed notification hook
		usb_printer_free_device(device);
	}

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


static status_t
usb_printer_get_device_id(printer_device *device, void *buffer)
{
	uint16 value = 0;
	uint16 index = (device->interface << 8) | device->alternate_setting;
	char device_id[USB_PRINTER_DEVICE_ID_LENGTH + 2];
	size_t deviceIdSize = 0;
	status_t status = gUSBModule->send_request(device->device, PRINTER_REQUEST,
		REQUEST_GET_DEVICE_ID, value, index,
		sizeof(device_id), device_id, &deviceIdSize);

	if (status == B_OK && deviceIdSize > 2) {
		// terminate string
		device_id[deviceIdSize - 1] = '\0';
		// skip first two bytes containing string length again
		status = user_strlcpy((char *)buffer, &device_id[2],
			USB_PRINTER_DEVICE_ID_LENGTH);
	} else {
		dprintf("%s: Failed to get device ID for interface %d "
			"and alternate setting %d: %s\n",
			DRIVER_NAME,
			(int)device->interface,
			(int)device->alternate_setting,
			strerror(status));
		status = user_strlcpy((char *)buffer,
			"MFG:Unknown;CMD:Unknown;MDL:Unknown;",
			USB_PRINTER_DEVICE_ID_LENGTH);
	}
	return status;
}


static status_t
usb_printer_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	printer_device *device = (printer_device *)cookie;
	mutex_lock(&device->lock);
	if (device->removed) {
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = B_DEV_INVALID_IOCTL;
	switch (op) {
		case USB_PRINTER_GET_DEVICE_ID: {
			result = usb_printer_get_device_id(device, buffer);
			break;
		}

		default:
			TRACE_ALWAYS("unhandled ioctl %ld\n", op);
			break;
	}

	mutex_unlock(&device->lock);
	return result;
}


static status_t
usb_printer_transfer(printer_device* device, bool directionIn, void* buffer,
	size_t* length)
{
	status_t result = B_ERROR;
	if (buffer == NULL || length == NULL || *length <= 0) {
		return result;
	}

	result = usb_printer_transfer_data(device, directionIn, buffer, *length);
	if (result != B_OK) {
		return result;
	}
	size_t transferedData = device->actual_length;
	if (device->status != B_OK || transferedData != *length) {
		// sending or receiving of the data failed
		if (device->status == B_DEV_STALLED) {
			TRACE("stall while transfering data\n");
			gUSBModule->clear_feature(directionIn ? device->bulk_in
				: device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
		} else {
			TRACE_ALWAYS("sending or receiving of the data failed\n");
			usb_printer_reset_recovery(device);
			return B_ERROR;
		}
	}
	*length = transferedData;

	return result;
}


static status_t
usb_printer_read(void *cookie, off_t position, void *buffer, size_t *length)
{
	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	TRACE("read(%lld, %ld)\n", position, *length);
	printer_device *device = (printer_device *)cookie;
	mutex_lock(&device->lock);
	if (device->removed) {
		*length = 0;
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = usb_printer_transfer(device, true, buffer, length);

	mutex_unlock(&device->lock);
	if (result == B_OK) {
		TRACE("read successful with %ld bytes\n", *length);
		return B_OK;
	}

	*length = 0;
	TRACE_ALWAYS("read fails with 0x%08lx\n", result);
	return result;
}


static status_t
usb_printer_write(void *cookie, off_t position, const void *buffer,
	size_t *length)
{
	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	TRACE("write(%lld, %ld)\n", position, *length);
	printer_device *device = (printer_device *)cookie;
	mutex_lock(&device->lock);
	if (device->removed) {
		*length = 0;
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = usb_printer_transfer(device, false, (void*)buffer,
		length);

	mutex_unlock(&device->lock);
	if (result == B_OK) {
		TRACE("write successful with %ld bytes\n", *length);
		return B_OK;
	}

	*length = 0;
	TRACE_ALWAYS("write fails with 0x%08lx\n", result);
	return result;
}


//
//#pragma mark - Driver Entry Points
//


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
	static usb_notify_hooks notifyHooks = {
		&usb_printer_device_added,
		&usb_printer_device_removed
	};

	static usb_support_descriptor supportedDevices = {
		PRINTER_INTERFACE_CLASS,
		PRINTER_INTERFACE_SUBCLASS,
		0, // any protocol
		0, // any vendor
		0  // any product
	};

	gDeviceList = NULL;
	gDeviceCount = 0;
	mutex_init(&gDeviceListLock, "usb_printer device list lock");

	TRACE("trying module %s\n", B_USB_MODULE_NAME);
	status_t result = get_module(B_USB_MODULE_NAME,
		(module_info **)&gUSBModule);
	if (result < B_OK) {
		TRACE_ALWAYS("getting module failed 0x%08lx\n", result);
		mutex_destroy(&gDeviceListLock);
		return result;
	}

	gUSBModule->register_driver(DRIVER_NAME, &supportedDevices, 1, NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	return B_OK;
}


void
uninit_driver()
{
	TRACE("uninit_driver()\n");
	gUSBModule->uninstall_notify(DRIVER_NAME);
	mutex_lock(&gDeviceListLock);

	if (gDeviceNames) {
		for (int32 i = 0; gDeviceNames[i]; i++)
			free(gDeviceNames[i]);
		free(gDeviceNames);
		gDeviceNames = NULL;
	}

	mutex_destroy(&gDeviceListLock);
	put_module(B_USB_MODULE_NAME);
}


const char **
publish_devices()
{
	TRACE("publish_devices()\n");
	if (gDeviceNames) {
		for (int32 i = 0; gDeviceNames[i]; i++)
			free(gDeviceNames[i]);
		free(gDeviceNames);
		gDeviceNames = NULL;
	}

	gDeviceNames = (char **)malloc(sizeof(char *) * (gDeviceCount + 1));
	if (gDeviceNames == NULL)
		return NULL;

	int32 index = 0;
	mutex_lock(&gDeviceListLock);
	printer_device *device = gDeviceList;
	while (device) {
		gDeviceNames[index++] = strdup(device->name);

		device = device->link;
	}

	gDeviceNames[index++] = NULL;
	mutex_unlock(&gDeviceListLock);
	return (const char **)gDeviceNames;
}


device_hooks *
find_device(const char *name)
{
	TRACE("find_device()\n");
	static device_hooks hooks = {
		&usb_printer_open,
		&usb_printer_close,
		&usb_printer_free,
		&usb_printer_ioctl,
		&usb_printer_read,
		&usb_printer_write,
		NULL,
		NULL,
		NULL,
		NULL
	};

	return &hooks;
}

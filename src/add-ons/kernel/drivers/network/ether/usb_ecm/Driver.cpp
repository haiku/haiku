/*
	Driver for USB Ethernet Control Model devices
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lock.h>

#include <bus/USB.h>


#include "Driver.h"
#include "ECMDevice.h"


#define DEVICE_BASE_NAME "net/usb_ecm/"
usb_module_info *gUSBModule = NULL;
device_manager_info *gDeviceManager;


#define USB_ECM_DRIVER_MODULE_NAME "drivers/network/usb_ecm/driver_v1"
#define USB_ECM_DEVICE_MODULE_NAME "drivers/network/usb_ecm/device_v1"
#define USB_ECM_DEVICE_ID_GENERATOR	"usb_ecm/device_id"


//	#pragma mark - device module API


static status_t
usb_ecm_init_device(void* _info, void** _cookie)
{
	CALLED();
	usb_ecm_driver_info* info = (usb_ecm_driver_info*)_info;

	device_node* parent = gDeviceManager->get_parent_node(info->node);
	gDeviceManager->get_driver(parent, (driver_module_info **)&info->usb,
		(void **)&info->usb_device);
	gDeviceManager->put_node(parent);

	usb_device device;
	if (gDeviceManager->get_attr_uint32(info->node, USB_DEVICE_ID_ITEM, &device, true) != B_OK)
		return B_ERROR;

	ECMDevice *ecmDevice = new ECMDevice(device);
	status_t status = ecmDevice->InitCheck();
	if (status < B_OK) {
		delete ecmDevice;
		return status;
	}

	info->device = ecmDevice;

	*_cookie = info;
	return status;
}


static void
usb_ecm_uninit_device(void* _cookie)
{
	CALLED();
	usb_ecm_driver_info* info = (usb_ecm_driver_info*)_cookie;

	delete info->device;
}


static void
usb_ecm_device_removed(void* _cookie)
{
	CALLED();
	usb_ecm_driver_info* info = (usb_ecm_driver_info*)_cookie;
	info->device->Removed();
}


static status_t
usb_ecm_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	usb_ecm_driver_info* info = (usb_ecm_driver_info*)_info;

	status_t status = info->device->Open();
	if (status != B_OK)
		return status;

	*_cookie = info->device;
	return B_OK;
}


static status_t
usb_ecm_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE("read(%p, %" B_PRIdOFF", %p, %lu)\n", cookie, position, buffer, *numBytes);
	ECMDevice *device = (ECMDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_ecm_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE("write(%p, %" B_PRIdOFF", %p, %lu)\n", cookie, position, buffer, *numBytes);
	ECMDevice *device = (ECMDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_ecm_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE("control(%p, %" B_PRIu32 ", %p, %lu)\n", cookie, op, buffer, length);
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
	return device->Free();
}


//	#pragma mark - driver module API


static float
usb_ecm_supports_device(device_node *parent)
{
	CALLED();
	const char *bus;

	// make sure parent is really the usb bus manager
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "usb"))
		return 0.0;


	// check whether it's really an ECM device
	device_attr *attr = NULL;
	uint8 baseClass = 0, subclass = 0;
	while (gDeviceManager->get_next_attr(parent, &attr) == B_OK) {
		if (attr->type != B_UINT8_TYPE)
			continue;

		if (!strcmp(attr->name, USB_DEVICE_CLASS))
			baseClass = attr->value.ui8;
		if (!strcmp(attr->name, USB_DEVICE_SUBCLASS))
			subclass = attr->value.ui8;
		if (baseClass != 0 && subclass != 0) {
			if (baseClass == USB_INTERFACE_CLASS_CDC && subclass == USB_INTERFACE_SUBCLASS_ECM)
				break;
			baseClass = subclass = 0;
		}
	}

	if (baseClass != USB_INTERFACE_CLASS_CDC || subclass != USB_INTERFACE_SUBCLASS_ECM)
		return 0.0;

	TRACE("USB-ECM device found!\n");

	return 0.6;
}


static status_t
usb_ecm_register_device(device_node *node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "USB ECM"} },
		{ NULL }
	};

	return gDeviceManager->register_node(node, USB_ECM_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
usb_ecm_init_driver(device_node *node, void **cookie)
{
	CALLED();

	usb_ecm_driver_info* info = (usb_ecm_driver_info*)malloc(
		sizeof(usb_ecm_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));
	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
usb_ecm_uninit_driver(void *_cookie)
{
	CALLED();
	usb_ecm_driver_info* info = (usb_ecm_driver_info*)_cookie;
	free(info);
}


static status_t
usb_ecm_register_child_devices(void* _cookie)
{
	CALLED();
	usb_ecm_driver_info* info = (usb_ecm_driver_info*)_cookie;
	status_t status;

	int32 id = gDeviceManager->create_id(USB_ECM_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), DEVICE_BASE_NAME "%" B_PRId32,
		id);

	status = gDeviceManager->publish_device(info->node, name,
		USB_ECM_DEVICE_MODULE_NAME);

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ B_USB_MODULE_NAME, (module_info**)&gUSBModule},
	{ NULL }
};

struct device_module_info sUsbEcmDevice = {
	{
		USB_ECM_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	usb_ecm_init_device,
	usb_ecm_uninit_device,
	usb_ecm_device_removed,

	usb_ecm_open,
	usb_ecm_close,
	usb_ecm_free,
	usb_ecm_read,
	usb_ecm_write,
	NULL,	// io
	usb_ecm_control,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sUsbEcmDriver = {
	{
		USB_ECM_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	usb_ecm_supports_device,
	usb_ecm_register_device,
	usb_ecm_init_driver,
	usb_ecm_uninit_driver,
	usb_ecm_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sUsbEcmDriver,
	(module_info*)&sUsbEcmDevice,
	NULL
};

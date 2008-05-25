/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.

	Some parts of the code are based on the previous usb_hid driver which
	was written by  Jérôme Duval.
 */
#include "DeviceList.h"
#include "Driver.h"
#include "HIDDevice.h"

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <lock.h> // for mutex
#else
#include "BeOSCompatibility.h" // for pseudo mutex
#endif


int32 api_version = B_CUR_DRIVER_API_VERSION;
usb_module_info *gUSBModule = NULL;
DeviceList *gDeviceList = NULL;
static int32 sParentCookie = 0;
static mutex sDriverLock;


// #pragma mark - notify hooks


static status_t
usb_hid_device_added(usb_device device, void **cookie)
{
	TRACE("device_added()\n");
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	TRACE("vendor id: 0x%04x; product id: 0x%04x\n",
		deviceDescriptor->vendor_id, deviceDescriptor->product_id);

	// wacom devices are handled by the dedicated wacom driver
	if (deviceDescriptor->vendor_id == USB_VENDOR_WACOM)
		return B_ERROR;

	const usb_configuration_info *config
		= gUSBModule->get_nth_configuration(device, USB_DEFAULT_CONFIGURATION);
	if (config == NULL) {
		TRACE_ALWAYS("cannot get default configuration\n");
		return B_ERROR;
	}

	// ensure default configuration is set
	status_t result = gUSBModule->set_configuration(device, config);
	if (result != B_OK) {
		TRACE_ALWAYS("set_configuration() failed 0x%08lx\n", result);
		return result;
	}

	// refresh config
	config = gUSBModule->get_configuration(device);
	if (config == NULL) {
		TRACE_ALWAYS("cannot get current configuration\n");
		return B_ERROR;
	}

	bool devicesFound = false;
	int32 parentCookie = atomic_add(&sParentCookie, 1);
	for (size_t i = 0; i < config->interface_count; i++) {
		const usb_interface_info *interface = config->interface[i].active;
		uint8 interfaceClass = interface->descr->interface_class;
		uint8 interfaceSubclass = interface->descr->interface_subclass;
		uint8 interfaceProtocol = interface->descr->interface_protocol;
		TRACE("interface %lu: class: %u; subclass: %u; protocol: %u\n",
			i, interfaceClass, interfaceSubclass, interfaceProtocol);

		if (interfaceClass == USB_INTERFACE_CLASS_HID
			&& interfaceSubclass == USB_INTERFACE_SUBCLASS_HID_BOOT) {
			mutex_lock(&sDriverLock);
			HIDDevice *hidDevice = HIDDevice::MakeHIDDevice(device, config, i);

			if (hidDevice != NULL && hidDevice->InitCheck() == B_OK) {
				hidDevice->SetParentCookie(parentCookie);
				gDeviceList->AddDevice(hidDevice->Name(), hidDevice);
				devicesFound = true;
			} else
				delete hidDevice;
			mutex_unlock(&sDriverLock);
		}
	}

	if (!devicesFound)
		return B_ERROR;

	*cookie = (void *)parentCookie;
	return B_OK;
}


static status_t
usb_hid_device_removed(void *cookie)
{
	mutex_lock(&sDriverLock);
	int32 parentCookie = (int32)cookie;
	TRACE("device_removed(%ld)\n", parentCookie);

	for (int32 i = 0; i < gDeviceList->CountDevices(); i++) {
		HIDDevice *device = (HIDDevice *)gDeviceList->DeviceAt(i);
		if (!device)
			continue;

		if (device->ParentCookie() == parentCookie) {
			// this device belongs to the one removed
			if (device->IsOpen()) {
				// the device will be deleted upon being freed
				device->Removed();
			} else {
				// remove the device and start over
				gDeviceList->RemoveDevice(NULL, device);
				i = -1;
			}
		}
	}

	mutex_unlock(&sDriverLock);
	return B_OK;
}


// #pragma mark - driver hooks


static status_t
usb_hid_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s, %lu, %p)\n", name, flags, cookie);
	mutex_lock(&sDriverLock);

	HIDDevice *device = (HIDDevice *)gDeviceList->FindDevice(name);
	if (device == NULL) {
		mutex_unlock(&sDriverLock);
		return B_ENTRY_NOT_FOUND;
	}

	status_t result = device->Open(flags);
	*cookie = device;
	mutex_unlock(&sDriverLock);
	return result;
}


static status_t
usb_hid_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE("read(%p, %Ld, %p, %lu)\n", cookie, position, buffer, *numBytes);
	HIDDevice *device = (HIDDevice *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
usb_hid_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE("write(%p, %Ld, %p, %lu)\n", cookie, position, buffer, *numBytes);
	HIDDevice *device = (HIDDevice *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
usb_hid_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE("control(%p, %lu, %p, %lu)\n", cookie, op, buffer, length);
	HIDDevice *device = (HIDDevice *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
usb_hid_close(void *cookie)
{
	TRACE("close(%p)\n", cookie);
	HIDDevice *device = (HIDDevice *)cookie;
	return device->Close();
}


static status_t
usb_hid_free(void *cookie)
{
	TRACE("free(%p)\n", cookie);
	HIDDevice *device = (HIDDevice *)cookie;
	mutex_lock(&sDriverLock);
	status_t status = device->Free();
	if (gDeviceList->RemoveDevice(NULL, device) == B_OK) {
		// the device is removed already but as it was open the removed hook
		// has not deleted the object
		delete device;
	}

	mutex_unlock(&sDriverLock);
	return status;
}


//	#pragma mark - driver API


status_t
init_hardware()
{
	TRACE("init_hardware() " __DATE__ " " __TIME__ "\n");
	return B_OK;
}


status_t
init_driver()
{
	TRACE("init_driver() " __DATE__ " " __TIME__ "\n");
	if (get_module(B_USB_MODULE_NAME, (module_info **)&gUSBModule) != B_OK)
		return B_ERROR;

	gDeviceList = new DeviceList();
	if (gDeviceList == NULL) {
		put_module(B_USB_MODULE_NAME);
		return B_NO_MEMORY;
	}

	static usb_notify_hooks notifyHooks = {
		&usb_hid_device_added,
		&usb_hid_device_removed
	};

	static usb_support_descriptor supportDescriptor = {
		USB_INTERFACE_CLASS_HID, 0, 0, 0, 0
	};

	gUSBModule->register_driver(DRIVER_NAME, &supportDescriptor, 1, NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	TRACE("init_driver() OK\n");
	return B_OK;
}


void
uninit_driver()
{
	TRACE("uninit_driver()\n");
	gUSBModule->uninstall_notify(DRIVER_NAME);
	put_module(B_USB_MODULE_NAME);
	delete gDeviceList;
	gDeviceList = NULL;
}


const char **
publish_devices()
{
	TRACE("publish_devices()\n");
	const char **publishList = gDeviceList->PublishDevices();

	int32 index = 0;
	while (publishList[index] != NULL) {
		TRACE("publishing %s\n", publishList[index]);
		index++;
	}

	return publishList;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		usb_hid_open,
		usb_hid_close,
		usb_hid_free,
		usb_hid_control,
		usb_hid_read,
		usb_hid_write,
		NULL,				/* select */
		NULL				/* deselect */
	};

	TRACE("find_device(%s)\n", name);
	if (gDeviceList->FindDevice(name) == NULL) {
		TRACE_ALWAYS("didn't find device %s\n", name);
		return NULL;
	}

	return &hooks;
}

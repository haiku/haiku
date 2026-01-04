/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Copyright 2020, 2022 Vladimir Kondratyev <wulf@FreeBSD.org>
 * Copyright 2023 Vladimir Serbinenko <phcoder@gmail.com>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for I2C Elan Devices.
// Partially based on FreeBSD ietp driver


#include <ACPI.h>
#include <device_manager.h>
#include <i2c.h>

#include "DeviceList.h"
#include "Driver.h"
#include "ELANDevice.h"

#include <lock.h>
#include <util/AutoLock.h>

#include <new>
#include <stdio.h>
#include <string.h>



struct elan_driver_cookie {
	ELANDevice*				elanDevice;
};

#define I2C_ELAN_DRIVER_NAME "drivers/input/i2c_elan/driver_v1"
#define I2C_ELAN_DEVICE_NAME "drivers/input/i2c_elan/device_v1"

/* Base Namespace devices are published to */
#define I2C_ELAN_BASENAME "input/i2c_elan/%d"

static const char* elan_iic_devs[] = {
	"ELAN0000",
	"ELAN0100",
	"ELAN0600",
	"ELAN0601",
	"ELAN0602",
	"ELAN0603",
	"ELAN0604",
	"ELAN0605",
	"ELAN0606",
	"ELAN0607",
	"ELAN0608",
	"ELAN0609",
	"ELAN060B",
	"ELAN060C",
	"ELAN060F",
	"ELAN0610",
	"ELAN0611",
	"ELAN0612",
	"ELAN0615",
	"ELAN0616",
	"ELAN0617",
	"ELAN0618",
	"ELAN0619",
	"ELAN061A",
	"ELAN061B",
	"ELAN061C",
	"ELAN061D",
	"ELAN061E",
	"ELAN061F",
	"ELAN0620",
	"ELAN0621",
	"ELAN0622",
	"ELAN0623",
	"ELAN0624",
	"ELAN0625",
	"ELAN0626",
	"ELAN0627",
	"ELAN0628",
	"ELAN0629",
	"ELAN062A",
	"ELAN062B",
	"ELAN062C",
	"ELAN062D",
	"ELAN062E",	/* Lenovo V340 Whiskey Lake U */
	"ELAN062F",	/* Lenovo V340 Comet Lake U */
	"ELAN0631",
	"ELAN0632",
	"ELAN0633",	/* Lenovo S145 */
	"ELAN0634",	/* Lenovo V340 Ice lake */
	"ELAN0635",	/* Lenovo V1415-IIL */
	"ELAN0636",	/* Lenovo V1415-Dali */
	"ELAN0637",	/* Lenovo V1415-IGLR */
	"ELAN1000"
};

static device_manager_info *sDeviceManager;

DeviceList *gDeviceList = NULL;
static mutex sDriverLock;


// #pragma mark - driver hooks


static status_t
i2c_elan_init_device(void *driverCookie, void **cookie)
{
	*cookie = driverCookie;
	return B_OK;
}


static void
i2c_elan_uninit_device(void *_cookie)
{

}


static status_t
i2c_elan_open(void *initCookie, const char *path, int flags, void **_cookie)
{
	TRACE("open(%s, %" B_PRIu32 ", %p)\n", path, flags, _cookie);

	elan_driver_cookie *cookie = new(std::nothrow) elan_driver_cookie();
	if (cookie == NULL)
		return B_NO_MEMORY;

	MutexLocker locker(sDriverLock);

	ELANDevice *elan = (ELANDevice *)gDeviceList->FindDevice(path);
	TRACE("  path %s: handler %p (elan)\n", path, elan);

	cookie->elanDevice = elan;

	status_t result = elan == NULL ? B_ENTRY_NOT_FOUND : B_OK;
	if (result == B_OK)
		result = elan->Open(flags);

	if (result != B_OK) {
		delete cookie;
		return result;
	}

	*_cookie = cookie;

	return B_OK;
}


static status_t
i2c_elan_read(void *_cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE_ALWAYS("unhandled read on i2c_elan\n");
	*numBytes = 0;
	return B_ERROR;
}


static status_t
i2c_elan_write(void *_cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE_ALWAYS("unhandled write on i2c_elan\n");
	*numBytes = 0;
	return B_ERROR;
}


static status_t
i2c_elan_control(void *_cookie, uint32 op, void *buffer, size_t length)
{
	elan_driver_cookie *cookie = (elan_driver_cookie *)_cookie;

	TRACE("control(%p, %" B_PRIu32 ", %p, %" B_PRIuSIZE ")\n", cookie, op, buffer, length);
	return cookie->elanDevice->Control(op, buffer, length);
}


static status_t
i2c_elan_close(void *_cookie)
{
	elan_driver_cookie *cookie = (elan_driver_cookie *)_cookie;

	TRACE("close(%p)\n", cookie);
	return cookie->elanDevice->Close();
}


static status_t
i2c_elan_free(void *_cookie)
{
	elan_driver_cookie *cookie = (elan_driver_cookie *)_cookie;
	TRACE("free(%p)\n", cookie);

	mutex_lock(&sDriverLock);

	ELANDevice *device = cookie->elanDevice;
	if (device->IsOpen()) {
		// another handler of this device is still open so we can't free it
	} else if (device->IsRemoved()) {
		// the parent device is removed already and none of its handlers are
		// open anymore so we can free it here
		delete device;
	}

	mutex_unlock(&sDriverLock);

	delete cookie;
	return B_OK;
}


//	#pragma mark - driver module API

static bool is_elan_name(const char *name) {
	if (name == NULL)
		return false;
	for (unsigned i = 0; i < B_COUNT_OF(elan_iic_devs); i++)
		if (strcmp(name, elan_iic_devs[i]) == 0)
			return true;
	return false;
};

static float
i2c_elan_support(device_node *parent)
{
	CALLED();

	// make sure parent is really the I2C bus manager
	const char *bus;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "i2c"))
		return 0.0;
	TRACE("i2c_elan_support found an i2c device %p\n", parent);

	// check whether it's an ELAN device
	uint64 handlePointer;
	if (sDeviceManager->get_attr_uint64(parent, ACPI_DEVICE_HANDLE_ITEM,
			&handlePointer, false) != B_OK) {
		TRACE("i2c_elan_support found an i2c device without acpi handle\n");
		return B_ERROR;
	}

	const char *name = nullptr;
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &name,
					    false) == B_OK && is_elan_name(name)) {
		TRACE("i2c_elan_support found an elan i2c device\n");
		return 0.6;
	}

	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_CID_ITEM, &name,
		false) == B_OK && is_elan_name(name)) {
		TRACE("i2c_elan_support found a compatible elan i2c device\n");
		return 0.6;
	}

	return 0.0;
}

static status_t
i2c_elan_register_device(device_node *node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "I2C ELAN Device" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, I2C_ELAN_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
i2c_elan_init_driver(device_node *node, void **driverCookie)
{
	CALLED();

	elan_driver_cookie *device
		= (elan_driver_cookie *)calloc(1, sizeof(elan_driver_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	*driverCookie = device;

	device_node *parent;
	i2c_device_interface*	i2c;
	i2c_device				i2c_cookie;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&i2c,
		(void **)&i2c_cookie);
	sDeviceManager->put_node(parent);

	mutex_lock(&sDriverLock);
	ELANDevice *elanDevice = new(std::nothrow) ELANDevice(node, i2c, i2c_cookie);

	if (elanDevice != NULL && elanDevice->InitCheck() == B_OK) {
		device->elanDevice = elanDevice;
	} else
		delete elanDevice;

	mutex_unlock(&sDriverLock);

	return device->elanDevice != NULL ? B_OK : B_IO_ERROR;
}


static void
i2c_elan_uninit_driver(void *driverCookie)
{
	CALLED();
	elan_driver_cookie *device = (elan_driver_cookie*)driverCookie;

	free(device);
}


static status_t
i2c_elan_register_child_devices(void *cookie)
{
	CALLED();
	elan_driver_cookie *device = (elan_driver_cookie*)cookie;
	ELANDevice* elanDevice = device->elanDevice;
	if (elanDevice == NULL)
		return B_OK;

	// As devices can be un- and replugged at will, we cannot
	// simply rely on a device count. If there is just one
	// keyboard, this does not mean that it uses the 0 name.
	// There might have been two keyboards and the one using 0
	// might have been unplugged. So we just generate names
	// until we find one that is not currently in use.
	int32 index = 0;
	char pathBuffer[B_DEV_NAME_LENGTH];
	while (true) {
		sprintf(pathBuffer, "input/mouse/" DEVICE_PATH_SUFFIX "/%" B_PRId32, index++);
		if (gDeviceList->FindDevice(pathBuffer) == NULL) {
			// this name is still free, use it
			elanDevice->SetPublishPath(strdup(pathBuffer));
			break;
		}
	}

	gDeviceList->AddDevice(elanDevice->PublishPath(), elanDevice);

	sDeviceManager->publish_device(device->elanDevice->Parent(), pathBuffer,
		I2C_ELAN_DEVICE_NAME);

	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			gDeviceList = new(std::nothrow) DeviceList();
			if (gDeviceList == NULL) {
				return B_NO_MEMORY;
			}
			mutex_init(&sDriverLock, "i2c elan driver lock");

			return B_OK;
		case B_MODULE_UNINIT:
			delete gDeviceList;
			gDeviceList = NULL;
			mutex_destroy(&sDriverLock);
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


//	#pragma mark -


driver_module_info i2c_elan_driver_module = {
	{
		I2C_ELAN_DRIVER_NAME,
		0,
		&std_ops
	},

	i2c_elan_support,
	i2c_elan_register_device,
	i2c_elan_init_driver,
	i2c_elan_uninit_driver,
	i2c_elan_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info i2c_elan_device_module = {
	{
		I2C_ELAN_DEVICE_NAME,
		0,
		NULL
	},

	i2c_elan_init_device,
	i2c_elan_uninit_device,
	NULL,

	i2c_elan_open,
	i2c_elan_close,
	i2c_elan_free,
	i2c_elan_read,
	i2c_elan_write,
	NULL,
	i2c_elan_control,

	NULL,
	NULL
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


module_info *modules[] = {
	(module_info *)&i2c_elan_driver_module,
	(module_info *)&i2c_elan_device_module,
	NULL
};

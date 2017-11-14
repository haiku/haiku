/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


/*!	Driver functions that adapt the FreeBSD driver to Haiku's driver API.
	The actual driver functions are exported by the HAIKU_FBSD_DRIVER_GLUE
	macro, and just call the functions here.
*/


#include "device.h"

#include <stdlib.h>
#include <sys/sockio.h>

#include <Drivers.h>
#include <ether_driver.h>
#include <PCI_x86.h>

#include <compat/sys/haiku-module.h>

#include <compat/sys/bus.h>
#include <compat/sys/mbuf.h>
#include <compat/net/ethernet.h>


//#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


const char *gDeviceNameList[MAX_DEVICES + 1];
struct ifnet *gDevices[MAX_DEVICES];
int32 gDeviceCount;


static status_t
init_root_device(device_t *_root)
{
	static driver_t sRootDriver = {
		"pci",
		NULL,
		sizeof(struct root_device_softc)
	};

	device_t root = device_add_child(NULL, NULL, 0);
	if (root == NULL)
		return B_NO_MEMORY;

	root->softc = malloc(sizeof(struct root_device_softc));
	if (root->softc == NULL) {
		device_delete_child(NULL, root);
		return B_NO_MEMORY;
	}

	bzero(root->softc, sizeof(struct root_device_softc));
	root->driver = &sRootDriver;
	root->root = root;

	if (_root != NULL)
		*_root = root;

	return B_OK;
}


static status_t
add_child_device(driver_t *driver, device_t root, device_t *_child)
{
	device_t child = device_add_child(root, driver->name, 0);
	if (child == NULL) {
		return B_ERROR;
	}

	if (_child != NULL)
		*_child = child;

	return B_OK;
}


static pci_info *
get_pci_info(struct device *device)
{
	return &((struct root_device_softc *)device->softc)->pci_info;
}


//	#pragma mark - Haiku Driver API


status_t
_fbsd_init_hardware(driver_t *drivers[])
{
	status_t status = B_ENTRY_NOT_FOUND;
	device_t root;
	pci_info *info;
	driver_t *driver = NULL;
	int i = 0;

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK)
		return B_ERROR;

	status = init_root_device(&root);
	if (status != B_OK)
		goto err;

	for (info = get_pci_info(root); gPci->get_nth_pci_info(i, info) == B_OK;
		i++) {
		int index;
		driver = NULL;

		for (index = 0; drivers[index] && gDeviceCount < MAX_DEVICES
			&& driver == NULL; index++) {
			int result;
			device_t device;
			status = add_child_device(drivers[index], root, &device);
			if (status < B_OK)
				break;

			result = device->methods.probe(device);
			if (result >= 0) {
				TRACE(("%s, found %s at %d\n", gDriverName,
					device_get_desc(device), i));
				driver = drivers[index];
			}
			device_delete_child(root, device);
		}

		if (driver != NULL)
			break;
	}

	device_delete_child(NULL, root);

	if (driver == NULL) {
		status = B_ERROR;
		TRACE(("%s: no hardware found.\n", gDriverName));
	}
err:
	put_module(B_PCI_MODULE_NAME);
	TRACE(("%s: status 0x%lx\n", gDriverName, status));

	return status;
}


status_t
_fbsd_init_drivers(driver_t *drivers[])
{
	status_t status;
	int i = 0;
	int index = 0;
	pci_info *info;
	device_t root;

	status = get_module(B_PCI_MODULE_NAME, (module_info **)&gPci);
	if (status < B_OK)
		return status;

	// if it fails we just don't support x86 specific features (like MSIs)
	if (get_module(B_PCI_X86_MODULE_NAME, (module_info **)&gPCIx86) != B_OK)
		gPCIx86 = NULL;

	status = init_hard_clock();
	if (status < B_OK)
		goto err1;

	status = init_mutexes();
	if (status < B_OK)
		goto err2;

	status = init_mbufs();
	if (status < B_OK)
		goto err3;

	status = init_callout();
	if (status < B_OK)
		goto err4;

	init_bounce_pages();

	if (HAIKU_DRIVER_REQUIRES(FBSD_TASKQUEUES)) {
		status = init_taskqueues();
		if (status < B_OK)
			goto err5;
	}

	status = init_wlan_stack();
	if (status < B_OK)
		goto err6;

	status = init_root_device(&root);
	if (status != B_OK)
		goto err7;

	for (info = get_pci_info(root); gPci->get_nth_pci_info(i, info) == B_OK;
		i++) {
		int best = 0;
		driver_t *driver = NULL;

		for (index = 0; drivers[index] && gDeviceCount < MAX_DEVICES; index++) {
			int result;
			device_t device;
			status = add_child_device(drivers[index], root, &device);
			if (status < B_OK)
				break;

			result = device->methods.probe(device);
			if (result >= 0 && (driver == NULL || result > best)) {
				TRACE(("%s, found %s at %d (%d)\n", gDriverName,
					device_get_desc(device), i, result));
				driver = drivers[index];
				best = result;
			}
			device_delete_child(root, device);
		}

		if (driver != NULL) {
			device_t device;
			status = add_child_device(driver, root, &device);
			if (status != B_OK)
				break;
			if (device_attach(device) == 0) {
				dprintf("%s: init_driver(%p) at %d\n", gDriverName, driver, i);
			} else
				device_delete_child(root, device);
		}
	}

	if (gDeviceCount > 0)
		return B_OK;

	device_delete_child(NULL, root);

	if (status == B_OK)
		status = B_ERROR;

err7:
	uninit_wlan_stack();

err6:
	if (HAIKU_DRIVER_REQUIRES(FBSD_TASKQUEUES))
		uninit_taskqueues();
err5:
	uninit_mbufs();
err4:
	uninit_callout();
err3:
	uninit_mutexes();
err2:
	uninit_hard_clock();
err1:
	put_module(B_PCI_MODULE_NAME);
	if (gPCIx86 != NULL)
		put_module(B_PCI_X86_MODULE_NAME);

	return status;
}


status_t
_fbsd_uninit_drivers(driver_t *drivers[])
{
	int i;

	for (i = 0; drivers[i]; i++)
		TRACE(("%s: uninit_driver(%p)\n", gDriverName, drivers[i]));

	for (i = 0; i < gDeviceCount; i++) {
		device_delete_child(NULL, gDevices[i]->root_device);
	}

	uninit_wlan_stack();
	uninit_bounce_pages();
	uninit_mbufs();
	if (HAIKU_DRIVER_REQUIRES(FBSD_TASKQUEUES))
		uninit_taskqueues();
	uninit_callout();
	uninit_mutexes();
	uninit_hard_clock();

	put_module(B_PCI_MODULE_NAME);
	if (gPCIx86 != NULL)
		put_module(B_PCI_X86_MODULE_NAME);

	return B_OK;
}

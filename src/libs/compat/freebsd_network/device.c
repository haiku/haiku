/*
 * Copyright 2007, Hugo Santos, hugosantos@gmail.com. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <stdio.h>

#include <KernelExport.h>
#include <image.h>
#include <kernel/heap.h>

#include <compat/machine/resource.h>
#include <compat/dev/mii/mii.h>
#include <compat/sys/bus.h>
#include <compat/net/if_media.h>

#include <compat/dev/mii/miivar.h>


spinlock __haiku_intr_spinlock;

struct net_stack_module_info *gStack;
pci_module_info *gPci;
struct pci_x86_module_info *gPCIx86;

static struct list sRootDevices;
static int sNextUnit;

//	#pragma mark - private functions


static device_t
init_device(device_t device, driver_t *driver)
{
	list_init_etc(&device->children, offsetof(struct device, link));
	device->unit = sNextUnit++;

	if (driver != NULL && device_set_driver(device, driver) < 0)
		return NULL;

	return device;
}


static device_t
new_device(driver_t *driver)
{
	device_t dev = malloc(sizeof(struct device));
	if (dev == NULL)
		return NULL;

	memset(dev, 0, sizeof(struct device));

	if (init_device(dev, driver) == NULL) {
		free(dev);
		return NULL;
	}

	return dev;
}


static image_id
find_own_image()
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info(B_SYSTEM_TEAM, &cookie, &info) == B_OK) {
		if (((addr_t)info.text <= (addr_t)find_own_image
			&& (addr_t)info.text + (addr_t)info.text_size
				> (addr_t)find_own_image)) {
			// found our own image
			return info.id;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static device_method_signature_t
resolve_method(driver_t *driver, const char *name)
{
	device_method_signature_t method = NULL;
	int i;

	for (i = 0; method == NULL && driver->methods[i].name != NULL; i++) {
		if (strcmp(driver->methods[i].name, name) == 0)
			method = driver->methods[i].method;
	}

	if (method == NULL)
		panic("resolve_method: method%s not found\n", name);

	return method;
}


//	#pragma mark - Device


void
driver_printf(const char *format, ...)
{
	va_list vl;
	va_start(vl, format);
	driver_vprintf(format, vl);
	va_end(vl);
}


static int
driver_vprintf_etc(const char *extra, const char *format, va_list vl)
{
	char buf[256];
	int ret = vsnprintf(buf, sizeof(buf), format, vl);

	if (extra)
		dprintf("[%s] (%s) %s", gDriverName, extra, buf);
	else
		dprintf("[%s] %s", gDriverName, buf);

	return ret;
}


int
driver_vprintf(const char *format, va_list vl)
{
	return driver_vprintf_etc(NULL, format, vl);
}


int
device_printf(device_t dev, const char *format, ...)
{
	va_list vl;

	va_start(vl, format);
	driver_vprintf_etc(dev->device_name, format, vl);
	va_end(vl);
	return 0;
}


void
device_set_desc(device_t dev, const char *desc)
{
	dev->description = desc;
}


void
device_set_desc_copy(device_t dev, const char *desc)
{
	dev->description = strdup(desc);
	dev->flags |= DEVICE_DESC_ALLOCED;
}


const char *
device_get_desc(device_t dev)
{
	return dev->description;
}


device_t
device_get_parent(device_t dev)
{
	return dev->parent;
}


devclass_t
device_get_devclass(device_t dev)
{
	// TODO find out what to do
	return 0;
}


int
device_get_children(device_t dev, device_t **devlistp, int *devcountp)
{
	int count;
	device_t child = NULL;
	device_t *list;

	count = 0;
	while ((child = list_get_next_item(&dev->children, child)) != NULL) {
		count++;
	}

	if (count == 0) {
		*devlistp = NULL;
		*devcountp = 0;
		return (0);
	}

	list = malloc(count * sizeof(device_t));
	if (!list)
		return (ENOMEM);

	count = 0;
	while ((child = list_get_next_item(&dev->children, child)) != NULL) {
		list[count] = child;
		count++;
	}

	*devlistp = list;
	*devcountp = count;

	return (0);
}


void
device_set_ivars(device_t dev, void *ivars)
{
	dev->ivars = ivars;
}


void *
device_get_ivars(device_t dev)
{
	return dev->ivars;
}


const char *
device_get_name(device_t dev)
{
	if (dev == NULL)
		return NULL;

	return dev->device_name;
}


int
device_get_unit(device_t dev)
{
	return dev->unit;
}


const char *
device_get_nameunit(device_t dev)
{
	return dev->nameunit;
}


void *
device_get_softc(device_t dev)
{
	return dev->softc;
}


void
device_set_softc(device_t dev, void *softc)
{
	if (dev->softc == softc)
		return;

	if ((dev->flags & DEVICE_SOFTC_SET) == 0) {
		// Not externally allocated. We own it so we must clean it up.
		free(dev->softc);
	}

	dev->softc = softc;
	if (dev->softc != NULL)
		dev->flags |= DEVICE_SOFTC_SET;
	else
		dev->flags &= ~DEVICE_SOFTC_SET;
}


u_int32_t
device_get_flags(device_t dev)
{
	return dev->flags;
}


int
device_set_driver(device_t dev, driver_t *driver)
{
	int i;

	dev->softc = malloc(driver->size);
	if (dev->softc == NULL)
		return -1;

	memset(dev->softc, 0, driver->size);
	dev->driver = driver;

	for (i = 0; driver->methods[i].name != NULL; i++) {
		device_method_t *mth = &driver->methods[i];

		if (strcmp(mth->name, "device_register") == 0)
			dev->methods.device_register = (void *)mth->method;
		else if (strcmp(mth->name, "device_probe") == 0)
			dev->methods.probe = (void *)mth->method;
		else if (strcmp(mth->name, "device_attach") == 0)
			dev->methods.attach = (void *)mth->method;
		else if (strcmp(mth->name, "device_detach") == 0)
			dev->methods.detach = (void *)mth->method;
		else if (strcmp(mth->name, "device_suspend") == 0)
			dev->methods.suspend = (void *)mth->method;
		else if (strcmp(mth->name, "device_resume") == 0)
			dev->methods.resume = (void *)mth->method;
		else if (strcmp(mth->name, "device_shutdown") == 0)
			dev->methods.shutdown = (void *)mth->method;
		else if (strcmp(mth->name, "miibus_readreg") == 0)
			dev->methods.miibus_readreg = (void *)mth->method;
		else if (strcmp(mth->name, "miibus_writereg") == 0)
			dev->methods.miibus_writereg = (void *)mth->method;
		else if (strcmp(mth->name, "miibus_statchg") == 0)
			dev->methods.miibus_statchg = (void *)mth->method;
		else if (!strcmp(mth->name, "miibus_linkchg"))
			dev->methods.miibus_linkchg = (void *)mth->method;
		else if (!strcmp(mth->name, "miibus_mediainit"))
			dev->methods.miibus_mediainit = (void *)mth->method;
		else if (!strcmp(mth->name, "bus_child_location_str"))
			dev->methods.bus_child_location_str = (void *)mth->method;
		else if (!strcmp(mth->name, "bus_child_pnpinfo_str"))
			dev->methods.bus_child_pnpinfo_str = (void *)mth->method;
		else if (!strcmp(mth->name, "bus_hinted_child"))
			dev->methods.bus_hinted_child = (void *)mth->method;
		else if (!strcmp(mth->name, "bus_print_child"))
			dev->methods.bus_print_child = (void *)mth->method;
		else if (!strcmp(mth->name, "bus_read_ivar"))
			dev->methods.bus_read_ivar = (void *)mth->method;
		else if (!strcmp(mth->name, "bus_get_dma_tag"))
			dev->methods.bus_get_dma_tag = (void *)mth->method;
		else
			panic("device_set_driver: method %s not found\n", mth->name);

	}

	return 0;
}


int
device_is_alive(device_t device)
{
	return (device->flags & DEVICE_ATTACHED) != 0;
}


device_t
device_add_child_driver(device_t parent, const char* name, driver_t* _driver,
	int unit)
{
	device_t child = NULL;

	if (_driver == NULL && name != NULL) {
		if (strcmp(name, "miibus") == 0)
			child = new_device(&miibus_driver);
		else {
			// find matching driver structure
			driver_t** driver;
			char symbol[128];

			snprintf(symbol, sizeof(symbol), "__fbsd_%s_%s", name,
				parent->driver->name);
			if (get_image_symbol(find_own_image(), symbol, B_SYMBOL_TYPE_DATA,
					(void**)&driver) == B_OK) {
				child = new_device(*driver);
			} else
				device_printf(parent, "couldn't find symbol %s\n", symbol);
		}
	} else if (_driver != NULL) {
		child = new_device(_driver);
	} else
		child = new_device(NULL);

	if (child == NULL)
		return NULL;

	if (name != NULL)
		strlcpy(child->device_name, name, sizeof(child->device_name));

	child->parent = parent;

	if (parent != NULL) {
		list_add_item(&parent->children, child);
		child->root = parent->root;
	} else {
		if (sRootDevices.link.next == NULL)
			list_init_etc(&sRootDevices, offsetof(struct device, link));
		list_add_item(&sRootDevices, child);
	}

	return child;
}


device_t
device_add_child(device_t parent, const char* name, int unit)
{
	return device_add_child_driver(parent, name, NULL, unit);
}


/*!	Delete the child and all of its children. Detach as necessary.
*/
int
device_delete_child(device_t parent, device_t child)
{
	int status;

	if (child == NULL)
		return 0;

	if (parent != NULL)
		list_remove_item(&parent->children, child);
	else
		list_remove_item(&sRootDevices, child);

	// We differentiate from the FreeBSD logic here - it will first delete
	// the children, and will then detach the device.
	// This has the problem that you cannot safely call device_delete_child()
	// as you don't know if one of the children deletes its own children this
	// way when it is detached.
	// Therefore, we'll detach first, and then delete whatever is left.

	parent = child;
	child = NULL;

	// detach children
	while ((child = list_get_next_item(&parent->children, child)) != NULL) {
		device_detach(child);
	}

	// detach device
	status = device_detach(parent);
	if (status != 0)
		return status;

	// delete children
	while ((child = list_get_first_item(&parent->children)) != NULL) {
		device_delete_child(parent, child);
	}

	// delete device
	if (parent->flags & DEVICE_DESC_ALLOCED)
		free((char *)parent->description);

	// Delete softc if we were the ones to allocate it.
	if ((parent->flags & DEVICE_SOFTC_SET) == 0)
		free(parent->softc);

	free(parent);
	return 0;
}


int
device_is_attached(device_t device)
{
	return (device->flags & DEVICE_ATTACHED) != 0;
}


int
device_attach(device_t device)
{
	int result;

	if (device->driver == NULL
		|| device->methods.attach == NULL)
		return B_ERROR;

	result = device->methods.attach(device);

	if (result == 0)
		atomic_or(&device->flags, DEVICE_ATTACHED);

	if (result == 0 && HAIKU_DRIVER_REQUIRES(FBSD_WLAN_FEATURE))
		result = start_wlan(device);

	return result;
}


int
device_detach(device_t device)
{
	if (device->driver == NULL)
		return B_ERROR;

	if ((atomic_and(&device->flags, ~DEVICE_ATTACHED) & DEVICE_ATTACHED) != 0
			&& device->methods.detach != NULL) {
		int result = 0;
		if (HAIKU_DRIVER_REQUIRES(FBSD_WLAN_FEATURE))
			result = stop_wlan(device);
		if (result != 0 && result != B_BAD_VALUE) {
			atomic_or(&device->flags, DEVICE_ATTACHED);
			return result;
		}

		result = device->methods.detach(device);
		if (result != 0) {
			atomic_or(&device->flags, DEVICE_ATTACHED);
			return result;
		}
	}

	return 0;
}


int
bus_generic_attach(device_t dev)
{
	device_t child = NULL;

	while ((child = list_get_next_item(&dev->children, child)) != NULL) {
		if (child->driver == NULL) {
			driver_t *driver = __haiku_select_miibus_driver(child);
			if (driver == NULL) {
				struct mii_attach_args *ma = device_get_ivars(child);

				device_printf(dev, "No PHY module found (%x/%x)!\n",
					MII_OUI(ma->mii_id1, ma->mii_id2), MII_MODEL(ma->mii_id2));
			} else
				device_set_driver(child, driver);
		} else
			child->methods.probe(child);

		if (child->driver != NULL) {
			int result = device_attach(child);
			if (result != 0)
				return result;
		}
	}

	return 0;
}


int
bus_generic_detach(device_t device)
{
	device_t child = NULL;

	if ((device->flags & DEVICE_ATTACHED) == 0)
		return B_ERROR;

	while (true) {
		child = list_get_next_item(&device->children, child);
		if (child == NULL)
			break;

		device_detach(child);
	}

	return 0;
}


//	#pragma mark - Misc, Malloc


device_t
find_root_device(int unit)
{
	device_t device = NULL;

	while ((device = list_get_next_item(&sRootDevices, device)) != NULL) {
		if (device->unit <= unit)
			return device;
	}

	return NULL;
}


driver_t *
__haiku_probe_miibus(device_t dev, driver_t *drivers[])
{
	driver_t *selected = NULL;
	int i, selectedResult = 0;

	if (drivers == NULL)
		return NULL;

	for (i = 0; drivers[i]; i++) {
		device_probe_t *probe = (device_probe_t *)
			resolve_method(drivers[i], "device_probe");
		if (probe) {
			int result = probe(dev);
			if (result >= 0) {
				if (selected == NULL || result < selectedResult) {
					selected = drivers[i];
					selectedResult = result;
					device_printf(dev, "Found MII: %s\n", selected->name);
				}
			}
		}
	}

	return selected;
}


int
printf(const char *format, ...)
{
	char buf[256];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buf, sizeof(buf), format, vl);
	va_end(vl);
	dprintf("%s", buf);

	return 0;
}


int
resource_int_value(const char *name, int unit, const char *resname,
	int *result)
{
	/* no support for hints */
	return -1;
}


int
resource_disabled(const char *name, int unit)
{
	int error, value;

	error = resource_int_value(name, unit, "disabled", &value);
	if (error)
	       return (0);
	return (value);
}

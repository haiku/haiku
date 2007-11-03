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

#include <compat/machine/resource.h>
#include <compat/dev/pci/pcivar.h>
#include <compat/sys/bus.h>

#include <compat/dev/mii/miivar.h>

//#define DEBUG_PCI
#ifdef DEBUG_PCI
#	define TRACE_PCI(dev, format, args...) device_printf(dev, format , ##args)
#else
#	define TRACE_PCI(dev, format, args...) do { } while (0)
#endif


spinlock __haiku_intr_spinlock;

struct net_stack_module_info *gStack;
pci_module_info *gPci;


//	#pragma mark - PCI


uint32_t
pci_read_config(device_t dev, int offset, int size)
{
	uint32_t value = gPci->read_pci_config(NETDEV(dev)->pci_info.bus,
		NETDEV(dev)->pci_info.device, NETDEV(dev)->pci_info.function,
		offset, size);
	TRACE_PCI(dev, "pci_read_config(%i, %i) = 0x%x\n", offset, size, value);
	return value;
}


void
pci_write_config(device_t dev, int offset, uint32_t value, int size)
{
	TRACE_PCI(dev, "pci_write_config(%i, 0x%x, %i)\n", offset, value, size);

	gPci->write_pci_config(NETDEV(dev)->pci_info.bus,
		NETDEV(dev)->pci_info.device, NETDEV(dev)->pci_info.function,
		offset, size, value);
}


uint16_t
pci_get_vendor(device_t dev)
{
	return pci_read_config(dev, PCI_vendor_id, 2);
}


uint16_t
pci_get_device(device_t dev)
{
	return pci_read_config(dev, PCI_device_id, 2);
}


uint16_t
pci_get_subvendor(device_t dev)
{
	return pci_read_config(dev, PCI_subsystem_vendor_id, 2);
}


uint16_t
pci_get_subdevice(device_t dev)
{
	return pci_read_config(dev, PCI_subsystem_id, 2);
}


uint8_t
pci_get_revid(device_t dev)
{
	return pci_read_config(dev, PCI_revision, 1);
}


static void
pci_set_command_bit(device_t dev, uint16_t bit)
{
	uint16_t command = pci_read_config(dev, PCI_command, 2);
	pci_write_config(dev, PCI_command, command | bit, 2);
}


int
pci_enable_busmaster(device_t dev)
{
	pci_set_command_bit(dev, PCI_command_master);
	return 0;
}


int
pci_enable_io(device_t dev, int space)
{
	/* adapted from FreeBSD's pci_enable_io_method */
	int bit = 0;

	switch (space) {
		case SYS_RES_IOPORT:
			bit = PCI_command_io;
			break;
		case SYS_RES_MEMORY:
			bit = PCI_command_memory;
			break;
		default:
			return EINVAL;
	}

	pci_set_command_bit(dev, bit);
	if (pci_read_config(dev, PCI_command, 2) & bit)
		return 0;

	device_printf(dev, "pci_enable_io(%d) failed.\n", space);

	return ENXIO;
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


static void
driver_vprintf_etc(const char *extra, const char *format, va_list vl)
{
	char buf[256];
	vsnprintf(buf, sizeof(buf), format, vl);

	if (extra)
		dprintf("[%s] (%s) %s", gDriverName, extra, buf);
	else
		dprintf("[%s] %s", gDriverName, buf);
}


void
driver_vprintf(const char *format, va_list vl)
{
	driver_vprintf_etc(NULL, format, vl);
}


int
device_printf(device_t dev, const char *format, ...)
{
	va_list vl;

	va_start(vl, format);
	driver_vprintf_etc(dev->dev_name, format, vl);
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


void
device_sprintf_name(device_t dev, const char *format, ...)
{
	va_list vl;
	va_start(vl, format);
	vsnprintf(dev->dev_name, sizeof(dev->dev_name), format, vl);
	va_end(vl);
}


const char *
device_get_name(device_t dev)
{
	if (dev)
		return dev->dev_name;
	return NULL;
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


device_t
init_device(device_t dev, driver_t *driver)
{
	list_init_etc(&dev->children, offsetof(struct device, link));

	if (driver == NULL)
		return dev;

	if (device_set_driver(dev, driver) < 0)
		return NULL;

	return dev;
}


int
device_set_driver(device_t dev, driver_t *driver)
{
	device_method_signature_t method = NULL;
	int i;

	dev->softc = malloc(driver->softc_size);
	if (dev->softc == NULL)
		return -1;

	memset(dev->softc, 0, driver->softc_size);
	dev->driver = driver;

	for (i = 0; method == NULL && driver->methods[i].name != NULL; i++) {
		device_method_t *mth = &driver->methods[i];
		if (strcmp(mth->name, "device_probe") == 0)
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
	}

	return 0;
}


void
uninit_device(device_t dev)
{
	if (dev->flags & DEVICE_DESC_ALLOCED)
		free((char *)dev->description);
	if (dev->softc)
		free(dev->softc);
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


device_t
device_add_child(device_t dev, const char *name, int order)
{
	device_t child = NULL;

	if (name != NULL) {
		if (strcmp(name, "miibus") == 0)
			child = new_device(&miibus_driver);
	} else
		child = new_device(NULL);

	if (child == NULL)
		return NULL;

	snprintf(child->dev_name, sizeof(child->dev_name), "%s", name);
	child->parent = dev;
	list_add_item(&dev->children, child);

	return child;
}


driver_t *
__haiku_probe_miibus(device_t dev, driver_t *drivers[], int count)
{
	driver_t *selected = NULL;
	int i, selcost = 0;

	for (i = 0; i < count; i++) {
		device_probe_t *probe = (device_probe_t *)
			_resolve_method(drivers[i], "device_probe");
		if (probe) {
			int result = probe(dev);
			// the ukphy driver (fallback for unknown PHYs) return -100 here
			if (result >= -100) {
				if (selected == NULL || result > selcost) {
					selected = drivers[i];
					selcost = result;
					device_printf(dev, "Found MII: %s\n", selected->name);
				}
			}
		}
	}

	return selected;
}


void
bus_generic_attach(device_t dev)
{
	device_t child = NULL;

	while ((child = list_get_next_item(&dev->children, child)) != NULL) {
		if (child->driver == NULL) {
			driver_t *driver = __haiku_select_miibus_driver(child);
			if (driver == NULL) {
				struct mii_attach_args *ma = device_get_ivars(child);

				device_printf(dev, "No PHY module found (%x/%x)!\n", ma->mii_id1,
					ma->mii_id2);
			} else
				device_set_driver(child, driver);
		} else if (child->driver == &miibus_driver)
			child->methods.probe(child);

		if (child->driver != NULL)
			child->methods.attach(child);
	}
}


int
device_delete_child(device_t dev, device_t child)
{
	UNIMPLEMENTED();
	return -1;
}


int
device_is_attached(device_t dev)
{
	UNIMPLEMENTED();
	return -1;
}


//	#pragma mark - Misc, Malloc


int
printf(const char *format, ...)
{
	char buf[256];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buf, sizeof(buf), format, vl);
	va_end(vl);
	dprintf(buf);

	return 0;
}


int
ffs(int value)
{
	int i = 1;

	if (value == 0)
		return 0;

	for (; !(value & 1); i++)
		value >>= 1;

	return i;
}


int resource_int_value(const char *name, int unit, const char *resname,
	int *result)
{
	/* no support for hints */
	return -1;
}


void *
_kernel_malloc(size_t size, int flags)
{
	// our kernel malloc() is insufficent, must handle M_WAIT

	void *ptr = malloc(size);
	if (ptr == NULL)
		return ptr;

	if (flags & M_ZERO)
		memset(ptr, 0, size);

	return ptr;
}


void
_kernel_free(void *ptr)
{
	free(ptr);
}


void *
_kernel_contigmalloc(const char *file, int line, size_t size, int flags,
	vm_paddr_t low, vm_paddr_t high, unsigned long alignment,
	unsigned long boundary)
{
	char name[256];
	area_id area;
	void *addr;

	snprintf(name, sizeof(name), "contig:%s:%d", file, line);

	area = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, size,
		B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < 0)
		return NULL;

	if (flags & M_ZERO)
		memset(addr, 0, size);

	driver_printf("(%s) addr = %p, area = %ld, size = %lu\n",
		name, addr, area, size);

	return addr;
}


void
_kernel_contigfree(void *addr, size_t size)
{
	delete_area(area_for(addr));
}


vm_paddr_t
pmap_kextract(vm_offset_t virtualAddress)
{
	physical_entry entry;
	status_t status = get_memory_map((void *)virtualAddress, 1, &entry, 1);
	if (status < B_OK) {
		panic("fbsd compat: get_memory_map failed for %p, error %08lx\n",
			(void *)virtualAddress, status);
	}

	return (vm_paddr_t)entry.address;
}


status_t
init_compat_layer()
{
	__haiku_intr_spinlock = 0;
	return B_OK;
}

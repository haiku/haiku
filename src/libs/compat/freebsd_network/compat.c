/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */

#include "device.h"

#include <stdio.h>

#include <KernelExport.h>

#include <compat/machine/resource.h>
#include <compat/dev/pci/pcivar.h>
#include <compat/sys/bus.h>

// TODO a lot.

#define DEBUG_PCI

#ifdef DEBUG_PCI
#define TRACE_PCI(dev, format, args...) device_printf(dev, format , ##args)
#else
#define TRACE_PCI(dev, format, args...) do { } while (0)
#endif

spinlock __haiku_intr_spinlock;

struct net_stack_module_info *gStack;
pci_module_info *gPci;

uint32_t
pci_read_config(device_t dev, int offset, int size)
{
	uint32_t value = gPci->read_pci_config(dev->pci_info.bus,
		dev->pci_info.device, dev->pci_info.function, offset, size);
	TRACE_PCI(dev, "pci_read_config(%i, %i) = 0x%lx\n", offset, size, value);
	return value;
}


void
pci_write_config(device_t dev, int offset, uint32_t value, int size)
{
	TRACE_PCI(dev, "pci_write_config(%i, 0x%lx, %i)\n", offset, value, size);

	gPci->write_pci_config(dev->pci_info.bus, dev->pci_info.device,
		dev->pci_info.function, offset, size, value);
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
	char devDesc[32];
	va_list vl;

	snprintf(devDesc, sizeof(devDesc), "%i:%i:%i", (int)dev->pci_info.bus,
		(int)dev->pci_info.device, (int)dev->pci_info.function);

	va_start(vl, format);
	driver_vprintf_etc(devDesc, format, vl);
	va_end(vl);
	return 0;
}


void
device_set_desc(device_t dev, const char *desc)
{
	dev->description = desc;
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


int
device_delete_child(device_t dev, device_t child)
{
	return -1;
}


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
		B_FULL_LOCK | B_CONTIGUOUS, 0);
	if (area < 0)
		return NULL;

	return addr;
}


void
_kernel_contigfree(void *addr, size_t size)
{
	delete_area(area_for(addr));
}


status_t
init_compat_layer()
{
	__haiku_intr_spinlock = 0;
	return B_OK;
}

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&gStack},
	{B_PCI_MODULE_NAME, (module_info **)&gPci},
	{}
};

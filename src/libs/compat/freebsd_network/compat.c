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

#include <compat/dev/pci/pcivar.h>
#include <compat/sys/bus.h>

// TODO a lot.

status_t init_compat_layer(void);

pci_module_info *gPci;

uint32_t
pci_read_config(device_t dev, int offset, int size)
{
	return gPci->read_pci_config(dev->pciInfo->bus, dev->pciInfo->device,
		dev->pciInfo->function, offset, size);
}


void
pci_write_config(device_t dev, int offset, uint32_t value, int size)
{
	gPci->write_pci_config(dev->pciInfo->bus, dev->pciInfo->device,
		dev->pciInfo->function, offset, size, value);
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


uint8_t
pci_get_revid(device_t dev)
{
	return pci_read_config(dev, PCI_revision, 1);
}


int
device_printf(device_t dev, const char *format, ...)
{
	char buf[256];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buf, sizeof(buf), format, vl);
	va_end(vl);

	dprintf("[...] %s", buf);
	return 0;
}


void *
_kernel_malloc(size_t size, int flags)
{
	// our kernel malloc() is insufficent

	return malloc(size);
}


void
_kernel_free(void *ptr)
{
	free(ptr);
}


status_t
init_compat_layer()
{
	return B_OK;
}

module_dependency module_dependencies[] = {
	{B_PCI_MODULE_NAME, (module_info **)&gPci},
	{}
};

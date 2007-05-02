/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */


#include <KernelExport.h>
#include <drivers/PCI.h>

#include <compat/dev/pci/pcivar.h>
#include <compat/sys/bus.h>

// TODO a lot.

status_t init_compat_layer(void);

pci_module_info *gPci;

struct device {
	int			devId;
	pci_info *	pciInfo;

	uint8		pciBus;
	uint8		pciDev;
	uint8		pciFunc;
};

uint32_t
pci_read_config(device_t dev, int offset, int size)
{
	return gPci->read_pci_config(dev->pciBus, dev->pciDev, dev->pciFunc,
		offset, size);
}


void
pci_write_config(device_t dev, int offset, uint32_t value, int size)
{
	gPci->write_pci_config(dev->pciBus, dev->pciDev, dev->pciFunc, offset,
		size, value);
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

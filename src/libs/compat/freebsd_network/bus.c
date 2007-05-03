/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */

#include <malloc.h>

#include <drivers/PCI.h>

#include <compat/dev/pci/pcivar.h>
#include <compat/machine/resource.h>
#include <compat/sys/bus.h>


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))


struct internal_intr {
	driver_intr_t handler;
	void *arg;
	int irq;
};


static area_id
map_mem(void **virtualAddr, void *_phy, size_t size, uint32 protection,
	const char *name)
{
	uint32 offset = (uint32)_phy & (B_PAGE_SIZE - 1);
	void *physicalAddr = (uint8 *)_phy - offset;
	area_id area;

	size = ROUNDUP(size + offset, B_PAGE_SIZE);
	area = map_physical_memory(name, physicalAddr, size, B_ANY_KERNEL_ADDRESS,
		protection, virtualAddr);
	if (area < B_OK)
		return area;

	*virtualAddr = (uint8 *)(*virtualAddr) + offset;

	return area;
}


/* straight from Marcus' ipro1000 driver */
struct resource *
bus_alloc_resource(device_t dev, int type, int *rid, unsigned long start,
	unsigned long end, unsigned long count, uint32 flags)
{
	switch (type) {
		case SYS_RES_IRQ:
		{
			uint8 res = pci_read_config(dev, PCI_interrupt_line, 1);
			if (res == 0 || res == 0xff)
				return NULL;
			return (struct resource *)(int)res;
		}

		case SYS_RES_MEMORY:
		{
			uint32 res = pci_read_config(dev, *rid, 4)
				& PCI_address_memory_32_mask;
			uint32 size = 128 * 1024; // TODO get size from BAR
			void *virtualAddr;
			if (map_mem(&virtualAddr, (void *)res, size, 0, "bus_alloc_resource(MEMORY)") < 0)
				return NULL;
			return (struct resource *)virtualAddr;
		}

		case SYS_RES_IOPORT:
			return (struct resource *)(pci_read_config(dev, *rid, 4)
				& PCI_address_io_mask);

		default:
			return NULL;
	}
}


int
bus_release_resource(device_t dev, int type, int rid, struct resource *res)
{
	switch (type) {
		case SYS_RES_IRQ:
		case SYS_RES_IOPORT:
			return 0;

		case SYS_RES_MEMORY:
			delete_area(area_for(res));
			return 0;

		default:
			return B_ERROR;
	}
}


static int32
intr_wrapper(void *data)
{
	struct internal_intr *intr = data;
	intr->handler(intr->arg);
	return 0;
}


int
bus_setup_intr(device_t dev, struct resource *res, int flags,
	driver_intr_t handler, void *arg, void **cookiep)
{
	struct internal_intr *intr = (struct internal_intr *)malloc(
		sizeof(struct internal_intr));
	status_t status;

	if (intr == NULL)
		return B_NO_MEMORY;

	intr->handler = handler;
	intr->arg = arg;
	intr->irq = (int)res;

	status = install_io_interrupt_handler(intr->irq, intr_wrapper, intr, 0);
	if (status < B_OK) {
		free(intr);
		return status;
	}

	*cookiep = intr;

	return status;
}


int
bus_teardown_intr(device_t dev, struct resource *res, void *arg)
{
	struct internal_intr *intr = arg;
	remove_io_interrupt_handler(intr->irq, intr_wrapper, intr);
	free(intr);
	return 0;
}


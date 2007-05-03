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
#include <compat/sys/rman.h>


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))


struct resource {
	int					type;
	bus_space_tag_t		tag;
	bus_space_handle_t	handle;

	area_id				mapped_area;
};


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


static int
bus_alloc_irq_resource(device_t dev, struct resource *res)
{
	uint8 irq = pci_read_config(dev, PCI_interrupt_line, 1);
	if (irq == 0 || irq == 0xff)
		return -1;

	/* XXX */
	res->tag = 0;
	res->handle = irq;

	return 0;
}


static int
bus_alloc_mem_resource(device_t dev, struct resource *res, int regid)
{
	uint32 addr = pci_read_config(dev, regid, 4) & PCI_address_memory_32_mask;
	uint32 size = 128 * 1024; /* XXX */
	void *virtualAddr;

	res->mapped_area = map_mem(&virtualAddr, (void *)addr, size, 0,
		"bus_alloc_resource(MEMORY)");

	if (res->mapped_area < 0)
		return -1;

	res->tag = I386_BUS_SPACE_MEM;
	res->handle = (bus_space_handle_t)virtualAddr;
	return 0;
}


static int
bus_alloc_ioport_resource(device_t dev, struct resource *res, int regid)
{
	res->tag = I386_BUS_SPACE_IO;
	res->handle = pci_read_config(dev, regid, 4) & PCI_address_io_mask;
	return 0;
}


struct resource *
bus_alloc_resource(device_t dev, int type, int *rid, unsigned long start,
	unsigned long end, unsigned long count, uint32 flags)
{
	struct resource *res;
	int result = -1;

	if (type != SYS_RES_IRQ && type != SYS_RES_MEMORY
		&& type != SYS_RES_IOPORT)
		return NULL;

	// maybe a local array of resources is enough
	res = malloc(sizeof(struct resource));
	if (res == NULL)
		return NULL;

	if (type == SYS_RES_IRQ)
		result = bus_alloc_irq_resource(dev, res);
	else if (type == SYS_RES_MEMORY)
		result = bus_alloc_mem_resource(dev, res, *rid);
	else if (type == SYS_RES_IOPORT)
		result = bus_alloc_ioport_resource(dev, res, *rid);

	if (result < 0) {
		free(res);
		return NULL;
	}

	res->type = type;
	return res;
}


int
bus_release_resource(device_t dev, int type, int rid, struct resource *res)
{
	if (res->type != type)
		panic("bus_release_resource: mismatch");

	if (type == SYS_RES_MEMORY)
		delete_area(res->mapped_area);

	free(res);
	return 0;
}


bus_space_handle_t
rman_get_bushandle(struct resource *res)
{
	return res->handle;
}


bus_space_tag_t
rman_get_bustag(struct resource *res)
{
	return res->tag;
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


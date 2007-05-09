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

#include <malloc.h>

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
	device_t dev;
	driver_intr_t handler;
	void *arg;
	int irq;

	thread_id thread;
	sem_id sem;
};


static int32 intr_wrapper(void *data);


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

	device_printf(dev, "bus_alloc_resource(%i, [%i], 0x%lx, 0x%lx, 0x%lx, 0x%lx)\n",
		type, *rid, start, end, count, flags);

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

	device_printf(intr->dev, "in interrupt handler.\n");

	if (!HAIKU_CHECK_DISABLE_INTERRUPTS(intr->dev))
		return B_UNHANDLED_INTERRUPT;

	release_sem_etc(intr->sem, 1, B_DO_NOT_RESCHEDULE);
	return B_INVOKE_SCHEDULER;
}


static int32
intr_fast_wrapper(void *data)
{
	struct internal_intr *intr = data;

	intr->handler(intr->arg);

	/* no way to know, so let other devices get it as well */
	return B_UNHANDLED_INTERRUPT;
}


static int32
intr_handler(void *data)
{
	struct internal_intr *intr = data;
	status_t status;

	while (1) {
		status = acquire_sem(intr->sem);
		if (status < B_OK)
			break;

		device_printf(intr->dev, "in soft interrupt handler.\n");

		intr->handler(intr->arg);
	}

	return 0;
}


static void
free_internal_intr(struct internal_intr *intr)
{
	if (intr->sem >= B_OK) {
		status_t status;
		delete_sem(intr->sem);
		wait_for_thread(intr->thread, &status);
	}

	free(intr);
}


int
bus_setup_intr(device_t dev, struct resource *res, int flags,
	driver_intr_t handler, void *arg, void **cookiep)
{
	/* TODO check MPSAFE etc */

	struct internal_intr *intr = (struct internal_intr *)malloc(
		sizeof(struct internal_intr));
	char semName[64];
	status_t status;

	if (intr == NULL)
		return B_NO_MEMORY;

	intr->dev = dev;
	intr->handler = handler;
	intr->arg = arg;
	intr->irq = res->handle;

	if (flags & INTR_FAST) {
		intr->sem = -1;
		intr->thread = -1;

		status = install_io_interrupt_handler(intr->irq,
			intr_fast_wrapper, intr, 0);
	} else {
		snprintf(semName, sizeof(semName), "%s intr", dev->dev_name);

		intr->sem = create_sem(0, semName);
		if (intr->sem < B_OK) {
			free(intr);
			return B_NO_MEMORY;
		}

		snprintf(semName, sizeof(semName), "%s intr handler", dev->dev_name);

		intr->thread = spawn_kernel_thread(intr_handler, semName,
			B_REAL_TIME_DISPLAY_PRIORITY, intr);
		if (intr->thread < B_OK) {
			delete_sem(intr->sem);
			free(intr);
			return B_NO_MEMORY;
		}

		status = install_io_interrupt_handler(intr->irq,
			intr_wrapper, intr, 0);
	}

	if (status < B_OK) {
		free_internal_intr(intr);
		return status;
	}

	resume_thread(intr->thread);

	*cookiep = intr;

	return 0;
}


int
bus_teardown_intr(device_t dev, struct resource *res, void *arg)
{
	struct internal_intr *intr = arg;
	remove_io_interrupt_handler(intr->irq, intr_wrapper, intr);
	free_internal_intr(intr);
	return 0;
}


int
bus_generic_detach(device_t dev)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


int
bus_generic_suspend(device_t dev)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


int
bus_generic_resume(device_t dev)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


#define DEBUG_BUS_SPACE_RW

#ifdef DEBUG_BUS_SPACE_RW
#define TRACE_BUS_SPACE_RW(x) driver_printf x
#else
#define TRACE_BUS_SPACE_RW(x)
#endif


#define BUS_SPACE_READ(size, type, fun) \
	type bus_space_read_##size(bus_space_tag_t tag, \
		bus_space_handle_t handle, bus_size_t offset) \
	{ \
		type value; \
		if (tag == I386_BUS_SPACE_IO) \
			value = fun(handle + offset); \
		else \
			value = *(volatile type *)(handle + offset); \
		if (tag == I386_BUS_SPACE_IO) \
			TRACE_BUS_SPACE_RW(("bus_space_read_%s(0x%lx, 0x%lx, 0x%lx) = 0x%lx\n", \
				#size, (uint32)tag, (uint32)handle, (uint32)offset, (uint32)value)); \
		return value; \
	}

#define BUS_SPACE_WRITE(size, type, fun) \
	void bus_space_write_##size(bus_space_tag_t tag, \
		bus_space_handle_t handle, bus_size_t offset, type value) \
	{ \
		if (tag == I386_BUS_SPACE_IO) \
			TRACE_BUS_SPACE_RW(("bus_space_write_%s(0x%lx, 0x%lx, 0x%lx, 0x%lx)\n", \
				#size, (uint32)tag, (uint32)handle, (uint32)offset, (uint32)value)); \
		if (tag == I386_BUS_SPACE_IO) \
			fun(value, handle + offset); \
		else \
			*(volatile type *)(handle + offset) = value; \
	}

BUS_SPACE_READ(1, uint8_t, in8)
BUS_SPACE_READ(2, uint16_t, in16)
BUS_SPACE_READ(4, uint32_t, in32)

BUS_SPACE_WRITE(1, uint8_t, out8)
BUS_SPACE_WRITE(2, uint16_t, out16)
BUS_SPACE_WRITE(4, uint32_t, out32)

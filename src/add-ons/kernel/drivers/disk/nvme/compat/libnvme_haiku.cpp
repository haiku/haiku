/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

#include <debug.h>
#include <kernel/vm/vm.h>
#include <PCI.h>

extern "C" {
#include "nvme.h"
#include "nvme_log.h"
#include "nvme_mem.h"
#include "nvme_pci.h"
}


static pci_module_info* sPCIModule = NULL;


// #pragma mark - memory


int
nvme_mem_init()
{
	/* nothing to do */
	return 0;
}


void
nvme_mem_cleanup()
{
	/* nothing to do */
}


void*
nvme_mem_alloc_node(size_t size, size_t align, unsigned int node_id,
	phys_addr_t* paddr)
{
	size = ROUNDUP(size, B_PAGE_SIZE);

	virtual_address_restrictions virtualRestrictions = {};

	physical_address_restrictions physicalRestrictions = {};
	physicalRestrictions.alignment = align;

	void* address;
	area_id area = create_area_etc(B_SYSTEM_TEAM, "nvme physical buffer",
		size, B_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		0, 0, &virtualRestrictions, &physicalRestrictions, &address);
	if (area < 0)
		return NULL;

	if (paddr != NULL)
		*paddr = nvme_mem_vtophys(address);
	return address;
}


void*
nvme_malloc_node(size_t size, size_t align, unsigned int node_id)
{
	return nvme_mem_alloc_node(size, align, node_id, NULL);
}


void
nvme_free(void* addr)
{
	delete_area(area_for(addr));
}


phys_addr_t
nvme_mem_vtophys(void* vaddr)
{
	physical_entry entry;
	status_t status = get_memory_map((void*)vaddr, 1, &entry, 1);
	if (status != B_OK) {
		panic("nvme: get_memory_map failed for %p: %s\n",
			(void*)vaddr, strerror(status));
		return NVME_VTOPHYS_ERROR;
	}

	return entry.address;
}


// #pragma mark - PCI


int
nvme_pci_init()
{
	status_t status = get_module(B_PCI_MODULE_NAME,
		(module_info**)&sPCIModule);
	return status;
}


int
nvme_pcicfg_read32(struct pci_device* dev, uint32_t* value, uint32_t offset)
{
	*value = sPCIModule->read_pci_config(dev->bus, dev->dev, dev->func, offset,
		sizeof(*value));
	return 0;
}


int
nvme_pcicfg_write32(struct pci_device* dev, uint32_t value, uint32_t offset)
{
	sPCIModule->write_pci_config(dev->bus, dev->dev, dev->func, offset,
		sizeof(value), value);
	return 0;
}


int
nvme_pcicfg_map_bar(void* devhandle, unsigned int bar, bool read_only,
	void** mapped_addr)
{
	struct pci_device* dev = (struct pci_device*)devhandle;
	pci_info* info = (pci_info*)dev->pci_info;

	uint64 addr = info->u.h0.base_registers[bar];
	if ((info->u.h0.base_register_flags[0] & PCI_address_type)
			== PCI_address_type_64) {
		addr |= (uint64)info->u.h0.base_registers[1] << 32;
	}

	uint32 size = info->u.h0.base_register_sizes[bar];
	area_id area = map_physical_memory("nvme mapped bar", (phys_addr_t)addr,
		size, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | (read_only ? 0 : B_KERNEL_WRITE_AREA),
		mapped_addr);
	if (area < B_OK)
		return area;

	return 0;
}


int
nvme_pcicfg_map_bar_write_combine(void* devhandle, unsigned int bar,
	void** mapped_addr)
{
	status_t status = nvme_pcicfg_map_bar(devhandle, bar, false, mapped_addr);
	if (status != 0)
		return status;

	// Turn on write combining for the area
	status = vm_set_area_memory_type(area_for(*mapped_addr),
		nvme_mem_vtophys(*mapped_addr), B_MTR_WC);
	if (status != 0)
		nvme_pcicfg_unmap_bar(devhandle, bar, *mapped_addr);
	return status;
}


int
nvme_pcicfg_unmap_bar(void* devhandle, unsigned int bar, void* addr)
{
	return delete_area(area_for(addr));
}


void
nvme_pcicfg_get_bar_addr_len(void* devhandle, unsigned int bar,
	uint64_t* addr, uint64_t* size)
{
	struct pci_device* dev = (struct pci_device*)devhandle;
	pci_info* info = (pci_info*)dev->pci_info;

	*addr = info->u.h0.base_registers[bar];
	*size = info->u.h0.base_register_sizes[bar];
}


// #pragma mark - logging


void
nvme_log(enum nvme_log_level level, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	nvme_vlog(level, format, ap);
	va_end(ap);
}


void
nvme_vlog(enum nvme_log_level level, const char *format, va_list ap)
{
	dvprintf(format, ap);
}

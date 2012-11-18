/*
 * Copyright 2004-2009, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "util.h"

#include <KernelExport.h>
#include <OS.h>
#include <vm/vm.h>
#include <string.h>


#define TRACE(a...) dprintf("ahci: " a)
#define ERROR(a...) dprintf("ahci: " a)


static inline uint32
round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}


area_id
alloc_mem(void **virt, phys_addr_t *phy, size_t size, uint32 protection,
	const char *name)
{
	physical_entry pe;
	void * virtadr;
	area_id areaid;
	status_t rv;

	TRACE("allocating %ld bytes for %s\n", size, name);

	size = round_to_pagesize(size);
	areaid = create_area(name, &virtadr, B_ANY_KERNEL_ADDRESS, size,
		B_CONTIGUOUS, protection);
	if (areaid < B_OK) {
		ERROR("couldn't allocate area %s\n", name);
		return B_ERROR;
	}
	rv = get_memory_map(virtadr, size, &pe, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		ERROR("couldn't get mapping for %s\n", name);
		return B_ERROR;
	}
	if (virt)
		*virt = virtadr;
	if (phy)
		*phy = pe.address;
	TRACE("area = %" B_PRId32 ", size = %ld, virt = %p, phy = %#" B_PRIxPHYSADDR "\n",
		areaid, size, virtadr, pe.address);
	return areaid;
}


area_id
map_mem(void **virt, phys_addr_t phy, size_t size, uint32 protection,
	const char *name)
{
	uint32 offset;
	phys_addr_t phyadr;
	void *mapadr;
	area_id area;

	TRACE("mapping physical address %#" B_PRIxPHYSADDR " with %" B_PRIuSIZE
		" bytes for %s\n", phy, size, name);

	offset = phy & (B_PAGE_SIZE - 1);
	phyadr = phy - offset;
	size = round_to_pagesize(size + offset);
	area = map_physical_memory(name, phyadr, size,
		B_ANY_KERNEL_BLOCK_ADDRESS, protection, &mapadr);
	if (area < B_OK) {
		ERROR("mapping '%s' failed, error 0x%" B_PRIx32 " (%s)\n", name,
			area, strerror(area));
		return area;
	}

	*virt = (char *)mapadr + offset;

	TRACE("physical = %#" B_PRIxPHYSADDR ", virtual = %p, offset = %"
		B_PRId32 ", phyadr = %#" B_PRIxPHYSADDR ", mapadr = %p, size = %"
		B_PRIuSIZE ", area = 0x%08" B_PRIx32 "\n", phy, *virt, offset, phyadr,
		mapadr, size, area);

	return area;
}


status_t
sg_memcpy(const physical_entry *sgTable, int sgCount, const void *data,
	size_t dataSize)
{
	int i;
	for (i = 0; i < sgCount && dataSize > 0; i++) {
		size_t size = min_c(dataSize, sgTable[i].size);

		TRACE("sg_memcpy phyAddr %#" B_PRIxPHYSADDR ", size %lu\n",
			sgTable[i].address, size);

		vm_memcpy_to_physical(sgTable[i].address, data, size, false);

		data = (char *)data + size;
		dataSize -= size;
	}
	if (dataSize != 0)
		return B_ERROR;
	return B_OK;
}


void
swap_words(void *data, size_t size)
{
	uint16 *word = (uint16*)data;
	size_t count = size / 2;
	while (count--) {
		*word = (*word << 8) | (*word >> 8);
		word++;
	}
}


int
fls(unsigned mask)
{
	if (mask == 0)
		return 0;
	int pos = 1;
	while (mask != 1) {
		mask >>= 1;
		pos++;
	}
	return pos;
}

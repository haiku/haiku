#include <Errors.h>
#include <OS.h>
#include <string.h>

#include "debug.h"
#include "util.h"

uint32 round_to_pagesize(uint32 size);

uint32 round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}

area_id
alloc_mem(void **virt, void **phy, size_t size, uint32 protection, const char *name)
{
	physical_entry pe;
	void * virtadr;
	area_id areaid;
	status_t rv;
	
	LOG("allocating %ld bytes for %s\n", size, name);

	size = round_to_pagesize(size);
	areaid = create_area(name, &virtadr, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK | B_CONTIGUOUS, protection);
	if (areaid < B_OK) {
		LOG("couldn't allocate area %s\n", name);
		return B_ERROR;
	}
	rv = get_memory_map(virtadr, size, &pe, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		LOG("couldn't map %s\n", name);
		return B_ERROR;
	}
	memset(virtadr, 0, size);
	if (virt)
		*virt = virtadr;
	if (phy)
		*phy = pe.address;
	LOG("area = %ld, size = %ld, virt = %p, phy = %p\n", areaid, size, virtadr, pe.address);
	return areaid;
}

/* This is not the most advanced method to map physical memory for io access.
 * Perhaps using B_ANY_KERNEL_ADDRESS instead of B_ANY_KERNEL_BLOCK_ADDRESS
 * makes the whole offset calculation and relocation obsolete. But the code
 * below does work, and I can't test if using B_ANY_KERNEL_ADDRESS also works.
 */
area_id
map_mem(void **virt, void *phy, size_t size, uint32 protection, const char *name)
{
	uint32 offset;
	void *phyadr;
	void *mapadr;
	area_id area;

	LOG("mapping physical address %p with %ld bytes for %s\n", phy, size, name);

	offset = (uint32)phy & (B_PAGE_SIZE - 1);
	phyadr = (char *)phy - offset;
	size = round_to_pagesize(size + offset);
	area = map_physical_memory(name, phyadr, size, B_ANY_KERNEL_BLOCK_ADDRESS, protection, &mapadr);
	if (area < B_OK) {
		LOG("mapping failed, error 0x%x (%s)\n", area, strerror(area));
		return area;
	}
	
	*virt = (char *)mapadr + offset;

	LOG("physical = %p, virtual = %p, offset = %ld, phyadr = %p, mapadr = %p, size = %ld, area = 0x%08lx\n",
		phy, *virt, offset, phyadr, mapadr, size, area);
	
	return area;
}

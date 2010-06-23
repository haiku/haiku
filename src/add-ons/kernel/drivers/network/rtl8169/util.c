/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <Errors.h>
#include <OS.h>
#include <string.h>

//#define DEBUG

#include "debug.h"
#include "util.h"


static inline uint32
round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}


area_id
alloc_contiguous(void **virt, void **phy, size_t size, uint32 protection,
	const char *name)
{
// TODO: phy should be phys_addr_t*!
	physical_entry pe;
	void * virtadr;
	area_id areaid;
	status_t rv;

	TRACE("allocating %ld bytes for %s\n", size, name);

	size = round_to_pagesize(size);
	areaid = create_area(name, &virtadr, B_ANY_KERNEL_ADDRESS, size,
		B_32_BIT_CONTIGUOUS, protection);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
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
	memset(virtadr, 0, size);
	if (virt)
		*virt = virtadr;
	if (phy)
		*phy = (void*)(addr_t)pe.address;
	TRACE("area = %ld, size = %ld, virt = %p, phy = %p\n", areaid, size, virtadr, pe.address);
	return areaid;
}


area_id
map_mem(void **virt, void *phy, size_t size, uint32 protection,
	const char *name)
{
	uint32 offset;
	void *phyadr;
	void *mapadr;
	area_id area;

	TRACE("mapping physical address %p with %ld bytes for %s\n", phy, size, name);

	offset = (uint32)phy & (B_PAGE_SIZE - 1);
	phyadr = (char *)phy - offset;
	size = round_to_pagesize(size + offset);
	area = map_physical_memory(name, (addr_t)phyadr, size, B_ANY_KERNEL_ADDRESS,
		protection, &mapadr);
	if (area < B_OK) {
		ERROR("mapping '%s' failed, error 0x%lx (%s)\n", name, area, strerror(area));
		return area;
	}

	*virt = (char *)mapadr + offset;

	TRACE("physical = %p, virtual = %p, offset = %ld, phyadr = %p, mapadr = "
		"%p, size = %ld, area = 0x%08lx\n", phy, *virt, offset, phyadr, mapadr,
		size, area);

	return area;
}
